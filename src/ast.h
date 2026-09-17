#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

// ==========================
// Base AST Node
// ==========================

class ASTNode {
public:
    virtual ~ASTNode() = default;
};

// ==========================
// Expressions
// ==========================

class Expression : public ASTNode {
public:
    virtual ~Expression() = default;
};

// Number: 25
class NumberExpr : public Expression {
public:
    int value;

    explicit NumberExpr(int value)
        : value(value) {}
};

// Variable: x
class VariableExpr : public Expression {
public:
    std::string name;

    explicit VariableExpr(std::string name)
        : name(std::move(name)) {}
};

// Binary expression: x + y
class BinaryExpr : public Expression {
public:
    std::string op;

    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;

    BinaryExpr(
        std::unique_ptr<Expression> left,
        std::string op,
        std::unique_ptr<Expression> right
    )
        : op(std::move(op)),
          left(std::move(left)),
          right(std::move(right)) {}
};

// ==========================
// Statements
// ==========================

class Statement : public ASTNode {
public:
    virtual ~Statement() = default;
};

// let x = expression;
class VariableDeclaration : public Statement {
public:
    std::string name;
    std::unique_ptr<Expression> value;

    VariableDeclaration(
        std::string name,
        std::unique_ptr<Expression> value
    )
        : name(std::move(name)),
          value(std::move(value)) {}
};

// x = expression;
class Assignment : public Statement {
public:
    std::string name;
    std::unique_ptr<Expression> value;

    Assignment(
        std::string name,
        std::unique_ptr<Expression> value
    )
        : name(std::move(name)),
          value(std::move(value)) {}
};

// ==========================
// Program
// ==========================

class Program : public ASTNode {
public:
    std::vector<std::unique_ptr<Statement>> statements;

    void addStatement(std::unique_ptr<Statement> statement)
    {
        statements.push_back(std::move(statement));
    }
};

void printAST(const Program* program);

#endif