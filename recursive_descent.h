#ifndef RECURSIVE_DESCENT_H
#define RECURSIVE_DESCENT_H

/* Token codes (0 means end of input) */
enum { INT = 256, FLOAT, ID, NUM, PLUS, MINUS, MUL, DIV,
       ASSIGN, LPAREN, RPAREN, SEMICOLON, INVALID };

int yylex(void);
extern char *yytext;
extern int yylineno;

#endif