%code requires {
#include "ast.h"
}

%{
#include <iostream>
#include <memory>
#include "ast.h"
#include "semantic_analyzer.h"

int yylex();
void yyerror(const char *s);

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

    ;

%%

void yyerror(const char *s)
{
    std::cout << "Syntax Error: " << s << std::endl;
}
int main()
{
    std::cout
        << "========================================\n"
        << "          MINILANG COMPILER\n"
        << "========================================\n"
        << std::endl;

    if (yyparse() == 0)
    {
        std::cout
            << "\n[1] Syntax Analysis       ✓"
            << std::endl;

        std::cout
            << "[2] AST Construction      ✓"
            << std::endl;

        printAST(root.get());

        SemanticAnalyzer analyzer;

        std::cout
            << "[3] Semantic Analysis     ";

        if (analyzer.analyze(root.get()))
        {
            std::cout << "✓" << std::endl;

            analyzer.printSymbolTable();

            std::cout
                << "========================================\n"
                << "       COMPILATION SUCCESSFUL\n"
                << "========================================\n";
        }
        else
        {
            std::cout << "✗" << std::endl;

            analyzer.printSymbolTable();

            std::cout
                << "========================================\n"
                << "         COMPILATION FAILED\n"
                << "========================================\n";
        }
    }
    else
    {
        std::cout
            << "\n[1] Syntax Analysis       ✗"
            << std::endl;

        std::cout
            << "========================================\n"
            << "         COMPILATION FAILED\n"
            << "========================================\n";
    }

    return 0;
}