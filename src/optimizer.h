#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "ast.h"

#include <cstddef>

class Optimizer {
    std::size_t foldedExpressions = 0;
    std::size_t removedStatements = 0;
    std::size_t simplifiedBranches = 0;

    void optimizeExpression(std::unique_ptr<Expression>& expression);
    void optimizeStatement(std::unique_ptr<Statement>& statement);
    void optimizeStatementList(std::vector<std::unique_ptr<Statement>>& statements);
    bool alwaysReturns(const Statement* statement) const;

public:
    void optimize(Program* program);
    void printSummary() const;
};

#endif
