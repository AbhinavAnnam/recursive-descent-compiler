
%{
#include "recursive_descent.h"
%}

%option noyywrap yylineno

%%
^[ \t]*#[^\n]*             { return OTHER; }

"int"                      { return INT; }
"float"                    { return FLOAT; }

"//"[^\n]*                 { /* Ignore single-line comments */ }
"/*"([^*]|\*+[^*/])*\*+"/" { /* Ignore block comments */ }

\"([^\\\n]|\\.)*\"         { return STRING; }

[0-9]+(\.[0-9]+)?          { return NUM; }
[a-zA-Z_][a-zA-Z0-9_]*     { return ID; }

"+="                       { return PLUS_ASSIGN; }
"-="                       { return MINUS_ASSIGN; }
"+"                        { return PLUS; }
"-"                        { return MINUS; }
"*"                        { return MUL; }
"/"                        { return DIV; }
"="                        { return ASSIGN; }
"("                        { return LPAREN; }
")"                        { return RPAREN; }
";"                        { return SEMICOLON; }
"{"                        { return LBRACE; }
"}"                        { return RBRACE; }

[ \t\r\n]+                 { /* Ignore whitespace */ }

.                          { return OTHER; }
%%
