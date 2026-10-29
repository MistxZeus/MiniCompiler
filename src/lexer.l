%option noyywrap nounput
%{
#include <iostream>
#include <cstdlib>
#include <cstring>
#include "parser.tab.h"

// Set by the lexer when an unsupported character is encountered. The parser's
// error handler checks this flag to avoid reporting the same problem twice.
extern bool lexicalError;
%}
%%
 /* Keyword rules precede the identifier rule so reserved words become tokens. */
"let"       { return LET; }
"print"     { return PRINT; }
"fn"        { return FN; }
"return"    { return RETURN; }
"if"        { return IF; }
"else"      { return ELSE; }
"while"     { return WHILE; }

 /* Multi-character operators must be matched before their single-char prefixes. */
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

 /* Numeric literals carry their integer value through Bison's semantic union. */
[0-9]+      { yylval.intValue = std::atoi(yytext); return NUMBER; }

 /* Identifiers are copied because yytext is owned by Flex's input buffer. */
[a-zA-Z_][a-zA-Z0-9_]* { yylval.stringValue = strdup(yytext); return IDENTIFIER; }

 /* Whitespace separates tokens but has no syntactic meaning in MiniLang. */
[ \t\r\n]+ { }

 /* Any unmatched character is a lexical error, not a valid token. */
. { lexicalError = true; std::cerr << "Lexical Error: Unknown character '" << yytext << "'\n"; return 0; }
%%
