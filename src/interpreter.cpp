#include "interpreter.h"

#include <iostream>

int* Interpreter::findVariable(const std::string& name) {
    // Reverse traversal implements lexical shadowing: the nearest declaration wins.
    for (auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope) {
        const auto found = scope->find(name);
        if (found != scope->end()) return &found->second;
    }
    return nullptr;
}

int Interpreter::evaluateExpression(const Expression* expression, bool& success) {
    if (const auto* number = dynamic_cast<const NumberExpr*>(expression)) {
        return number->value;
    }
    if (const auto* variable = dynamic_cast<const VariableExpr*>(expression)) {
        int* value = findVariable(variable->name);
        if (!value) {
            std::cerr << "Runtime Error: Variable '" << variable->name
                      << "' has no runtime value.\n";
            success = false;
            return 0;
        }
        return *value;
    }
    if (const auto* unary = dynamic_cast<const UnaryExpr*>(expression)) {
        const int value = evaluateExpression(unary->operand.get(), success);
        if (!success) return 0;
        if (unary->op == "-") return -value;
        if (unary->op == "!") return !value;

        std::cerr << "Runtime Error: Unknown unary operator '" << unary->op << "'.\n";
        success = false;
        return 0;
    }
    if (const auto* call = dynamic_cast<const CallExpr*>(expression)) {
        std::vector<int> arguments;
        for (const auto& argument : call->arguments) {
            arguments.push_back(evaluateExpression(argument.get(), success));
            if (!success) return 0;
        }

        const auto function = functions.find(call->name);
        if (function == functions.end()) {
            std::cerr << "Runtime Error: Unknown function '" << call->name << "'.\n";
            success = false;
            return 0;
        }

        int result = 0;
        if (!executeFunction(function->second, arguments, result, success)) success = false;
        return result;
    }
    if (const auto* binary = dynamic_cast<const BinaryExpr*>(expression)) {
        const int left = evaluateExpression(binary->left.get(), success);
        if (!success) return 0;

        // These early returns implement logical short-circuiting and prevent
        // evaluation of a right operand that cannot affect the result.
        if (binary->op == "&&" && left == 0) return 0;
        if (binary->op == "||" && left != 0) return 1;

        const int right = evaluateExpression(binary->right.get(), success);
        if (!success) return 0;
        if (binary->op == "+") return left + right;
        if (binary->op == "-") return left - right;
        if (binary->op == "*") return left * right;
        if (binary->op == "/") {
            if (right == 0) {
                std::cerr << "Runtime Error: Division by zero.\n";
                success = false;
                return 0;
            }
            return left / right;
        }
        if (binary->op == "<") return left < right;
        if (binary->op == ">") return left > right;
        if (binary->op == "<=") return left <= right;
        if (binary->op == ">=") return left >= right;
        if (binary->op == "==") return left == right;
        if (binary->op == "!=") return left != right;
        if (binary->op == "&&") return left && right;
        if (binary->op == "||") return left || right;

        std::cerr << "Runtime Error: Unknown operator '" << binary->op << "'.\n";
        success = false;
        return 0;
    }

    std::cerr << "Runtime Error: Unknown expression.\n";
    success = false;
    return 0;
}

bool Interpreter::executeStatement(const Statement* statement, bool& success,
                                  bool& didReturn, int& returnValue) {
    if (const auto* declaration = dynamic_cast<const VariableDeclaration*>(statement)) {
        const int value = evaluateExpression(declaration->value.get(), success);
        if (!success) return false;
        if (scopes.back().count(declaration->name)) {
            std::cerr << "Runtime Error: Duplicate variable '" << declaration->name << "'.\n";
            return false;
        }
        scopes.back()[declaration->name] = value;
        return true;
    }
    if (const auto* assignment = dynamic_cast<const Assignment*>(statement)) {
        const int value = evaluateExpression(assignment->value.get(), success);
        if (!success) return false;
        int* target = findVariable(assignment->name);
        if (!target) {
            std::cerr << "Runtime Error: Variable '" << assignment->name << "' is not declared.\n";
            return false;
        }
        *target = value;
        return true;
    }
    if (const auto* print = dynamic_cast<const PrintStatement*>(statement)) {
        const int value = evaluateExpression(print->value.get(), success);
        if (!success) return false;
        std::cout << "Program Output: " << value << '\n';
        return true;
    }
    if (const auto* block = dynamic_cast<const BlockStatement*>(statement)) {
        // A block creates a scope that is removed even when a child fails or returns.
        scopes.emplace_back();
        for (const auto& child : block->statements) {
            if (!executeStatement(child.get(), success, didReturn, returnValue)) {
                scopes.pop_back();
                return false;
            }
            if (didReturn) break;
        }
        scopes.pop_back();
        return success;
    }
    if (const auto* conditional = dynamic_cast<const IfStatement*>(statement)) {
        const int condition = evaluateExpression(conditional->condition.get(), success);
        if (!success) return false;
        if (condition) {
            return executeStatement(conditional->thenBlock.get(), success, didReturn, returnValue);
        }
        if (conditional->elseBlock) {
            return executeStatement(conditional->elseBlock.get(), success, didReturn, returnValue);
        }
        return true;
    }
    if (const auto* loop = dynamic_cast<const WhileStatement*>(statement)) {
        while (success) {
            const int condition = evaluateExpression(loop->condition.get(), success);
            if (!success || condition == 0) break;
            if (!executeStatement(loop->body.get(), success, didReturn, returnValue)) return false;
            if (didReturn) break;
        }
        return success;
    }
    if (const auto* ret = dynamic_cast<const ReturnStatement*>(statement)) {
        returnValue = evaluateExpression(ret->value.get(), success);
        if (!success) return false;
        didReturn = true;
        return true;
    }
    // Declarations are registered before top-level execution and skipped here.
    if (dynamic_cast<const FunctionDeclaration*>(statement)) return true;

    std::cerr << "Runtime Error: Unknown statement.\n";
    return false;
}

bool Interpreter::executeFunction(const FunctionDeclaration* function,
                                  const std::vector<int>& arguments,
                                  int& result, bool& success) {
    if (arguments.size() != function->parameters.size()) {
        std::cerr << "Runtime Error: Incorrect argument count for '" << function->name << "'.\n";
        success = false;
        return false;
    }

    // Parameters live in a new function scope; the function body block then
    // creates its own nested scope according to normal block semantics.
    scopes.emplace_back();
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        scopes.back()[function->parameters[i]] = arguments[i];
    }

    bool didReturn = false;
    int returnValue = 0;
    const bool executed = executeStatement(function->body.get(), success, didReturn, returnValue);
    scopes.pop_back();

    if (!executed || !success) return false;
    result = didReturn ? returnValue : 0;
    return true;
}

bool Interpreter::execute(const Program* program) {
    scopes.clear();
    scopes.emplace_back();
    functions.clear();
    if (!program) return false;

    // Register all functions first so calls can target later declarations and recurse.
    for (const auto& statement : program->statements) {
        if (const auto* function = dynamic_cast<const FunctionDeclaration*>(statement.get())) {
            functions[function->name] = function;
        }
    }

    bool success = true;
    bool didReturn = false;
    int returnValue = 0;
    for (const auto& statement : program->statements) {
        if (dynamic_cast<const FunctionDeclaration*>(statement.get())) continue;
        if (!executeStatement(statement.get(), success, didReturn, returnValue) || !success) return false;
        if (didReturn) {
            std::cerr << "Runtime Error: Return outside function.\n";
            return false;
        }
    }
    return true;
}

void Interpreter::printEnvironment() const {
    std::cout << "\n=== RUNTIME ENVIRONMENT ===\n\n";
    if (scopes.empty() || scopes.front().empty()) {
        std::cout << "No variables were created.\n";
    } else {
        for (const auto& entry : scopes.front()) {
            std::cout << entry.first << " = " << entry.second << '\n';
        }
    }
    std::cout << '\n';
}
