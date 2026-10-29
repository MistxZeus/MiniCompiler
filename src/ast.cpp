#include "ast.h"

#include <iostream>

namespace {
// Keep indentation logic local to this translation unit; the public API only
// exposes printAST(), not the individual expression/statement printers.
void indent(int depth) {
    while (depth--) std::cout << "  ";
}

void printExpr(const Expression* expression, int depth) {
    indent(depth);

    if (const auto* number = dynamic_cast<const NumberExpr*>(expression)) {
        std::cout << "Number: " << number->value << '\n';
    } else if (const auto* variable = dynamic_cast<const VariableExpr*>(expression)) {
        std::cout << "Variable: " << variable->name << '\n';
    } else if (const auto* binary = dynamic_cast<const BinaryExpr*>(expression)) {
        std::cout << "Binary: " << binary->op << '\n';
        printExpr(binary->left.get(), depth + 1);
        printExpr(binary->right.get(), depth + 1);
    } else if (const auto* unary = dynamic_cast<const UnaryExpr*>(expression)) {
        std::cout << "Unary: " << unary->op << '\n';
        printExpr(unary->operand.get(), depth + 1);
    } else if (const auto* call = dynamic_cast<const CallExpr*>(expression)) {
        std::cout << "Call: " << call->name << '\n';
        for (const auto& argument : call->arguments) {
            printExpr(argument.get(), depth + 1);
        }
    } else {
        std::cout << "<unknown expression>\n";
    }
}

void printStmt(const Statement* statement, int depth) {
    indent(depth);

    if (const auto* declaration = dynamic_cast<const VariableDeclaration*>(statement)) {
        std::cout << "Declare: " << declaration->name << '\n';
        printExpr(declaration->value.get(), depth + 1);
    } else if (const auto* assignment = dynamic_cast<const Assignment*>(statement)) {
        std::cout << "Assign: " << assignment->name << '\n';
        printExpr(assignment->value.get(), depth + 1);
    } else if (const auto* print = dynamic_cast<const PrintStatement*>(statement)) {
        std::cout << "Print\n";
        printExpr(print->value.get(), depth + 1);
    } else if (const auto* block = dynamic_cast<const BlockStatement*>(statement)) {
        std::cout << "Block\n";
        for (const auto& child : block->statements) printStmt(child.get(), depth + 1);
    } else if (const auto* conditional = dynamic_cast<const IfStatement*>(statement)) {
        std::cout << "If\n";
        printExpr(conditional->condition.get(), depth + 1);
        printStmt(conditional->thenBlock.get(), depth + 1);
        if (conditional->elseBlock) printStmt(conditional->elseBlock.get(), depth + 1);
    } else if (const auto* loop = dynamic_cast<const WhileStatement*>(statement)) {
        std::cout << "While\n";
        printExpr(loop->condition.get(), depth + 1);
        printStmt(loop->body.get(), depth + 1);
    } else if (const auto* ret = dynamic_cast<const ReturnStatement*>(statement)) {
        std::cout << "Return\n";
        printExpr(ret->value.get(), depth + 1);
    } else if (const auto* function = dynamic_cast<const FunctionDeclaration*>(statement)) {
        std::cout << "Function: " << function->name << '(';
        for (std::size_t i = 0; i < function->parameters.size(); ++i) {
            if (i) std::cout << ", ";
            std::cout << function->parameters[i];
        }
        std::cout << ")\n";
        printStmt(function->body.get(), depth + 1);
    } else {
        std::cout << "<unknown statement>\n";
    }
}
} // namespace

// This is a diagnostic tree printer rather than a serializer: output is meant
// for a developer inspecting the parser or optimizer, not for reparsing.
void printAST(const Program* program) {
    std::cout << "\n=== ABSTRACT SYNTAX TREE ===\n\nProgram\n";
    if (program) {
        for (const auto& statement : program->statements) printStmt(statement.get(), 1);
    }
    std::cout << '\n';
}
