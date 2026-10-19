
/*
   Recursive Descent Compiler with Three-Address Code Generation

   Program       -> StatementList
   StatementList -> Statement StatementList | epsilon
   Statement     -> Declaration | Assignment
   Declaration   -> type id [ = Expression ] ;
   Assignment    -> id = Expression ;
   Expression    -> Term { (+|-) Term }
   Term          -> Factor { (*|/) Factor }
   Factor        -> ( Expression ) | id | num
   type          -> int | float
   num           -> digits | digits . digits
*/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "recursive_descent.h"

#define MAX 100
#define MAX_TAC 500
#define MAX_OPERAND 100

/* ---------- Three-Address Code storage ---------- */

static char tac[MAX_TAC][300];
static int tacCount = 0;
static int tempCount = 0;

static void newTemp(char *dest, size_t size)
{
    snprintf(dest, size, "t%d", ++tempCount);
}

static void emit(const char *fmt, ...)
{
    if (tacCount >= MAX_TAC)
        return;

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(tac[tacCount], sizeof tac[0], fmt, ap);
    va_end(ap);

    tacCount++;
}

/* ---------- Symbol table and error list ---------- */

char symName[MAX][50], symType[MAX][10];
int symCount = 0;

char errMsg[MAX][300];
int errLine[MAX], errCount = 0;

static int tok;
static int prevLine = 1;
static int panic = 0;

static const char *tokName[] = {
    "'int'", "'float'", "identifier", "number",
    "'+'", "'-'", "'*'", "'/'", "'='", "'('",
    "')'", "';'", "invalid token"
};

/* ---------- Error reporting ---------- */

static void report(int line, const char *fmt, ...)
{
    va_list ap;

    if (errCount >= MAX)
        return;

    va_start(ap, fmt);
    vsnprintf(errMsg[errCount], sizeof errMsg[0], fmt, ap);
    va_end(ap);

    errLine[errCount++] = line;
}

static void next(void)
{
    prevLine = yylineno;
    tok = yylex();

    if (tok == INVALID)
        report(yylineno,
               "Lexical Error: unexpected character '%s'.", yytext);
}

/* ---------- Semantic checks ---------- */

static int find(const char *name)
{
    for (int i = 0; i < symCount; i++)
        if (strcmp(symName[i], name) == 0)
            return i;

    return -1;
}

static void declare(const char *name, const char *type, int line)
{
    if (find(name) >= 0)
        report(line,
               "Semantic Error: variable '%s' is already declared.",
               name);
    else if (symCount >= MAX)
        report(line, "Semantic Error: symbol table is full.");
    else {
        snprintf(symName[symCount], sizeof symName[0], "%s", name);
        snprintf(symType[symCount], sizeof symType[0], "%s", type);
        symCount++;
    }
}

static void checkDeclared(const char *name, int line)
{
    if (find(name) < 0)
        report(line,
               "Semantic Error: variable '%s' is not declared.", name);
}

/* ---------- Syntax errors and recovery ---------- */

static void fail(const char *expected, int line)
{
    if (panic)
        return;

    panic = 1;

    if (tok == INVALID)
        return;

    if (tok == 0)
        report(line,
               "Syntax Error: expected %s, found end of input.",
               expected);
    else
        report(line,
               "Syntax Error: expected %s, found '%s'.",
               expected, yytext);
}

static void recover(void)
{
    while (tok != 0 && tok != SEMICOLON &&
           tok != INT && tok != FLOAT && tok != ID)
        next();

    if (tok == SEMICOLON)
        next();
}

static int match(int expected)
{
    if (tok == expected) {
        next();
        return 1;
    }

    /* A missing semicolon belongs to the preceding line. */
    fail(tokName[expected - INT],
         expected == SEMICOLON ? prevLine : yylineno);

    return 0;
}

/* ---------- Grammar rules and TAC generation ---------- */

/*
   The functions return 1 on successful parsing and 0 on failure.
   On success, result contains the identifier, number, or temporary
   holding the value of the parsed expression.
*/

static int expression(char *result, size_t size);

static int factor(char *result, size_t size)
{
    if (tok == ID) {
        snprintf(result, size, "%s", yytext);
        checkDeclared(result, yylineno);
        next();
        return !panic;
    }

    if (tok == NUM) {
        snprintf(result, size, "%s", yytext);
        next();
        return !panic;
    }

    if (tok == LPAREN) {
        next();

        if (!expression(result, size))
            return 0;

        if (!match(RPAREN))
            return 0;

        return !panic;
    }

    fail("identifier, number, or '('", yylineno);
    return 0;
}

static int term(char *result, size_t size)
{
    char left[MAX_OPERAND];
    char right[MAX_OPERAND];
    char temp[MAX_OPERAND];

    if (!factor(left, sizeof left))
        return 0;

    while (tok == MUL || tok == DIV) {
        int op = tok;
        next();

        if (!factor(right, sizeof right))
            return 0;

        newTemp(temp, sizeof temp);

        emit("%s = %s %s %s",
             temp, left, op == MUL ? "*" : "/", right);

        snprintf(left, sizeof left, "%s", temp);
    }

    snprintf(result, size, "%s", left);
    return !panic;
}

static int expression(char *result, size_t size)
{
    char left[MAX_OPERAND];
    char right[MAX_OPERAND];
    char temp[MAX_OPERAND];

    if (!term(left, sizeof left))
        return 0;

    while (tok == PLUS || tok == MINUS) {
        int op = tok;
        next();

        if (!term(right, sizeof right))
            return 0;

        newTemp(temp, sizeof temp);

        emit("%s = %s %s %s",
             temp, left, op == PLUS ? "+" : "-", right);

        snprintf(left, sizeof left, "%s", temp);
    }

    snprintf(result, size, "%s", left);
    return !panic;
}

/* ---------- Statements ---------- */


static void statement(void)
{
    char name[50];
    char result[MAX_OPERAND];
    int line;

    /* Record the state before parsing this statement. */
    int tacStart = tacCount;
    int errStart = errCount;

    panic = 0;

    if (tok == INT || tok == FLOAT) {
        /* Declaration -> type id [ = Expression ] ; */

        const char *type = (tok == INT) ? "int" : "float";
        next();

        if (tok == ID) {
            snprintf(name, sizeof name, "%s", yytext);
            line = yylineno;
            next();

            declare(name, type, line);

            if (tok == ASSIGN) {
                next();

                if (expression(result, sizeof result) && !panic)
                    emit("%s = %s", name, result);
            }

            match(SEMICOLON);
        }
        else {
            fail("identifier", yylineno);
        }
    }
    else if (tok == ID) {
        /* Assignment -> id = Expression ; */

        snprintf(name, sizeof name, "%s", yytext);
        checkDeclared(name, yylineno);
        next();

        if (match(ASSIGN)) {
            if (expression(result, sizeof result) && !panic)
                emit("%s = %s", name, result);

            match(SEMICOLON);
        }
    }
    else {
        fail("declaration or assignment", yylineno);
    }

    /*
       Discard TAC only if this statement introduced an error.
       Errors from earlier statements must not discard valid TAC
       generated by the current statement.
    */
    if (panic || errCount > errStart)
        tacCount = tacStart;

    if (panic)
        recover();
}


/* ---------- Main ---------- */

int main(void)
{
    printf("Enter the program (press Ctrl+D when finished):\n\n");

    next();

    while (tok != 0)
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

    if (symCount == 0)
        printf("No symbols declared.\n");

    printf("========================================\n");

    /* ---------- TAC output ---------- */

    printf("\n======= THREE-ADDRESS CODE (TAC) =======\n");

    if (tacCount == 0)
        printf("No TAC generated.\n");
    else {
        for (int i = 0; i < tacCount; i++)
            printf("%d: %s\n", i + 1, tac[i]);
    }

    printf("========================================\n");

    return errCount == 0 ? 0 : 1;
}
