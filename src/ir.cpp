#include "ir.h"

#include <iostream>

// Temporary names and labels are numbered independently for easier inspection.
std::string IRGenerator::newTemp() {
    return "t" + std::to_string(++tempCount);
}

std::string IRGenerator::newLabel() {
    return "L" + std::to_string(++labelCount);
}

std::string IRGenerator::generateExpression(const Expression* expression) {
    if (const auto* number = dynamic_cast<const NumberExpr*>(expression)) {
        return std::to_string(number->value);
    }
    if (const auto* variable = dynamic_cast<const VariableExpr*>(expression)) {
        return variable->name;
    }
    if (const auto* unary = dynamic_cast<const UnaryExpr*>(expression)) {
        const auto operand = generateExpression(unary->operand.get());
        const auto temporary = newTemp();
        instructions.push_back(temporary + " = " + unary->op + operand);
        return temporary;
    }
    if (const auto* call = dynamic_cast<const CallExpr*>(expression)) {
        // Emit arguments in source order before the call instruction.
        for (const auto& argument : call->arguments) {
            instructions.push_back("PARAM " + generateExpression(argument.get()));
        }
        const auto temporary = newTemp();
        instructions.push_back(temporary + " = CALL " + call->name + ", " +
                               std::to_string(call->arguments.size()));
        return temporary;
    }
    if (const auto* binary = dynamic_cast<const BinaryExpr*>(expression)) {
        if (binary->op == "&&" || binary->op == "||") {
            // Logical operators need jumps rather than eager evaluation so the
            // right operand is skipped when the left operand decides the result.
            const auto temporary = newTemp();
            const auto shortCircuitLabel = newLabel();
            const auto endLabel = newLabel();
            const auto left = generateExpression(binary->left.get());

            instructions.push_back(temporary + " = " +
                                   (binary->op == "&&" ? "0" : "1"));
            instructions.push_back(std::string(binary->op == "&&" ? "IF_FALSE " : "IF_TRUE ") +
                                   left + " GOTO " + shortCircuitLabel);

            const auto right = generateExpression(binary->right.get());
            instructions.push_back(std::string(binary->op == "&&" ? "IF_FALSE " : "IF_TRUE ") +
                                   right + " GOTO " + shortCircuitLabel);
            instructions.push_back(temporary + " = " +
                                   (binary->op == "&&" ? "1" : "0"));
            instructions.push_back("GOTO " + endLabel);
            instructions.push_back("LABEL " + shortCircuitLabel);
            instructions.push_back("LABEL " + endLabel);
            return temporary;
        }

        // Ordinary binary expressions evaluate both operands and then emit one
        // three-address operation into a fresh temporary.
        const auto left = generateExpression(binary->left.get());
        const auto right = generateExpression(binary->right.get());
        const auto temporary = newTemp();
        instructions.push_back(temporary + " = " + left + " " + binary->op + " " + right);
        return temporary;
    }

    // Defensive fallback for an unexpected AST node.
    return "0";
}

void IRGenerator::generateStatement(const Statement* statement) {
    if (const auto* declaration = dynamic_cast<const VariableDeclaration*>(statement)) {
        instructions.push_back(declaration->name + " = " +
                               generateExpression(declaration->value.get()));
    } else if (const auto* assignment = dynamic_cast<const Assignment*>(statement)) {
        instructions.push_back(assignment->name + " = " +
                               generateExpression(assignment->value.get()));
    } else if (const auto* print = dynamic_cast<const PrintStatement*>(statement)) {
        instructions.push_back("PRINT " + generateExpression(print->value.get()));
    } else if (const auto* block = dynamic_cast<const BlockStatement*>(statement)) {
        for (const auto& child : block->statements) generateStatement(child.get());
    } else if (const auto* conditional = dynamic_cast<const IfStatement*>(statement)) {
        const auto condition = generateExpression(conditional->condition.get());
        const auto otherwiseLabel = newLabel();
        const auto endLabel = newLabel();

        instructions.push_back("IF_FALSE " + condition + " GOTO " + otherwiseLabel);
        generateStatement(conditional->thenBlock.get());
        if (conditional->elseBlock) {
            instructions.push_back("GOTO " + endLabel);
            instructions.push_back("LABEL " + otherwiseLabel);
            generateStatement(conditional->elseBlock.get());
            instructions.push_back("LABEL " + endLabel);
        } else {
            instructions.push_back("LABEL " + otherwiseLabel);
        }
    } else if (const auto* loop = dynamic_cast<const WhileStatement*>(statement)) {
        const auto startLabel = newLabel();
        const auto endLabel = newLabel();

        // The condition is emitted at the loop header so it is reevaluated on
        // every iteration rather than only once before entering the loop.
        instructions.push_back("LABEL " + startLabel);
        const auto condition = generateExpression(loop->condition.get());
        instructions.push_back("IF_FALSE " + condition + " GOTO " + endLabel);
        generateStatement(loop->body.get());
        instructions.push_back("GOTO " + startLabel);
        instructions.push_back("LABEL " + endLabel);
    } else if (const auto* ret = dynamic_cast<const ReturnStatement*>(statement)) {
        instructions.push_back("RETURN " + generateExpression(ret->value.get()));
    } else if (const auto* function = dynamic_cast<const FunctionDeclaration*>(statement)) {
        instructions.push_back("FUNC " + function->name);
        for (const auto& parameter : function->parameters) {
            instructions.push_back("PARAM_DECL " + parameter);
        }
        generateStatement(function->body.get());
        instructions.push_back("END_FUNC " + function->name);
    }
}

void IRGenerator::generate(const Program* program) {
    // Reset state so reusing one generator does not append stale instructions.
    instructions.clear();
    tempCount = 0;
    labelCount = 0;
    if (program) {
        for (const auto& statement : program->statements) generateStatement(statement.get());
    }
}

void IRGenerator::printTAC() const {
    std::cout << "\n=== THREE-ADDRESS CODE ===\n\n";
    if (instructions.empty()) {
        std::cout << "No intermediate code generated.\n";
    } else {
        for (const auto& instruction : instructions) std::cout << instruction << '\n';
    }
    std::cout << '\n';
}
