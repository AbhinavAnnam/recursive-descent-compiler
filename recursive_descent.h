
#ifndef RECURSIVE_DESCENT_H
#define RECURSIVE_DESCENT_H

enum {
    INT = 256,
    FLOAT,
    ID,
    NUM,
    PLUS,
    MINUS,
    MUL,
    DIV,
    ASSIGN,
    LPAREN,
    RPAREN,
    SEMICOLON,
    LBRACE,
    RBRACE,
    STRING,
    OTHER,
    PLUS_ASSIGN,
    MINUS_ASSIGN
};

int yylex(void);
extern char *yytext;
extern int yylineno;

#endif
