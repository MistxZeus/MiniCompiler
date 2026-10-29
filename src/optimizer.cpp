// AST optimizer: folds safe constant expressions, simplifies known branches,
// and removes statements that cannot execute after an unconditional return.
#include "optimizer.h"

#include <climits>
#include <iostream>
#include <limits>
#include <memory>
#include <string>

namespace {
// A small helper keeps constant checks explicit without changing node ownership.
const NumberExpr* number(const Expression* expression) {
    return dynamic_cast<const NumberExpr*>(expression);
}

// Compute a constant operation only when the result is representable as int.
// Returning false leaves operations such as division by zero for runtime handling.
bool calculateBinary(const std::string& op, int left, int right, int& result) {
    const long long a = left;
    const long long b = right;
    long long value = 0;

    if (op == "+") value = a + b;
    else if (op == "-") value = a - b;
    else if (op == "*") value = a * b;
    else if (op == "/") {
        if (right == 0 || (left == INT_MIN && right == -1)) return false;
        result = left / right;
        return true;
    }
    else if (op == "<") value = left < right;
    else if (op == ">") value = left > right;
    else if (op == "<=") value = left <= right;
    else if (op == ">=") value = left >= right;
    else if (op == "==") value = left == right;
    else if (op == "!=") value = left != right;
    else if (op == "&&") value = (left != 0) && (right != 0);
    else if (op == "||") value = (left != 0) || (right != 0);
    else return false;

    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max()) return false;
    result = static_cast<int>(value);
    return true;
}
}

// Optimize children before their parent so newly constant parent expressions
// can be folded in the same pass.
void Optimizer::optimizeExpression(std::unique_ptr<Expression>& expression) {
    if (!expression) return;

    if (auto* unary = dynamic_cast<UnaryExpr*>(expression.get())) {
        optimizeExpression(unary->operand);
        if (const auto* operand = number(unary->operand.get())) {
            if (unary->op == "!") {
                expression = std::make_unique<NumberExpr>(operand->value == 0 ? 1 : 0);
                ++foldedExpressions;
            } else if (unary->op == "-" && operand->value != INT_MIN) {
                expression = std::make_unique<NumberExpr>(-operand->value);
                ++foldedExpressions;
            }
        }
        return;
    }

    if (auto* call = dynamic_cast<CallExpr*>(expression.get())) {
        for (auto& argument : call->arguments) optimizeExpression(argument);
        return;
    }

    auto* binary = dynamic_cast<BinaryExpr*>(expression.get());
    if (!binary) return;

    optimizeExpression(binary->left);
    if (const auto* left = number(binary->left.get())) {
        // Preserve the language's short-circuit semantics: the right side is
        // not evaluated when the left operand already determines the result.
        if (binary->op == "&&" && left->value == 0) {
            expression = std::make_unique<NumberExpr>(0);
            ++foldedExpressions;
            return;
        }
        if (binary->op == "||" && left->value != 0) {
            expression = std::make_unique<NumberExpr>(1);
            ++foldedExpressions;
            return;
        }
    }
    optimizeExpression(binary->right);

    const auto* left = number(binary->left.get());
    const auto* right = number(binary->right.get());
    if (left && right) {
        int result = 0;
        if (calculateBinary(binary->op, left->value, right->value, result)) {
            expression = std::make_unique<NumberExpr>(result);
            ++foldedExpressions;
            return;
        }
    }

    // Safe algebraic identities that do not remove side effects from the
    // other operand. We intentionally do not simplify x * 0 or x - x because
    // evaluating x may call a function or fail at runtime.
    if (right && binary->op == "+" && right->value == 0) {
        auto replacement = std::move(binary->left);
        expression = std::move(replacement);
        ++foldedExpressions;
    } else if (right && binary->op == "-" && right->value == 0) {
        auto replacement = std::move(binary->left);
        expression = std::move(replacement);
        ++foldedExpressions;
    } else if (right && binary->op == "*" && right->value == 1) {
        auto replacement = std::move(binary->left);
        expression = std::move(replacement);
        ++foldedExpressions;
    } else if (right && binary->op == "/" && right->value == 1) {
        auto replacement = std::move(binary->left);
        expression = std::move(replacement);
        ++foldedExpressions;
    } else if (left && binary->op == "+" && left->value == 0) {
        auto replacement = std::move(binary->right);
        expression = std::move(replacement);
        ++foldedExpressions;
    } else if (left && binary->op == "*" && left->value == 1) {
        auto replacement = std::move(binary->right);
        expression = std::move(replacement);
        ++foldedExpressions;
    }
}

// Used for dead-statement elimination. A conditional is considered guaranteed
// to return only when both branches exist and both return.
bool Optimizer::alwaysReturns(const Statement* statement) const {
    if (dynamic_cast<const ReturnStatement*>(statement)) return true;
    if (const auto* block = dynamic_cast<const BlockStatement*>(statement)) {
        for (const auto& child : block->statements) {
            if (alwaysReturns(child.get())) return true;
        }
        return false;
    }
    if (const auto* conditional = dynamic_cast<const IfStatement*>(statement)) {
        return conditional->elseBlock && alwaysReturns(conditional->thenBlock.get()) &&
               alwaysReturns(conditional->elseBlock.get());
    }
    return false;
}

void Optimizer::optimizeStatement(std::unique_ptr<Statement>& statement) {
    if (!statement) return;
    if (auto* declaration = dynamic_cast<VariableDeclaration*>(statement.get())) {
        optimizeExpression(declaration->value);
    } else if (auto* assignment = dynamic_cast<Assignment*>(statement.get())) {
        optimizeExpression(assignment->value);
    } else if (auto* print = dynamic_cast<PrintStatement*>(statement.get())) {
        optimizeExpression(print->value);
    } else if (auto* block = dynamic_cast<BlockStatement*>(statement.get())) {
        optimizeStatementList(block->statements);
    } else if (auto* conditional = dynamic_cast<IfStatement*>(statement.get())) {
        optimizeExpression(conditional->condition);
        const auto* constant = number(conditional->condition.get());
        if (constant) {
            if (constant->value != 0) {
                optimizeStatementList(conditional->thenBlock->statements);
                std::unique_ptr<Statement> replacement = std::move(conditional->thenBlock);
                statement = std::move(replacement);
            } else if (conditional->elseBlock) {
                optimizeStatementList(conditional->elseBlock->statements);
                std::unique_ptr<Statement> replacement = std::move(conditional->elseBlock);
                statement = std::move(replacement);
            } else {
                statement = std::make_unique<BlockStatement>();
            }
            ++simplifiedBranches;
            return;
        }
        optimizeStatementList(conditional->thenBlock->statements);
        if (conditional->elseBlock) optimizeStatementList(conditional->elseBlock->statements);
    } else if (auto* loop = dynamic_cast<WhileStatement*>(statement.get())) {
        optimizeExpression(loop->condition);
        if (const auto* constant = number(loop->condition.get()); constant && constant->value == 0) {
            statement = std::make_unique<BlockStatement>();
            ++simplifiedBranches;
            ++removedStatements;
            return;
        }
        optimizeStatementList(loop->body->statements);
    } else if (auto* ret = dynamic_cast<ReturnStatement*>(statement.get())) {
        optimizeExpression(ret->value);
    } else if (auto* function = dynamic_cast<FunctionDeclaration*>(statement.get())) {
        optimizeStatementList(function->body->statements);
    }
}

// Process a statement list in order. Once a statement always returns, all
// later statements in that same list are unreachable.
void Optimizer::optimizeStatementList(std::vector<std::unique_ptr<Statement>>& statements) {
    for (std::size_t i = 0; i < statements.size(); ++i) {
        optimizeStatement(statements[i]);
        if (alwaysReturns(statements[i].get()) && i + 1 < statements.size()) {
            removedStatements += statements.size() - (i + 1);
            statements.erase(statements.begin() + static_cast<std::ptrdiff_t>(i + 1), statements.end());
            break;
        }
    }
}

// Reset counters on every invocation so the summary is pass-local.
void Optimizer::optimize(Program* program) {
    foldedExpressions = removedStatements = simplifiedBranches = 0;
    if (program) optimizeStatementList(program->statements);
}

void Optimizer::printSummary() const {
    std::cout << "Constant expressions folded: " << foldedExpressions << '\n'
              << "Dead statements removed: " << removedStatements << '\n'
              << "Constant branches simplified: " << simplifiedBranches << '\n';
}
