#include "semantic_analyzer.h"
#include <iostream>

using namespace std;

SemanticAnalyzer::SemanticAnalyzer()
    : hasErrors(false)
{
}

bool SemanticAnalyzer::analyze(
    const Program* program)
{
    hasErrors = false;

    for (const auto& statement : program->statements)
    {
        analyzeStatement(statement.get());
    }

    return !hasErrors;
}

void SemanticAnalyzer::analyzeStatement(
    const Statement* statement)
{
    // Variable declaration
    if (const auto* declaration =
            dynamic_cast<const VariableDeclaration*>(statement))
    {
        if (!symbolTable.declare(declaration->name))
        {
            cout << "Semantic Error: Variable '"
                 << declaration->name
                 << "' is already declared."
                 << endl;

            hasErrors = true;
        }

        analyzeExpression(declaration->value.get());
    }

    // Assignment
    else if (const auto* assignment =
                 dynamic_cast<const Assignment*>(statement))
    {
        if (!symbolTable.exists(assignment->name))
        {
            cout << "Semantic Error: Variable '"
                 << assignment->name
                 << "' is not declared."
                 << endl;

            hasErrors = true;
        }

        analyzeExpression(assignment->value.get());
    }
}

void SemanticAnalyzer::analyzeExpression(
    const Expression* expression)
{
    // Variable usage
    if (const auto* variable =
            dynamic_cast<const VariableExpr*>(expression))
    {
        if (!symbolTable.exists(variable->name))
        {
            cout << "Semantic Error: Variable '"
                 << variable->name
                 << "' is not declared."
                 << endl;

            hasErrors = true;
        }
    }

    // Binary expression
    else if (const auto* binary =
                 dynamic_cast<const BinaryExpr*>(expression))
    {
        analyzeExpression(binary->left.get());
        analyzeExpression(binary->right.get());
    }

    // NumberExpr requires no semantic validation
}

void SemanticAnalyzer::printSymbolTable() const
{
    symbolTable.print();
}