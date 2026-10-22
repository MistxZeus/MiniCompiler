#ifndef IR_H
#define IR_H
#include "ast.h"
#include <string>
#include <vector>
class IRGenerator {
    std::vector<std::string> instructions; int tempCount=0, labelCount=0;
    std::string newTemp(); std::string newLabel();
    std::string generateExpression(const Expression* expression);
    void generateStatement(const Statement* statement);
public:
    void generate(const Program* program); void printTAC() const;
};
#endif
