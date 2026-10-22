#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

class ASTNode { public: virtual ~ASTNode() = default; };
class Expression : public ASTNode { public: virtual ~Expression() = default; };

class NumberExpr : public Expression {
public: int value; explicit NumberExpr(int v) : value(v) {}
};
class VariableExpr : public Expression {
public: std::string name; explicit VariableExpr(std::string n) : name(std::move(n)) {}
};
class BinaryExpr : public Expression {
public:
    std::string op; std::unique_ptr<Expression> left, right;
    BinaryExpr(std::unique_ptr<Expression> l, std::string o, std::unique_ptr<Expression> r)
        : op(std::move(o)), left(std::move(l)), right(std::move(r)) {}
};
class UnaryExpr : public Expression {
public:
    std::string op; std::unique_ptr<Expression> operand;
    UnaryExpr(std::string o, std::unique_ptr<Expression> e) : op(std::move(o)), operand(std::move(e)) {}
};
class CallExpr : public Expression {
public:
    std::string name; std::vector<std::unique_ptr<Expression>> arguments;
    CallExpr(std::string n, std::vector<std::unique_ptr<Expression>> a)
        : name(std::move(n)), arguments(std::move(a)) {}
};

class Statement : public ASTNode { public: virtual ~Statement() = default; };
class VariableDeclaration : public Statement {
public:
    std::string name; std::unique_ptr<Expression> value;
    VariableDeclaration(std::string n, std::unique_ptr<Expression> v) : name(std::move(n)), value(std::move(v)) {}
};
class Assignment : public Statement {
public:
    std::string name; std::unique_ptr<Expression> value;
    Assignment(std::string n, std::unique_ptr<Expression> v) : name(std::move(n)), value(std::move(v)) {}
};
class PrintStatement : public Statement {
public:
    std::unique_ptr<Expression> value;
    explicit PrintStatement(std::unique_ptr<Expression> v) : value(std::move(v)) {}
};
class BlockStatement : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> statements;
    void addStatement(std::unique_ptr<Statement> s) { statements.push_back(std::move(s)); }
};
class IfStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<BlockStatement> thenBlock, elseBlock;
    IfStatement(std::unique_ptr<Expression> c, std::unique_ptr<BlockStatement> t, std::unique_ptr<BlockStatement> e = nullptr)
        : condition(std::move(c)), thenBlock(std::move(t)), elseBlock(std::move(e)) {}
};
class WhileStatement : public Statement {
public:
    std::unique_ptr<Expression> condition; std::unique_ptr<BlockStatement> body;
    WhileStatement(std::unique_ptr<Expression> c, std::unique_ptr<BlockStatement> b) : condition(std::move(c)), body(std::move(b)) {}
};
class ReturnStatement : public Statement {
public:
    std::unique_ptr<Expression> value;
    explicit ReturnStatement(std::unique_ptr<Expression> v) : value(std::move(v)) {}
};
class FunctionDeclaration : public Statement {
public:
    std::string name; std::vector<std::string> parameters; std::unique_ptr<BlockStatement> body;
    FunctionDeclaration(std::string n, std::vector<std::string> p, std::unique_ptr<BlockStatement> b)
        : name(std::move(n)), parameters(std::move(p)), body(std::move(b)) {}
};

class Program : public ASTNode {
public:
    std::vector<std::unique_ptr<Statement>> statements;
    void addStatement(std::unique_ptr<Statement> s) { statements.push_back(std::move(s)); }
};

void printAST(const Program* program);
#endif
