#ifndef IR_H
#define IR_H

#include "ast.h"
#include <string>
#include <vector>

// ==========================
// Three-Address Code Generator
// ==========================

class IRGenerator {
private:
    std::vector<std::string> instructions;
    int tempCount = 0;

    // Creates a new temporary variable: t1, t2, t3, ...
    std::string newTemp();

    // Generates TAC for an expression and returns
    // the name/value containing the result.
    std::string generateExpression(const Expression* expression);

    // Generates TAC for a statement.
    void generateStatement(const Statement* statement);

public:
    // Generate TAC for the complete program.
    void generate(const Program* program);

    // Print the generated TAC.
    void printTAC() const;
};

#endif