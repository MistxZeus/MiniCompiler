%option noyywrap nounput
%{
#include <iostream>
#include <cstdlib>
#include <cstring>
#include "parser.tab.h"
extern bool lexicalError;
%}
%%
"let"       { return LET; }
"print"     { return PRINT; }
"fn"        { return FN; }
"return"    { return RETURN; }
"if"        { return IF; }
"else"      { return ELSE; }
"while"     { return WHILE; }
"<="        { return LE; }
">="        { return GE; }
"=="        { return EQ; }
"!="        { return NE; }
"&&"        { return AND; }
"||"        { return OR; }
"="         { return ASSIGN; }
"<"         { return LT; }
">"         { return GT; }
"!"         { return NOT; }
";"         { return SEMICOLON; }
","         { return COMMA; }
"{"         { return LBRACE; }
"}"         { return RBRACE; }
"("         { return '('; }
")"         { return ')'; }
"+"         { return PLUS; }
"-"         { return MINUS; }
"*"         { return MULTIPLY; }
"/"         { return DIVIDE; }
[0-9]+      { yylval.intValue = std::atoi(yytext); return NUMBER; }
[a-zA-Z_][a-zA-Z0-9_]* { yylval.stringValue = strdup(yytext); return IDENTIFIER; }
[ \t\r\n]+ { }
. { lexicalError = true; std::cerr << "Lexical Error: Unknown character '" << yytext << "'\n"; return 0; }
%%
