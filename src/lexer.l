%option noyywrap

%{
#include <iostream>
#include <cstring>
#include "parser.tab.h"

using namespace std;
extern bool lexicalError;
%}

%%

"let"       { return LET; }
"print"     { return PRINT; }

"="         { return ASSIGN; }

";"         { return SEMICOLON; }

"+"         { return PLUS; }
"-"         { return MINUS; }
"*"         { return MULTIPLY; }
"/"         { return DIVIDE; }

"("     { return '('; }
")"     { return ')'; }

[0-9]+      {
                yylval.intValue = std::stoi(yytext);
                return NUMBER;
            }

[a-zA-Z_][a-zA-Z0-9_]* {
                yylval.stringValue = strdup(yytext);
                return IDENTIFIER;
            }

[ \t\n]+    { }

. {
      lexicalError = true;
    cerr << "Lexical Error: Unknown character '" << yytext << "'" << endl;
    return 0;
}
%%
