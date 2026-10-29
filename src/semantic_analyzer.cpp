#include "semantic_analyzer.h"

#include <iostream>

SemanticAnalyzer::SemanticAnalyzer() {
    // Every analysis starts with a global scope.
    scopes.emplace_back();
}

bool SemanticAnalyzer::declareVariable(const std::string& name) {
    auto& currentScope = scopes.back();

    // Duplicate declarations are rejected within one scope, while a nested
    // scope may shadow a name declared in an outer scope.
    if (currentScope.count(name)) return false;
    currentScope[name] = "int";
    return true;
}

bool SemanticAnalyzer::variableExists(const std::string& name) const {
    // Lexical lookup walks from the innermost scope toward the global scope.
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->count(name)) return true;
    }
    return false;
}

bool SemanticAnalyzer::analyze(const Program* program) {
    // Reset all state so an analyzer instance can safely be reused.
    hasErrors = false;
    scopes.clear();
    scopes.emplace_back();
    functions.clear();
    currentFunction.clear();

    if (!program) {
        hasErrors = true;
        return false;
    }

    // Register functions before validating calls, allowing forward references
    // and recursion regardless of where a function appears in the file.
    for (const auto& statement : program->statements) {
        if (const auto* function = dynamic_cast<const FunctionDeclaration*>(statement.get())) {
            if (functions.count(function->name)) {
                std::cout << "Semantic Error: Function '" << function->name
                          << "' is already declared.\n";
                hasErrors = true;
            } else {
                functions[function->name] = function;
            }
        }
    }

    // Analyze top-level executable statements first so function bodies can
    // resolve names declared globally in the program.
    for (const auto& statement : program->statements) {
        if (!dynamic_cast<const FunctionDeclaration*>(statement.get())) {
            analyzeStatement(statement.get());
        }
    }

    // Analyze each function in a fresh local scope containing its parameters.
    for (const auto& statement : program->statements) {
        if (const auto* function = dynamic_cast<const FunctionDeclaration*>(statement.get())) {
            currentFunction = function->name;
            scopes.emplace_back();

            for (const auto& parameter : function->parameters) {
                if (!declareVariable(parameter)) {
                    std::cout << "Semantic Error: Duplicate parameter '" << parameter
                              << "' in function '" << function->name << "'.\n";
                    hasErrors = true;
                }
            }

            analyzeStatement(function->body.get());
            scopes.pop_back();
            currentFunction.clear();
        }
    }

    return !hasErrors;
}

void SemanticAnalyzer::analyzeStatement(const Statement* statement) {
    if (const auto* declaration = dynamic_cast<const VariableDeclaration*>(statement)) {
        // Analyze the initializer before declaring the new name; this prevents
        // a declaration from silently referring to itself during initialization.
        analyzeExpression(declaration->value.get());
        if (!declareVariable(declaration->name)) {
            std::cout << "Semantic Error: Variable '" << declaration->name
                      << "' is already declared in this scope.\n";
            hasErrors = true;
        }
    } else if (const auto* assignment = dynamic_cast<const Assignment*>(statement)) {
        if (!variableExists(assignment->name)) {
            std::cout << "Semantic Error: Variable '" << assignment->name
                      << "' is not declared.\n";
            hasErrors = true;
        }
        analyzeExpression(assignment->value.get());
    } else if (const auto* print = dynamic_cast<const PrintStatement*>(statement)) {
        analyzeExpression(print->value.get());
    } else if (const auto* block = dynamic_cast<const BlockStatement*>(statement)) {
        scopes.emplace_back();
        for (const auto& child : block->statements) analyzeStatement(child.get());
        scopes.pop_back();
    } else if (const auto* conditional = dynamic_cast<const IfStatement*>(statement)) {
        analyzeExpression(conditional->condition.get());
        analyzeStatement(conditional->thenBlock.get());
        if (conditional->elseBlock) analyzeStatement(conditional->elseBlock.get());
    } else if (const auto* loop = dynamic_cast<const WhileStatement*>(statement)) {
        analyzeExpression(loop->condition.get());
        analyzeStatement(loop->body.get());
    } else if (const auto* ret = dynamic_cast<const ReturnStatement*>(statement)) {
        if (currentFunction.empty()) {
            std::cout << "Semantic Error: 'return' is only valid inside a function.\n";
            hasErrors = true;
        }
        analyzeExpression(ret->value.get());
    } else if (dynamic_cast<const FunctionDeclaration*>(statement)) {
        // Function declarations are only accepted at program scope.
        std::cout << "Semantic Error: Nested function declarations are not supported.\n";
        hasErrors = true;
    }
}

void SemanticAnalyzer::analyzeExpression(const Expression* expression) {
    if (const auto* variable = dynamic_cast<const VariableExpr*>(expression)) {
        if (!variableExists(variable->name)) {
            std::cout << "Semantic Error: Variable '" << variable->name
                      << "' is not declared.\n";
            hasErrors = true;
        }
    } else if (const auto* binary = dynamic_cast<const BinaryExpr*>(expression)) {
        analyzeExpression(binary->left.get());
        analyzeExpression(binary->right.get());
    } else if (const auto* unary = dynamic_cast<const UnaryExpr*>(expression)) {
        analyzeExpression(unary->operand.get());
    } else if (const auto* call = dynamic_cast<const CallExpr*>(expression)) {
        for (const auto& argument : call->arguments) analyzeExpression(argument.get());

        const auto function = functions.find(call->name);
        if (function == functions.end()) {
            std::cout << "Semantic Error: Function '" << call->name
                      << "' is not declared.\n";
            hasErrors = true;
        } else if (function->second->parameters.size() != call->arguments.size()) {
            std::cout << "Semantic Error: Function '" << call->name << "' expects "
                      << function->second->parameters.size() << " argument(s), got "
                      << call->arguments.size() << ".\n";
            hasErrors = true;
        }
    }
}

void SemanticAnalyzer::printSymbolTable() const {
    std::cout << "\n=== SYMBOL TABLE (GLOBAL SCOPE) ===\n\nVariable       Type\n---------------------\n";

    if (scopes.empty() || scopes.front().empty()) {
        std::cout << "No global variables declared.\n\n";
        return;
    }

    for (const auto& entry : scopes.front()) {
        const std::size_t padding = entry.first.size() < 15 ? 15 - entry.first.size() : 1;
        std::cout << entry.first << std::string(padding, ' ') << entry.second << '\n';
    }
    std::cout << '\n';
}
