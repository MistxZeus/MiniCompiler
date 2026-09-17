%option noyywrap

%{
#include <iostream>
#include <cstring>
#include "parser.tab.h"

using namespace std;
%}

%%

"let"       { return LET; }

"="         { return ASSIGN; }

";"         { return SEMICOLON; }

"+"         { return PLUS; }
"-"         { return MINUS; }
"*"         { return MULTIPLY; }
"/"         { return DIVIDE; }

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
    cout << "Unknown character: " << yytext << endl;
}

%%