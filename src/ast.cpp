#include "ast.h"
#include <iostream>

using namespace std;

void printExpression(
    const Expression* expression,
    const string& prefix,
    bool isLast
);

void printStatement(
    const Statement* statement,
    const string& prefix,
    bool isLast
);

void printExpression(
    const Expression* expression,
    const string& prefix,
    bool isLast
)
{
    cout << prefix;

    if (isLast)
        cout << "└── ";
    else
        cout << "├── ";

    if (const auto* number =
            dynamic_cast<const NumberExpr*>(expression))
    {
        cout << "Number: " << number->value << endl;
    }
    else if (const auto* variable =
                 dynamic_cast<const VariableExpr*>(expression))
    {
        cout << "Variable: " << variable->name << endl;
    }
    else if (const auto* binary =
                 dynamic_cast<const BinaryExpr*>(expression))
    {
        cout << "Binary: " << binary->op << endl;

        string childPrefix = prefix;

        if (isLast)
            childPrefix += "    ";
        else
            childPrefix += "│   ";

        printExpression(
            binary->left.get(),
            childPrefix,
            false
        );

        printExpression(
            binary->right.get(),
            childPrefix,
            true
        );
    }
}

void printStatement(
    const Statement* statement,
    const std::string& prefix,
    bool isLast)
{
    if (const auto* declaration =
            dynamic_cast<const VariableDeclaration*>(statement))
    {
        std::cout << prefix
                  << (isLast ? "└── " : "├── ")
                  << "VariableDeclaration: "
                  << declaration->name
                  << std::endl;

        std::string childPrefix = prefix;

        if (isLast)
            childPrefix += "    ";
        else
            childPrefix += "│   ";

        printExpression(
            declaration->value.get(),
            childPrefix,
            true
        );
    }
    else if (const auto* assignment =
                 dynamic_cast<const Assignment*>(statement))
    {
        std::cout << prefix
                  << (isLast ? "└── " : "├── ")
                  << "Assignment: "
                  << assignment->name
                  << std::endl;

        std::string childPrefix = prefix;

        if (isLast)
            childPrefix += "    ";
        else
            childPrefix += "│   ";

        printExpression(
            assignment->value.get(),
            childPrefix,
            true
        );
    }
    else if (const auto* print =
                 dynamic_cast<const PrintStatement*>(statement))
    {
        std::cout << prefix
                  << (isLast ? "└── " : "├── ")
                  << "PrintStatement"
                  << std::endl;

        std::string childPrefix = prefix;

        if (isLast)
            childPrefix += "    ";
        else
            childPrefix += "│   ";

        printExpression(
            print->value.get(),
            childPrefix,
            true
        );
    }
}

void printAST(const Program* program)
{
    cout << "\n=== ABSTRACT SYNTAX TREE ===\n\n";
    cout << "Program" << endl;

    for (size_t i = 0;
         i < program->statements.size();
         i++)
    {
        bool isLast =
            (i == program->statements.size() - 1);

        printStatement(
            program->statements[i].get(),
            "",
            isLast
        );
    }

    cout << endl;
}