%{
#include "recursive_descent.h"
%}

%option noyywrap yylineno

%%
"int"                   { return INT; }
"float"                 { return FLOAT; }
[0-9]+(\.[0-9]+)?       { return NUM; }
[a-zA-Z_][a-zA-Z0-9_]*  { return ID; }
"+"                     { return PLUS; }
"-"                     { return MINUS; }
"*"                     { return MUL; }
"/"                     { return DIV; }
"="                     { return ASSIGN; }
"("                     { return LPAREN; }
")"                     { return RPAREN; }
";"                     { return SEMICOLON; }
[ \t\r\n]+              { /* skip whitespace */ }
.                       { return INVALID; }
%%