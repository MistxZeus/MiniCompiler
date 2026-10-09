#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"
#include <map>
#include <string>

class Interpreter {
private:
    // Runtime environment: variable name -> current value
    std::map<std::string, int> environment;

    int evaluateExpression(
        const Expression* expression,
        bool& success
    );

    bool executeStatement(const Statement* statement);

public:
    bool execute(const Program* program);

    void printEnvironment() const;
};

#endif