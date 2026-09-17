#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "ast.h"
#include "semantic.h"

class SemanticAnalyzer {
private:
    SymbolTable symbolTable;
    bool hasErrors;

    void analyzeStatement(const Statement* statement);
    void analyzeExpression(const Expression* expression);

public:
    SemanticAnalyzer();

    bool analyze(const Program* program);

    void printSymbolTable() const;
};

#endif