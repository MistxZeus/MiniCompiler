%code requires {
#include "ast.h"
#include <string>
#include <vector>
}
%{
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ast.h"
#include "semantic_analyzer.h"
#include "ir.h"
#include "interpreter.h"
int yylex();
void yyerror(const char *s);
bool lexicalError = false;
std::unique_ptr<Program> root;
static std::vector<std::unique_ptr<Expression>> takeExpressions(std::vector<Expression*>* raw) {
    std::vector<std::unique_ptr<Expression>> out;
    if (raw) { for (auto* e : *raw) out.emplace_back(e); delete raw; }
    return out;
}
static std::vector<std::string> takeStrings(std::vector<std::string>* raw) {
    std::vector<std::string> out; if (raw) { out = std::move(*raw); delete raw; } return out;
}
static std::unique_ptr<BlockStatement> takeBlock(BlockStatement* b) { return std::unique_ptr<BlockStatement>(b); }
%}
%union {
    int intValue;
    char* stringValue;
    Expression* expression;
    Statement* statement;
    BlockStatement* block;
    std::vector<Statement*>* statementList;
    std::vector<Expression*>* expressionList;
    std::vector<std::string>* stringList;
}
%token LET PRINT FN RETURN IF ELSE WHILE
%token <stringValue> IDENTIFIER
%token <intValue> NUMBER
%token ASSIGN SEMICOLON COMMA LBRACE RBRACE
%token PLUS MINUS MULTIPLY DIVIDE
%token LT GT LE GE EQ NE AND OR NOT
%left OR
%left AND
%nonassoc EQ NE LT GT LE GE
%left PLUS MINUS
%left MULTIPLY DIVIDE
%right NOT UMINUS
%type <statementList> top_item_list statement_list
%type <statement> top_item statement
%type <block> block
%type <expression> expression
%type <expressionList> argument_list argument_list_opt
%type <stringList> parameter_list parameter_list_opt
%start program
%%
program:
    top_item_list {
        root = std::make_unique<Program>();
        for (auto* s : *$1) root->addStatement(std::unique_ptr<Statement>(s));
        delete $1;
    }
    ;
top_item_list:
    %empty { $$ = new std::vector<Statement*>(); }
  | top_item_list top_item { $1->push_back($2); $$ = $1; }
    ;
top_item:
    statement { $$ = $1; }
  | FN IDENTIFIER '(' parameter_list_opt ')' block {
        $$ = new FunctionDeclaration($2, takeStrings($4), takeBlock($6));
    }
    ;
parameter_list_opt:
    %empty { $$ = new std::vector<std::string>(); }
  | parameter_list { $$ = $1; }
    ;
parameter_list:
    IDENTIFIER { $$ = new std::vector<std::string>(); $$->push_back($1); }
  | parameter_list COMMA IDENTIFIER { $1->push_back($3); $$ = $1; }
    ;
statement_list:
    %empty { $$ = new std::vector<Statement*>(); }
  | statement_list statement { $1->push_back($2); $$ = $1; }
    ;
block:
    LBRACE statement_list RBRACE {
        $$ = new BlockStatement();
        for (auto* s : *$2) $$->addStatement(std::unique_ptr<Statement>(s));
        delete $2;
    }
    ;
statement:
    LET IDENTIFIER ASSIGN expression SEMICOLON { $$ = new VariableDeclaration($2, std::unique_ptr<Expression>($4)); }
  | IDENTIFIER ASSIGN expression SEMICOLON { $$ = new Assignment($1, std::unique_ptr<Expression>($3)); }
  | PRINT '(' expression ')' SEMICOLON { $$ = new PrintStatement(std::unique_ptr<Expression>($3)); }
  | block { $$ = $1; }
  | IF '(' expression ')' block { $$ = new IfStatement(std::unique_ptr<Expression>($3), takeBlock($5)); }
  | IF '(' expression ')' block ELSE block { $$ = new IfStatement(std::unique_ptr<Expression>($3), takeBlock($5), takeBlock($7)); }
  | WHILE '(' expression ')' block { $$ = new WhileStatement(std::unique_ptr<Expression>($3), takeBlock($5)); }
  | RETURN expression SEMICOLON { $$ = new ReturnStatement(std::unique_ptr<Expression>($2)); }
    ;
argument_list_opt:
    %empty { $$ = new std::vector<Expression*>(); }
  | argument_list { $$ = $1; }
    ;
argument_list:
    expression { $$ = new std::vector<Expression*>(); $$->push_back($1); }
  | argument_list COMMA expression { $1->push_back($3); $$ = $1; }
    ;
expression:
    NUMBER { $$ = new NumberExpr($1); }
  | IDENTIFIER { $$ = new VariableExpr($1); }
  | IDENTIFIER '(' argument_list_opt ')' { $$ = new CallExpr($1, takeExpressions($3)); }
  | '(' expression ')' { $$ = $2; }
  | expression PLUS expression { $$ = new BinaryExpr(std::unique_ptr<Expression>($1), "+", std::unique_ptr<Expression>($3)); }
  | expression MINUS expression { $$ = new BinaryExpr(std::unique_ptr<Expression>($1), "-", std::unique_ptr<Expression>($3)); }
  | expression MULTIPLY expression { $$ = new BinaryExpr(std::unique_ptr<Expression>($1), "*", std::unique_ptr<Expression>($3)); }
  | expression DIVIDE expression { $$ = new BinaryExpr(std::unique_ptr<Expression>($1), "/", std::unique_ptr<Expression>($3)); }
  | expression LT expression { $$ = new BinaryExpr(std::unique_ptr<Expression>($1), "<", std::unique_ptr<Expression>($3)); }
  | expression GT expression { $$ = new BinaryExpr(std::unique_ptr<Expression>($1), ">", std::unique_ptr<Expression>($3)); }
  | expression LE expression { $$ = new BinaryExpr(std::unique_ptr<Expression>($1), "<=", std::unique_ptr<Expression>($3)); }
  | expression GE expression { $$ = new BinaryExpr(std::unique_ptr<Expression>($1), ">=", std::unique_ptr<Expression>($3)); }
  | expression EQ expression { $$ = new BinaryExpr(std::unique_ptr<Expression>($1), "==", std::unique_ptr<Expression>($3)); }
  | expression NE expression { $$ = new BinaryExpr(std::unique_ptr<Expression>($1), "!=", std::unique_ptr<Expression>($3)); }
  | expression AND expression { $$ = new BinaryExpr(std::unique_ptr<Expression>($1), "&&", std::unique_ptr<Expression>($3)); }
  | expression OR expression { $$ = new BinaryExpr(std::unique_ptr<Expression>($1), "||", std::unique_ptr<Expression>($3)); }
  | MINUS expression %prec UMINUS { $$ = new UnaryExpr("-", std::unique_ptr<Expression>($2)); }
  | NOT expression { $$ = new UnaryExpr("!", std::unique_ptr<Expression>($2)); }
    ;
%%
void yyerror(const char *s) { if (!lexicalError) std::cout << "Syntax Error: " << s << std::endl; }
int main() {
    std::cout << "========================================\n          MINILANG COMPILER\n========================================\n\n";
    int parseResult = yyparse();
    if (lexicalError) { std::cout << "\n[1] Lexical Analysis      X\n========================================\n        COMPILATION FAILED\n========================================\n"; return 1; }
    if (parseResult != 0 || !root) { std::cout << "\n[1] Lexical Analysis      ✓\n[2] Syntax Analysis       X\n========================================\n        COMPILATION FAILED\n========================================\n"; return 1; }
    std::cout << "[1] Lexical Analysis      ✓\n[2] Syntax Analysis       ✓\n[3] AST Construction      ✓\n";
    printAST(root.get());
    SemanticAnalyzer analyzer;
    std::cout << "[4] Semantic Analysis     ";
    if (!analyzer.analyze(root.get())) { std::cout << "X\n"; analyzer.printSymbolTable(); std::cout << "COMPILATION FAILED\n"; return 1; }
    std::cout << "✓\n"; analyzer.printSymbolTable();
    IRGenerator ir; ir.generate(root.get());
    std::cout << "[5] Intermediate Code     ✓\n"; ir.printTAC();
    Interpreter interpreter;
    std::cout << "[6] Execution             ";
    if (!interpreter.execute(root.get())) { std::cout << "X\n========================================\n        EXECUTION FAILED\n========================================\n"; return 1; }
    std::cout << "✓\n"; interpreter.printEnvironment();
    std::cout << "========================================\n       COMPILATION SUCCESSFUL\n========================================\n";
    return 0;
}
