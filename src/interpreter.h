#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"

#include <map>
#include <string>
#include <vector>

// Direct AST interpreter used by the final execution stage of the pipeline.
// Each map is one lexical scope; later entries in the vector are inner scopes.
class Interpreter {
    std::vector<std::map<std::string, int>> scopes;
    std::map<std::string, const FunctionDeclaration*> functions;

    int evaluateExpression(const Expression* expression, bool& success);
    bool executeStatement(const Statement* statement, bool& success,
                          bool& didReturn, int& returnValue);
    bool executeFunction(const FunctionDeclaration* function,
                         const std::vector<int>& arguments, int& result,
                         bool& success);

    // Searches from the innermost scope outward and returns null if undefined.
    int* findVariable(const std::string& name);

public:
    bool execute(const Program* program);
    void printEnvironment() const;
};

#endif // INTERPRETER_H
