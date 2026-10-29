#ifndef IR_H
#define IR_H

#include "ast.h"

#include <string>
#include <vector>

// Converts AST nodes into a textual three-address-code representation.
// Instructions are retained in order and printed after the whole program is lowered.
class IRGenerator {
    std::vector<std::string> instructions;
    int tempCount = 0;
    int labelCount = 0;

    std::string newTemp();
    std::string newLabel();
    std::string generateExpression(const Expression* expression);
    void generateStatement(const Statement* statement);

public:
    void generate(const Program* program);
    void printTAC() const;
};

#endif // IR_H
