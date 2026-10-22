#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H
#include "ast.h"
#include <map>
#include <string>
#include <vector>
class SemanticAnalyzer {
    std::vector<std::map<std::string, std::string>> scopes;
    std::map<std::string, const FunctionDeclaration*> functions;
    bool hasErrors = false;
    std::string currentFunction;
    bool declareVariable(const std::string& name);
    bool variableExists(const std::string& name) const;
    void analyzeStatement(const Statement* statement);
    void analyzeExpression(const Expression* expression);
public:
    SemanticAnalyzer();
    bool analyze(const Program* program);
    void printSymbolTable() const;
};
#endif
