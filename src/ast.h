#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

// Base types let compiler passes process expressions and statements uniformly.
// The virtual destructors make deletion through base-class pointers safe.
class ASTNode {
public:
    virtual ~ASTNode() = default;
};

class Expression : public ASTNode {
public:
    virtual ~Expression() = default;
};

// Leaf expression: an integer literal such as 42.
class NumberExpr : public Expression {
public:
    int value;
    explicit NumberExpr(int v) : value(v) {}
};

// Leaf expression: a read of a variable by name.
class VariableExpr : public Expression {
public:
    std::string name;
    explicit VariableExpr(std::string n) : name(std::move(n)) {}
};

// An operator with two operands, e.g. left + right or left < right.
class BinaryExpr : public Expression {
public:
    std::string op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;

    BinaryExpr(std::unique_ptr<Expression> l, std::string o,
               std::unique_ptr<Expression> r)
        : op(std::move(o)), left(std::move(l)), right(std::move(r)) {}
};

// A prefix operator with one operand, currently unary minus and logical NOT.
class UnaryExpr : public Expression {
public:
    std::string op;
    std::unique_ptr<Expression> operand;

    UnaryExpr(std::string o, std::unique_ptr<Expression> e)
        : op(std::move(o)), operand(std::move(e)) {}
};

// A function call is an expression because it produces an integer result.
class CallExpr : public Expression {
public:
    std::string name;
    std::vector<std::unique_ptr<Expression>> arguments;

    CallExpr(std::string n, std::vector<std::unique_ptr<Expression>> a)
        : name(std::move(n)), arguments(std::move(a)) {}
};

class Statement : public ASTNode {
public:
    virtual ~Statement() = default;
};

// `let name = value;` introduces a name in the current scope.
class VariableDeclaration : public Statement {
public:
    std::string name;
    std::unique_ptr<Expression> value;

    VariableDeclaration(std::string n, std::unique_ptr<Expression> v)
        : name(std::move(n)), value(std::move(v)) {}
};

// `name = value;` updates an existing variable.
class Assignment : public Statement {
public:
    std::string name;
    std::unique_ptr<Expression> value;

    Assignment(std::string n, std::unique_ptr<Expression> v)
        : name(std::move(n)), value(std::move(v)) {}
};

// `print(value);` evaluates an expression and writes its value to standard output.
class PrintStatement : public Statement {
public:
    std::unique_ptr<Expression> value;
    explicit PrintStatement(std::unique_ptr<Expression> v) : value(std::move(v)) {}
};

// A block groups statements and creates a lexical scope in analysis/execution.
class BlockStatement : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> statements;

    void addStatement(std::unique_ptr<Statement> statement) {
        statements.push_back(std::move(statement));
    }
};

class IfStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<BlockStatement> thenBlock;
    std::unique_ptr<BlockStatement> elseBlock; // Null when there is no `else`.

    IfStatement(std::unique_ptr<Expression> c, std::unique_ptr<BlockStatement> t,
                std::unique_ptr<BlockStatement> e = nullptr)
        : condition(std::move(c)), thenBlock(std::move(t)), elseBlock(std::move(e)) {}
};

class WhileStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<BlockStatement> body;

    WhileStatement(std::unique_ptr<Expression> c, std::unique_ptr<BlockStatement> b)
        : condition(std::move(c)), body(std::move(b)) {}
};

class ReturnStatement : public Statement {
public:
    std::unique_ptr<Expression> value;
    explicit ReturnStatement(std::unique_ptr<Expression> v) : value(std::move(v)) {}
};

// Function declarations are stored at program level and refer to a statement block.
class FunctionDeclaration : public Statement {
public:
    std::string name;
    std::vector<std::string> parameters;
    std::unique_ptr<BlockStatement> body;

    FunctionDeclaration(std::string n, std::vector<std::string> p,
                        std::unique_ptr<BlockStatement> b)
        : name(std::move(n)), parameters(std::move(p)), body(std::move(b)) {}
};

// The AST owns every top-level statement. unique_ptr expresses ownership and
// ensures child nodes are released automatically when a tree is replaced.
class Program : public ASTNode {
public:
    std::vector<std::unique_ptr<Statement>> statements;

    void addStatement(std::unique_ptr<Statement> statement) {
        statements.push_back(std::move(statement));
    }
};

// Print a human-readable, indented representation of the AST for debugging.
void printAST(const Program* program);

#endif // AST_H
