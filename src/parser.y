%code requires {
#include "ast.h"
}

%{
#include <iostream>
#include <memory>

#include "ast.h"
#include "semantic_analyzer.h"
#include "ir.h"
#include "interpreter.h"

int yylex();
void yyerror(const char *s);

bool lexicalError = false;

std::unique_ptr<Program> root;
%}

%union {
    int intValue;
    char* stringValue;
    Expression* expression;
    Statement* statement;
    std::vector<Statement*>* statementList;
}

%token LET
%token PRINT

%token <stringValue> IDENTIFIER
%token <intValue> NUMBER

%token ASSIGN
%token SEMICOLON

%token PLUS
%token MINUS
%token MULTIPLY
%token DIVIDE

%left PLUS MINUS
%left MULTIPLY DIVIDE

%type <expression> expression
%type <statement> statement
%type <statementList> statement_list

%%

program:
    statement_list
    {
        root = std::make_unique<Program>();

        for (auto statement : *$1)
        {
            root->addStatement(
                std::unique_ptr<Statement>(statement)
            );
        }

        delete $1;
    }
    ;

statement_list:
    statement
    {
        $$ = new std::vector<Statement*>();
        $$->push_back($1);
    }

    | statement_list statement
    {
        $1->push_back($2);
        $$ = $1;
    }
    ;

statement:
    LET IDENTIFIER ASSIGN expression SEMICOLON
    {
        $$ = new VariableDeclaration(
            $2,
            std::unique_ptr<Expression>($4)
        );
    }

    | IDENTIFIER ASSIGN expression SEMICOLON
    {
        $$ = new Assignment(
            $1,
            std::unique_ptr<Expression>($3)
        );
    }

    | PRINT '(' expression ')' SEMICOLON
    {
        $$ = new PrintStatement(
            std::unique_ptr<Expression>($3)
        );
    }
    ;

expression:
    NUMBER
    {
        $$ = new NumberExpr($1);
    }

    | IDENTIFIER
    {
        $$ = new VariableExpr($1);
    }

    | expression PLUS expression
    {
        $$ = new BinaryExpr(
            std::unique_ptr<Expression>($1),
            "+",
            std::unique_ptr<Expression>($3)
        );
    }

    | expression MINUS expression
    {
        $$ = new BinaryExpr(
            std::unique_ptr<Expression>($1),
            "-",
            std::unique_ptr<Expression>($3)
        );
    }

    | expression MULTIPLY expression
    {
        $$ = new BinaryExpr(
            std::unique_ptr<Expression>($1),
            "*",
            std::unique_ptr<Expression>($3)
        );
    }

    | expression DIVIDE expression
    {
        $$ = new BinaryExpr(
            std::unique_ptr<Expression>($1),
            "/",
            std::unique_ptr<Expression>($3)
        );
    }

    | '(' expression ')'
    {
        $$ = $2;
    }
    ;

%%

void yyerror(const char *s)
{
    if (!lexicalError)
        std::cout << "Syntax Error: " << s << std::endl;
}

int main()
{
    std::cout
        << "========================================\n"
        << "          MINILANG COMPILER\n"
        << "========================================\n"
        << std::endl;

    int parseResult = yyparse();

    // --------------------------
    // Lexical Error
    // --------------------------

    if (lexicalError)
    {
        std::cout
            << "\n[1] Lexical Analysis      X\n"
            << "========================================\n"
            << "        COMPILATION FAILED\n"
            << "========================================\n";

        return 0;
    }

    // --------------------------
    // Syntax Error
    // --------------------------

    if (parseResult != 0)
    {
        std::cout
            << "\n[1] Lexical Analysis      ✓\n"
            << "[2] Syntax Analysis       X\n"
            << "========================================\n"
            << "        COMPILATION FAILED\n"
            << "========================================\n";

        return 0;
    }

    // --------------------------
    // Syntax + AST
    // --------------------------

    std::cout
        << "\n[1] Lexical Analysis      ✓"
        << std::endl;

    std::cout
        << "[2] Syntax Analysis       ✓"
        << std::endl;

    std::cout
        << "[3] AST Construction      ✓"
        << std::endl;

    printAST(root.get());

    // --------------------------
    // Semantic Analysis
    // --------------------------

    SemanticAnalyzer analyzer;

    std::cout
        << "[4] Semantic Analysis     ";

    if (!analyzer.analyze(root.get()))
    {
        std::cout << "X" << std::endl;

        analyzer.printSymbolTable();

        std::cout
            << "========================================\n"
            << "        COMPILATION FAILED\n"
            << "========================================\n";

        return 0;
    }

    std::cout << "✓" << std::endl;

    analyzer.printSymbolTable();

    // --------------------------
    // Intermediate Code
    // --------------------------

    IRGenerator irGenerator;

    irGenerator.generate(root.get());

    std::cout
        << "[5] Intermediate Code     ✓"
        << std::endl;

    irGenerator.printTAC();

    // --------------------------
    // Execution
    // --------------------------

    Interpreter interpreter;

    std::cout
        << "[6] Execution             ";

    if (!interpreter.execute(root.get()))
    {
        std::cout << "X" << std::endl;

        std::cout
            << "========================================\n"
            << "        EXECUTION FAILED\n"
            << "========================================\n";

        return 0;
    }

    std::cout << "✓" << std::endl;

    interpreter.printEnvironment();

    std::cout
        << "========================================\n"
        << "       COMPILATION SUCCESSFUL\n"
        << "========================================\n";

    return 0;
}