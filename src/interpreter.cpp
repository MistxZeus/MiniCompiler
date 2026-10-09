#include "interpreter.h"

#include <iostream>

// ==========================
// Expression Evaluation
// ==========================

int Interpreter::evaluateExpression(
    const Expression* expression,
    bool& success)
{
    if (const auto* number =
            dynamic_cast<const NumberExpr*>(expression))
    {
        return number->value;
    }

    if (const auto* variable =
            dynamic_cast<const VariableExpr*>(expression))
    {
        auto it = environment.find(variable->name);

        if (it == environment.end())
        {
            std::cerr
                << "Runtime Error: Variable '"
                << variable->name
                << "' has no runtime value."
                << std::endl;

            success = false;
            return 0;
        }

        return it->second;
    }

    if (const auto* binary =
            dynamic_cast<const BinaryExpr*>(expression))
    {
        int left =
            evaluateExpression(binary->left.get(), success);

        if (!success)
            return 0;

        int right =
            evaluateExpression(binary->right.get(), success);

        if (!success)
            return 0;

        if (binary->op == "+")
            return left + right;

        if (binary->op == "-")
            return left - right;

        if (binary->op == "*")
            return left * right;

        if (binary->op == "/")
        {
            if (right == 0)
            {
                std::cerr
                    << "Runtime Error: Division by zero."
                    << std::endl;

                success = false;
                return 0;
            }

            return left / right;
        }

        std::cerr
            << "Runtime Error: Unknown operator '"
            << binary->op
            << "'."
            << std::endl;

        success = false;
        return 0;
    }

    std::cerr
        << "Runtime Error: Unknown expression."
        << std::endl;

    success = false;
    return 0;
}

// ==========================
// Statement Execution
// ==========================

bool Interpreter::executeStatement(
    const Statement* statement)
{
    if (const auto* declaration =
            dynamic_cast<const VariableDeclaration*>(statement))
    {
        bool success = true;

        int value =
            evaluateExpression(
                declaration->value.get(),
                success
            );

        if (!success)
            return false;

        environment[declaration->name] = value;

        return true;
    }

    if (const auto* assignment =
            dynamic_cast<const Assignment*>(statement))
    {
        bool success = true;

        int value =
            evaluateExpression(
                assignment->value.get(),
                success
            );

        if (!success)
            return false;

        environment[assignment->name] = value;

        return true;
    }

    if (const auto* print =
            dynamic_cast<const PrintStatement*>(statement))
    {
        bool success = true;

        int value =
            evaluateExpression(
                print->value.get(),
                success
            );

        if (!success)
            return false;

        std::cout
            << "Program Output: "
            << value
            << std::endl;

        return true;
    }

    std::cerr
        << "Runtime Error: Unknown statement."
        << std::endl;

    return false;
}

// ==========================
// Program Execution
// ==========================

bool Interpreter::execute(const Program* program)
{
    environment.clear();

    if (!program)
        return false;

    for (const auto& statement : program->statements)
    {
        if (!executeStatement(statement.get()))
            return false;
    }

    return true;
}

// ==========================
// Runtime Environment
// ==========================

void Interpreter::printEnvironment() const
{
    std::cout
        << "\n=== RUNTIME ENVIRONMENT ===\n\n";

    if (environment.empty())
    {
        std::cout
            << "No variables were created.\n";
    }
    else
    {
        for (const auto& entry : environment)
        {
            std::cout
                << entry.first
                << " = "
                << entry.second
                << '\n';
        }
    }

    std::cout << std::endl;
}