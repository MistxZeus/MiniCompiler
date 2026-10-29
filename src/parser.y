%code requires {
#include "ast.h"
#include <string>
#include <vector>
}
%{
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ast.h"
#include "semantic_analyzer.h"
#include "ir.h"
#include "interpreter.h"
#include "optimizer.h"
#include "target_codegen.h"

int yylex();
void yyerror(const char *s);

// The lexer sets this flag for unknown characters; root receives the parsed AST.
bool lexicalError = false;
std::unique_ptr<Program> root;

// Bison's union stores raw pointers. These helpers immediately transfer them
// into STL containers/unique_ptr so the constructed AST owns its children.
static std::vector<std::unique_ptr<Expression>> takeExpressions(
    std::vector<Expression*>* raw) {
    std::vector<std::unique_ptr<Expression>> out;
    if (raw) {
        for (auto* expression : *raw) out.emplace_back(expression);
        delete raw;
    }
    return out;
}

static std::vector<std::string> takeStrings(std::vector<std::string>* raw) {
    std::vector<std::string> out;
    if (raw) {
        out = std::move(*raw);
        delete raw;
    }
    return out;
}

static std::unique_ptr<BlockStatement> takeBlock(BlockStatement* block) {
    return std::unique_ptr<BlockStatement>(block);
}
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

/* From lowest to highest precedence: OR, AND, comparisons, arithmetic, unary. */
%left OR
%left AND
%nonassoc EQ NE LT GT LE GE
%left PLUS MINUS
%left MULTIPLY DIVIDE
%precedence NOT UMINUS

%type <statementList> top_item_list statement_list
%type <statement> top_item statement
%type <block> block
%type <expression> expression
%type <expressionList> argument_list argument_list_opt
%type <stringList> parameter_list parameter_list_opt
%start program
%%

/* A program is a sequence of top-level statements and function declarations. */
program:
    top_item_list {
        root = std::make_unique<Program>();
        for (auto* statement : *$1) {
            root->addStatement(std::unique_ptr<Statement>(statement));
        }
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
    IDENTIFIER {
        $$ = new std::vector<std::string>();
        $$->push_back($1);
    }
  | parameter_list COMMA IDENTIFIER { $1->push_back($3); $$ = $1; }
    ;

statement_list:
    %empty { $$ = new std::vector<Statement*>(); }
  | statement_list statement { $1->push_back($2); $$ = $1; }
    ;

block:
    LBRACE statement_list RBRACE {
        $$ = new BlockStatement();
        for (auto* statement : *$2) {
            $$->addStatement(std::unique_ptr<Statement>(statement));
        }
        delete $2;
    }
    ;

/* Each statement production constructs the corresponding AST node. */
statement:
    LET IDENTIFIER ASSIGN expression SEMICOLON {
        $$ = new VariableDeclaration($2, std::unique_ptr<Expression>($4));
    }
  | IDENTIFIER ASSIGN expression SEMICOLON {
        $$ = new Assignment($1, std::unique_ptr<Expression>($3));
    }
  | PRINT '(' expression ')' SEMICOLON {
        $$ = new PrintStatement(std::unique_ptr<Expression>($3));
    }
  | block { $$ = $1; }
  | IF '(' expression ')' block {
        $$ = new IfStatement(std::unique_ptr<Expression>($3), takeBlock($5));
    }
  | IF '(' expression ')' block ELSE block {
        $$ = new IfStatement(std::unique_ptr<Expression>($3), takeBlock($5), takeBlock($7));
    }
  | WHILE '(' expression ')' block {
        $$ = new WhileStatement(std::unique_ptr<Expression>($3), takeBlock($5));
    }
  | RETURN expression SEMICOLON {
        $$ = new ReturnStatement(std::unique_ptr<Expression>($2));
    }
    ;

argument_list_opt:
    %empty { $$ = new std::vector<Expression*>(); }
  | argument_list { $$ = $1; }
    ;

argument_list:
    expression {
        $$ = new std::vector<Expression*>();
        $$->push_back($1);
    }
  | argument_list COMMA expression { $1->push_back($3); $$ = $1; }
    ;

/* Expression rules use the precedence declarations above to resolve ambiguity. */
expression:
    NUMBER { $$ = new NumberExpr($1); }
  | IDENTIFIER { $$ = new VariableExpr($1); }
  | IDENTIFIER '(' argument_list_opt ')' {
        $$ = new CallExpr($1, takeExpressions($3));
    }
  | '(' expression ')' { $$ = $2; }
  | expression PLUS expression {
        $$ = new BinaryExpr(std::unique_ptr<Expression>($1), "+", std::unique_ptr<Expression>($3));
    }
  | expression MINUS expression {
        $$ = new BinaryExpr(std::unique_ptr<Expression>($1), "-", std::unique_ptr<Expression>($3));
    }
  | expression MULTIPLY expression {
        $$ = new BinaryExpr(std::unique_ptr<Expression>($1), "*", std::unique_ptr<Expression>($3));
    }
  | expression DIVIDE expression {
        $$ = new BinaryExpr(std::unique_ptr<Expression>($1), "/", std::unique_ptr<Expression>($3));
    }
  | expression LT expression {
        $$ = new BinaryExpr(std::unique_ptr<Expression>($1), "<", std::unique_ptr<Expression>($3));
    }
  | expression GT expression {
        $$ = new BinaryExpr(std::unique_ptr<Expression>($1), ">", std::unique_ptr<Expression>($3));
    }
  | expression LE expression {
        $$ = new BinaryExpr(std::unique_ptr<Expression>($1), "<=", std::unique_ptr<Expression>($3));
    }
  | expression GE expression {
        $$ = new BinaryExpr(std::unique_ptr<Expression>($1), ">=", std::unique_ptr<Expression>($3));
    }
  | expression EQ expression {
        $$ = new BinaryExpr(std::unique_ptr<Expression>($1), "==", std::unique_ptr<Expression>($3));
    }
  | expression NE expression {
        $$ = new BinaryExpr(std::unique_ptr<Expression>($1), "!=", std::unique_ptr<Expression>($3));
    }
  | expression AND expression {
        $$ = new BinaryExpr(std::unique_ptr<Expression>($1), "&&", std::unique_ptr<Expression>($3));
    }
  | expression OR expression {
        $$ = new BinaryExpr(std::unique_ptr<Expression>($1), "||", std::unique_ptr<Expression>($3));
    }
  | MINUS expression %prec UMINUS {
        $$ = new UnaryExpr("-", std::unique_ptr<Expression>($2));
    }
  | NOT expression {
        $$ = new UnaryExpr("!", std::unique_ptr<Expression>($2));
    }
    ;
%%

// Suppress a duplicate syntax diagnostic when the lexer already found an error.
void yyerror(const char *s) {
    if (!lexicalError) std::cout << "Syntax Error: " << s << std::endl;
}

int main(int argc, char** argv) {
    // Target code generation is part of the normal pipeline. By default the
    // generated assembly is printed in the terminal alongside the TAC. The
    // optional flag also saves a copy for assembling into a native executable.
    std::string assemblyOutputPath;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--emit-asm" && i + 1 < argc) {
            assemblyOutputPath = argv[++i];
        } else if (argument == "--help") {
            std::cout << "Usage: ./src/compiler [--emit-asm output.s] < program.min\n"
                         "Target assembly is displayed in the terminal during every successful run.\n"
                         "Use --emit-asm output.s to also save the assembly to a file.\n"
                         "The generated GNU x86-64 assembly can be assembled with GCC.\n";
            return 0;
        } else {
            std::cerr << "Unknown or incomplete option: " << argument << "\n"
                      << "Usage: ./src/compiler [--emit-asm output.s] < program.min\n";
            return 2;
        }
    }

    std::cout << "========================================\n          MINILANG COMPILER\n========================================\n\n";
    const int parseResult = yyparse();

    if (lexicalError) {
        std::cout << "\n[1] Lexical Analysis      X\n========================================\n        COMPILATION FAILED\n========================================\n";
        return 1;
    }
    if (parseResult != 0 || !root) {
        std::cout << "\n[1] Lexical Analysis      ✓\n[2] Syntax Analysis       X\n========================================\n        COMPILATION FAILED\n========================================\n";
        return 1;
    }

    std::cout << "[1] Lexical Analysis      ✓\n[2] Syntax Analysis       ✓\n[3] AST Construction      ✓\n";
    printAST(root.get());

    SemanticAnalyzer analyzer;
    std::cout << "[4] Semantic Analysis     ";
    if (!analyzer.analyze(root.get())) {
        std::cout << "X\n";
        analyzer.printSymbolTable();
        std::cout << "COMPILATION FAILED\n";
        return 1;
    }
    std::cout << "✓\n";
    analyzer.printSymbolTable();

    // Validate before optimization so an unreachable branch cannot hide a
    // semantic error such as an undeclared variable or unknown function.
    Optimizer optimizer;
    optimizer.optimize(root.get());
    std::cout << "[5] Optimization          ✓\n";
    optimizer.printSummary();
    printAST(root.get());

    IRGenerator ir;
    ir.generate(root.get());
    std::cout << "[6] Intermediate Code     ✓\n";
    ir.printTAC();

    // Generate native target code during every successful compiler run.
    // The interpreter remains enabled so the existing demonstration output
    // and regression behavior are preserved alongside the new backend.
    TargetCodeGenerator targetGenerator;
    std::string assembly;
    std::string generationError;
    if (!targetGenerator.generate(root.get(), assembly, generationError)) {
        std::cerr << "Target code generation failed: " << generationError << "\n";
        std::cout << "COMPILATION FAILED\n";
        return 1;
    }

    std::cout << "[7] Target Code Generation ✓\n"
              << "\n=== TARGET CODE (GNU x86-64 Assembly) ===\n\n"
              << assembly
              << "=== END TARGET CODE ===\n";

    if (!assemblyOutputPath.empty()) {
        std::ofstream assemblyFile(assemblyOutputPath);
        if (!assemblyFile) {
            std::cerr << "Could not write assembly file: " << assemblyOutputPath << "\n";
            return 1;
        }
        assemblyFile << assembly;
        assemblyFile.close();
        if (!assemblyFile) {
            std::cerr << "Failed while writing assembly file: " << assemblyOutputPath << "\n";
            return 1;
        }
        std::cout << "Assembly also saved to: " << assemblyOutputPath << "\n"
                  << "Build native executable with: gcc " << assemblyOutputPath
                  << " -o minilang_program\n";
    }

    Interpreter interpreter;
    std::cout << "[8] AST Interpreter       ";
    if (!interpreter.execute(root.get())) {
        std::cout << "X\n========================================\n        EXECUTION FAILED\n========================================\n";
        return 1;
    }
    std::cout << "✓\n";
    interpreter.printEnvironment();
    std::cout << "========================================\n       COMPILATION SUCCESSFUL\n========================================\n";
    return 0;
}
