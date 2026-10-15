/*
   Recursive Descent Parser (Flex lexer + C parser)

   Program       -> StatementList
   StatementList -> Statement StatementList | epsilon
   Statement     -> Declaration | Assignment
   Declaration   -> type id [ = Expression ] ;
   Assignment    -> id = Expression ;
   Expression    -> Term { (+|-) Term }      (Expression -> Term Expression')
   Term          -> Factor { (*|/) Factor }  (Term -> Factor Term')
   Factor        -> ( Expression ) | id | num
   type          -> int | float
   num           -> digits | digits . digits
*/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "recursive_descent.h"

#define MAX 100

/* ---------- Symbol table and error list ---------- */

char symName[MAX][50], symType[MAX][10];
int  symCount = 0;

char errMsg[MAX][300];
int  errLine[MAX], errCount = 0;

static int tok;            /* current token */
static int prevLine = 1;   /* line of the last consumed token */
static int panic = 0;      /* 1 = error already reported in this statement */

static const char *tokName[] = { "'int'", "'float'", "identifier", "number",
    "'+'", "'-'", "'*'", "'/'", "'='", "'('", "')'", "';'", "invalid token" };

static void report(int line, const char *fmt, ...) {
    va_list ap;
    if (errCount >= MAX) return;
    va_start(ap, fmt);
    vsnprintf(errMsg[errCount], sizeof errMsg[0], fmt, ap);
    va_end(ap);
    errLine[errCount++] = line;
}

static void next(void) {
    prevLine = yylineno;
    tok = yylex();
    if (tok == INVALID)                    /* report every bad character once */
        report(yylineno, "Lexical Error: unexpected character '%s'.", yytext);
}

/* ---------- Semantic checks ---------- */

static int find(const char *name) {
    for (int i = 0; i < symCount; i++)
        if (strcmp(symName[i], name) == 0) return i;
    return -1;
}

static void declare(const char *name, const char *type, int line) {
    if (find(name) >= 0)
        report(line, "Semantic Error: variable '%s' is already declared.", name);
    else if (symCount >= MAX)
        report(line, "Semantic Error: symbol table is full.");
    else {
        snprintf(symName[symCount], sizeof symName[0], "%s", name);
        snprintf(symType[symCount], sizeof symType[0], "%s", type);
        symCount++;
    }
}

static void checkDeclared(const char *name, int line) {
    if (find(name) < 0)
        report(line, "Semantic Error: variable '%s' is not declared.", name);
}

/* ---------- Syntax errors and recovery ---------- */

/* Only the first error in each statement is reported */
static void fail(const char *expected, int line) {
    if (panic) return;
    panic = 1;
    if (tok == INVALID) return;            /* already reported by next() */
    if (tok == 0)
        report(line, "Syntax Error: expected %s, found end of input.", expected);
    else
        report(line, "Syntax Error: expected %s, found '%s'.", expected, yytext);
}

/* Skip to the end of the bad statement */
static void recover(void) {
    while (tok != 0 && tok != SEMICOLON && tok != INT && tok != FLOAT && tok != ID)
        next();
    if (tok == SEMICOLON) next();
}

static int match(int expected) {
    if (tok == expected) { next(); return 1; }
    /* a missing ';' belongs to the previous line */
    fail(tokName[expected - INT], expected == SEMICOLON ? prevLine : yylineno);
    return 0;
}

/* ---------- Grammar rules ---------- */

static void expression(void);

static void factor(void) {                 /* ( Expression ) | id | num */
    if (tok == ID)       { checkDeclared(yytext, yylineno); next(); }
    else if (tok == NUM) next();
    else if (tok == LPAREN) { next(); expression(); match(RPAREN); }
    else fail("identifier, number, or '('", yylineno);
}

static void term(void) {                   /* Factor { (*|/) Factor } */
    factor();
    while (tok == MUL || tok == DIV) { next(); factor(); }
}

static void expression(void) {             /* Term { (+|-) Term } */
    term();
    while (tok == PLUS || tok == MINUS) { next(); term(); }
}

static void statement(void) {
    char name[50];
    int line;
    panic = 0;

    if (tok == INT || tok == FLOAT) {      /* Declaration -> type id [ = Expression ] ; */
        const char *type = (tok == INT) ? "int" : "float";
        next();
        if (tok == ID) {
            snprintf(name, sizeof name, "%s", yytext);
            line = yylineno;
            next();
            declare(name, type, line);
            if (tok == ASSIGN) { next(); expression(); }   /* optional = Expression */
            match(SEMICOLON);
        } else
            fail("identifier", yylineno);
    }
    else if (tok == ID) {                  /* Assignment -> id = Expression ; */
        checkDeclared(yytext, yylineno);
        next();
        if (match(ASSIGN)) { expression(); match(SEMICOLON); }
    }
    else
        fail("declaration or assignment", yylineno);

    if (panic) recover();
}

/* ---------- Main ---------- */

int main(void) {
    printf("Enter the program (press Ctrl+D when finished):\n\n");

    next();
    while (tok != 0)                       /* StatementList */
        statement();

    printf("\n========== COMPILATION REPORT ==========\n");
    if (errCount == 0)
        printf("Compilation successful!\n");
    else {
        printf("Compilation failed with %d error(s):\n", errCount);
        for (int i = 0; i < errCount; i++)
            printf("Line %d: %s\n", errLine[i], errMsg[i]);
    }

    printf("\n============ SYMBOL TABLE ==============\n");
    printf("%-20s %s\n", "Identifier", "Type");
    for (int i = 0; i < symCount; i++)
        printf("%-20s %s\n", symName[i], symType[i]);
    if (symCount == 0) printf("No symbols declared.\n");
    printf("========================================\n");
    return 0;
}
