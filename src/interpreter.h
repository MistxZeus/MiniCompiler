#ifndef INTERPRETER_H
#define INTERPRETER_H
#include "ast.h"
#include <map>
#include <string>
#include <vector>
class Interpreter {
    std::vector<std::map<std::string,int>> scopes;
    std::map<std::string,const FunctionDeclaration*> functions;
    int evaluateExpression(const Expression*, bool& success);
    bool executeStatement(const Statement*, bool& success, bool& didReturn, int& returnValue);
    bool executeFunction(const FunctionDeclaration*, const std::vector<int>&, int&, bool& success);
    int* findVariable(const std::string& name);
public:
    bool execute(const Program* program);
    void printEnvironment() const;
};
#endif
