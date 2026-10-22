
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "recursive_descent.h"

#define MAX_SYMBOLS 500
#define MAX_TAC 1000
#define MAX_NAME 128
#define MAX_LINE 512
#define MAX_ERRORS 200

/*
Program       → StatementList
StatementList → Statement StatementList | ε
Statement     → Declaration | Assignment
Declaration   → type id [ = Expression ] ;
Assignment    → id = Expression ;
Expression    → Term { (+|-) Term }
Term          → Factor { (*|/) Factor }
Factor        → ( Expression ) | id | num
type          → int | float
num           → digits | digits . digits

*/
typedef struct {
    char name[MAX_NAME];
    char type[16];
} Symbol;

typedef struct {
    int line;
    char message[MAX_LINE];
} CompileError;

static Symbol symbols[MAX_SYMBOLS];
static int symbolCount = 0;

static char tac[MAX_TAC][MAX_LINE];
static int tacCount = 0;
static int tempCount = 0;

static CompileError compileErrors[MAX_ERRORS];
static int errors = 0;

static int token;
static int tokenLine = 1;

/* ---------- Lexer helpers ---------- */

static void nextToken(void) {
    token = yylex();
    tokenLine = yylineno;
}

static void copyTokenText(char *dest, size_t size) {
    snprintf(dest, size, "%s", yytext);
}

/* ---------- Error reporting ---------- */

static void reportErrorAt(int line, const char *message) {
    if (errors < MAX_ERRORS) {
        compileErrors[errors].line = line;
        snprintf(compileErrors[errors].message,
                 MAX_LINE, "%s", message);
    }
    errors++;
}

static void reportError(const char *message) {
    reportErrorAt(tokenLine, message);
}

/* ---------- Symbol table ---------- */

static int findSymbol(const char *name) {
    for (int i = 0; i < symbolCount; i++) {
        if (strcmp(symbols[i].name, name) == 0)
            return i;
    }
    return -1;
}

static int declareSymbol(const char *name, const char *type) {
    if (findSymbol(name) >= 0) {
        reportError("Variable already declared.");
        return 0;
    }

    if (symbolCount >= MAX_SYMBOLS) {
        reportError("Symbol table is full.");
        return 0;
    }

    snprintf(symbols[symbolCount].name,
             sizeof(symbols[symbolCount].name), "%s", name);
    snprintf(symbols[symbolCount].type,
             sizeof(symbols[symbolCount].type), "%s", type);

    symbolCount++;
    return 1;
}

/* ---------- TAC helpers ---------- */

static void emit(const char *text) {
    if (tacCount < MAX_TAC) {
        snprintf(tac[tacCount], MAX_LINE, "%s", text);
        tacCount++;
    } else {
        reportError("TAC buffer is full.");
    }
}

static void newTemp(char *dest, size_t size) {
    tempCount++;
    snprintf(dest, size, "t%d", tempCount);
}

static void emitOperation(char *result, size_t size,
                          const char *left, const char *op,
                          const char *right) {
    char temp[MAX_NAME];
    char line[MAX_LINE];

    newTemp(temp, sizeof(temp));

    snprintf(line, sizeof(line), "%s = %s %s %s",
             temp, left, op, right);
    emit(line);

    snprintf(result, size, "%s", temp);
}

/* ---------- Expression parser ---------- */

static int parseExpression(char *result, size_t size);

static int parseFactor(char *result, size_t size) {
    if (token == NUM) {
        copyTokenText(result, size);
        nextToken();
        return 1;
    }

    if (token == ID) {
        char name[MAX_NAME];

        copyTokenText(name, sizeof(name));
        int line = tokenLine;
        nextToken();

        if (findSymbol(name) < 0) {
            reportErrorAt(line, "Use of an undeclared variable.");
            return 0;
        }

        snprintf(result, size, "%s", name);
        return 1;
    }

    /* Support unary minus: -1, -a, -(a + b). */
    if (token == MINUS) {
        nextToken();

        char operand[MAX_LINE];

        if (!parseFactor(operand, sizeof(operand)))
            return 0;

        if (operand[0] >= '0' && operand[0] <= '9') {
            snprintf(result, size, "-%s", operand);
        } else {
            emitOperation(result, size, "0", "-", operand);
        }

        return 1;
    }

    if (token == LPAREN) {
        nextToken();

        if (!parseExpression(result, size))
            return 0;

        if (token != RPAREN) {
            reportError("Expected ')'.");
            return 0;
        }

        nextToken();
        return 1;
    }

    reportError(
        "Expected a number, variable, unary minus, or parenthesized expression."
    );
    return 0;
}

static int parseTerm(char *result, size_t size) {
    if (!parseFactor(result, size))
        return 0;

    while (token == MUL || token == DIV) {
        const char *op = (token == MUL) ? "*" : "/";
        char left[MAX_LINE];
        char right[MAX_LINE];

        snprintf(left, sizeof(left), "%s", result);
        nextToken();

        if (!parseFactor(right, sizeof(right)))
            return 0;

        emitOperation(result, size, left, op, right);
    }

    return 1;
}

static int parseExpression(char *result, size_t size) {
    if (!parseTerm(result, size))
        return 0;

    while (token == PLUS || token == MINUS) {
        const char *op = (token == PLUS) ? "+" : "-";
        char left[MAX_LINE];
        char right[MAX_LINE];

        snprintf(left, sizeof(left), "%s", result);
        nextToken();

        if (!parseTerm(right, sizeof(right)))
            return 0;

        emitOperation(result, size, left, op, right);
    }

    return 1;
}

/* ---------- Error recovery ---------- */

static void recoverStatement(void) {
    while (token != 0 &&
           token != SEMICOLON &&
           token != RBRACE &&
           token != INT &&
           token != FLOAT) {
        nextToken();
    }

    if (token == SEMICOLON)
        nextToken();
}

/* Ignore unsupported C constructs, preserving block braces. */
static void skipUnsupported(void) {
    int parenDepth = 0;

    while (token != 0) {
        if (token == LPAREN) {
            parenDepth++;
            nextToken();
        } else if (token == RPAREN) {
            if (parenDepth > 0)
                parenDepth--;
            nextToken();
        } else if (parenDepth == 0 &&
                   (token == LBRACE || token == RBRACE)) {
            return;
        } else if (parenDepth == 0 && token == SEMICOLON) {
            nextToken();
            return;
        } else {
            nextToken();
        }
    }
}

/* ---------- Declaration parser ---------- */

static void parseDeclaration(void) {
    char type[16];
    char name[MAX_NAME];
    char result[MAX_LINE];

    int statementLine = tokenLine;
    int tacStart = tacCount;
    int tempStart = tempCount;
    int errorStart = errors;
    int symbolStart = symbolCount;

    snprintf(type, sizeof(type), "%s",
             token == INT ? "int" : "float");

    nextToken();

    if (token != ID) {
        reportErrorAt(statementLine,
                      "Expected a variable name after the type.");
        recoverStatement();
        return;
    }

    copyTokenText(name, sizeof(name));
    nextToken();

    int declared = declareSymbol(name, type);

    if (!declared) {
        recoverStatement();
        return;
    }

    if (token == ASSIGN) {
        nextToken();

        if (!parseExpression(result, sizeof(result))) {
            tacCount = tacStart;
            tempCount = tempStart;
            symbolCount = symbolStart;
            recoverStatement();
            return;
        }

        if (token != SEMICOLON) {
            reportErrorAt(statementLine,
                          "Expected ';' after declaration.");

            tacCount = tacStart;
            tempCount = tempStart;
            symbolCount = symbolStart;
            recoverStatement();
            return;
        }

        /* Emit the final assignment after expression operations. */
        char line[MAX_LINE];
        snprintf(line, sizeof(line), "%s = %s", name, result);
        emit(line);
    } else if (token != SEMICOLON) {
        reportErrorAt(statementLine,
                      "Expected '=' or ';' after variable name.");

        tacCount = tacStart;
        tempCount = tempStart;
        symbolCount = symbolStart;
        recoverStatement();
        return;
    }

    if (token == SEMICOLON)
        nextToken();

    if (errors > errorStart) {
        tacCount = tacStart;
        tempCount = tempStart;
        symbolCount = symbolStart;
    }
}

/* ---------- Assignment parser ---------- */

static void parseAssignment(const char *name, int statementLine) {
    char result[MAX_LINE];

    int tacStart = tacCount;
    int tempStart = tempCount;
    int errorStart = errors;

    /*
     * Validate the left-hand variable BEFORE parsing the expression.
     * Assignments must never implicitly declare variables.
     */
    if (findSymbol(name) < 0) {
        reportErrorAt(statementLine,
                      "Assignment to an undeclared variable.");
        recoverStatement();
        return;
    }

    nextToken(); /* Consume '='. */

    if (!parseExpression(result, sizeof(result))) {
        tacCount = tacStart;
        tempCount = tempStart;
        recoverStatement();
        return;
    }

    if (token != SEMICOLON) {
        reportErrorAt(statementLine,
                      "Expected ';' after assignment.");

        tacCount = tacStart;
        tempCount = tempStart;
        recoverStatement();
        return;
    }

    if (errors == errorStart) {
        char line[MAX_LINE];
        snprintf(line, sizeof(line), "%s = %s", name, result);
        emit(line);
    } else {
        tacCount = tacStart;
        tempCount = tempStart;
    }

    nextToken();
}

/* ---------- Compound assignment parser ---------- */

static void parseCompoundAssignment(const char *name,
                                    const char *op,
                                    int statementLine) {
    char result[MAX_LINE];

    int tacStart = tacCount;
    int tempStart = tempCount;
    int errorStart = errors;

    if (findSymbol(name) < 0) {
        reportErrorAt(statementLine,
                      "Assignment to an undeclared variable.");
        recoverStatement();
        return;
    }

    nextToken(); /* Consume += or -=. */

    if (!parseExpression(result, sizeof(result))) {
        tacCount = tacStart;
        tempCount = tempStart;
        recoverStatement();
        return;
    }

    if (token != SEMICOLON) {
        reportErrorAt(statementLine,
                      "Expected ';' after compound assignment.");

        tacCount = tacStart;
        tempCount = tempStart;
        recoverStatement();
        return;
    }

    if (errors == errorStart) {
        char temp[MAX_NAME];
        char line[MAX_LINE];

        emitOperation(temp, sizeof(temp), name, op, result);
        snprintf(line, sizeof(line), "%s = %s", name, temp);
        emit(line);
    } else {
        tacCount = tacStart;
        tempCount = tempStart;
    }

    nextToken();
}

/* ---------- Main scanner ---------- */

int main(void) {
    nextToken();

    while (token != 0) {
        if (token == INT || token == FLOAT) {
            parseDeclaration();
        } else if (token == ID) {
            char name[MAX_NAME];
            int statementLine = tokenLine;

            copyTokenText(name, sizeof(name));
            nextToken();

            if (token == ASSIGN) {
                parseAssignment(name, statementLine);
            } else if (token == PLUS_ASSIGN) {
                parseCompoundAssignment(name, "+", statementLine);
            } else if (token == MINUS_ASSIGN) {
                parseCompoundAssignment(name, "-", statementLine);
            } else {
                skipUnsupported();
            }
        } else {
            nextToken();
        }
    }

    /* ---------- Compilation report ---------- */

    printf("\n========== COMPILATION REPORT ==========\n");

    if (errors == 0) {
        printf("Compilation successful with 0 error(s).\n");
    } else {
        printf("Compilation failed with %d error(s):\n", errors);

        int displayed = errors < MAX_ERRORS ? errors : MAX_ERRORS;

        for (int i = 0; i < displayed; i++) {
            printf("Line %d: Syntax Error: %s\n",
                   compileErrors[i].line,
                   compileErrors[i].message);
        }

        if (errors > MAX_ERRORS) {
            printf("Additional errors omitted: %d\n",
                   errors - MAX_ERRORS);
        }
    }

    /* ---------- Symbol table ---------- */

    printf("\n========== SYMBOL TABLE ==========\n");
    printf("%-20s %s\n", "Identifier", "Type");

    for (int i = 0; i < symbolCount; i++) {
        printf("%-20s %s\n",
               symbols[i].name, symbols[i].type);
    }

    printf("==================================\n");

    /* ---------- Three-address code ---------- */

    printf("\n========== THREE-ADDRESS CODE (TAC) ==========\n");

    if (tacCount == 0) {
        printf("No three-address code generated.\n");
    } else {
        for (int i = 0; i < tacCount; i++) {
            printf("%d: %s\n", i + 1, tac[i]);
        }
    }

    printf("==============================================\n");

    return errors > 0 ? 1 : 0;
}
