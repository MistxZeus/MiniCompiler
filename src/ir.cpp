#include "ir.h"
#include <iostream>

std::string IRGenerator::newTemp()
{
    return "t" + std::to_string(++tempCount);
}

std::string IRGenerator::generateExpression(
    const Expression* expression)
{
    if (const auto* number =
            dynamic_cast<const NumberExpr*>(expression))
    {
        return std::to_string(number->value);
    }

    if (const auto* variable =
            dynamic_cast<const VariableExpr*>(expression))
    {
        return variable->name;
    }

    if (const auto* binary =
            dynamic_cast<const BinaryExpr*>(expression))
    {
        std::string left =
            generateExpression(binary->left.get());

        std::string right =
            generateExpression(binary->right.get());

        std::string temp = newTemp();

        instructions.push_back(
            temp + " = " +
            left + " " +
            binary->op + " " +
            right
        );

        return temp;
    }

    return "";
}

void IRGenerator::generateStatement(
    const Statement* statement)
{
    if (const auto* declaration =
            dynamic_cast<const VariableDeclaration*>(statement))
    {
        std::string value =
            generateExpression(declaration->value.get());

        instructions.push_back(
            declaration->name + " = " + value
        );

        return;
    }

    if (const auto* assignment =
            dynamic_cast<const Assignment*>(statement))
    {
        std::string value =
            generateExpression(assignment->value.get());

        instructions.push_back(
            assignment->name + " = " + value
        );

        return;
    }

    if (const auto* print =
            dynamic_cast<const PrintStatement*>(statement))
    {
        std::string value =
            generateExpression(print->value.get());

        instructions.push_back(
            "PRINT " + value
        );

        return;
    }
}

void IRGenerator::generate(const Program* program)
{
    instructions.clear();
    tempCount = 0;

    if (!program)
        return;

    for (const auto& statement : program->statements)
    {
        generateStatement(statement.get());
    }
}

void IRGenerator::printTAC() const
{
    std::cout
        << "\n=== THREE-ADDRESS CODE ===\n\n";

    if (instructions.empty())
    {
        std::cout
            << "No intermediate code generated.\n";
    }
    else
    {
        for (const auto& instruction : instructions)
        {
            std::cout << instruction << '\n';
        }
    }

    std::cout << std::endl;
}