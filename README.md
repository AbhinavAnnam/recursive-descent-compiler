# Recursive Descent Compiler

A small C project that uses **Flex** for lexical analysis and a hand-written **recursive-descent parser** for syntax checking and basic semantic validation. The compiler supports variable declarations, assignments, arithmetic expressions, and three-address code (TAC) generation.

It also maintains a symbol table, reports syntax and semantic errors with line numbers, and attempts to recover from malformed statements so that additional errors can be reported. It does not generate machine code or execute the input program.

## Features

- **Lexical analysis:** Uses Flex to recognize keywords, identifiers, numbers, arithmetic operators, assignment operators, parentheses, semicolons, and braces.
- **Recursive-descent parsing:** Parses supported arithmetic expressions while respecting operator precedence and parentheses.
- **Symbol table:** Records declared variables and their types (`int` or `float`).
- **Semantic checks:** Detects duplicate declarations and uses or assignments of undeclared variables.
- **Three-address code generation:** Generates intermediate instructions for declarations with initializers, assignments, and arithmetic expressions.
- **Compound assignments:** Supports `+=` and `-=`.
- **Unary minus:** Supports negative numbers and negated expressions.
- **Error reporting:** Reports compilation errors with line numbers and displays the compilation report, symbol table, and generated TAC.
- **Error recovery:** Attempts to continue processing after malformed supported statements.
- **File input:** Accepts source code from standard input or a redirected input file.

## How It Works

1. `lexer.l` defines the lexical rules. Flex converts these rules into the generated source file `lex.yy.c`.
2. `recursive_descent.c` calls `yylex()` to retrieve tokens from the input program.
3. Recursive-descent functions parse declarations, assignments, and arithmetic expressions.
4. The parser maintains a symbol table and checks for duplicate declarations and undeclared variables.
5. As expressions are parsed, the compiler generates three-address code using temporary variables such as `t1`, `t2`, and `t3`.
6. The compiler prints a compilation report, the symbol table, and the numbered TAC instructions.

## Supported Grammar

The following grammar describes the main supported constructs:

```text
program            -> statement*

statement          -> declaration
                    | assignment
                    | compound_assignment

declaration        -> type identifier
                      [ "=" expression ] ";"

type               -> "int" | "float"

assignment         -> identifier "=" expression ";"

compound_assignment
                   -> identifier "+=" expression ";"
                    | identifier "-=" expression ";"

expression         -> term { ("+" | "-") term }

term               -> factor { ("*" | "/") factor }

factor             -> identifier
                    | number
                    | "-" factor
                    | "(" expression ")"
```

### Identifiers and Numbers

Identifiers start with a letter or underscore and may contain letters, digits, and underscores.

Examples of valid identifiers:

```text
total
count
_rate
value2
```

The lexer recognizes integer and decimal numbers, such as:

```text
10
25
4.5
0.75
```

### Operator Precedence

Multiplication and division have higher precedence than addition and subtraction. Parentheses can be used to change the evaluation order.

For example:

```c
int a;
a = 10 + 20 * 3;
```

The multiplication is performed first.

Generated TAC:

```text
1: t1 = 20 * 3
2: t2 = 10 + t1
3: a = t2
```

Parentheses change the evaluation order:

```c
int b;
b = (10 + 20) * 3;
```

The addition is performed before the multiplication.

## Example Input

The following example demonstrates declarations, initialization, arithmetic expressions, negative values, and compound assignments.

```c
#include <stdio.h>

void main() {
    int total;
    int count = 10 + 2 * (3 - 1);
    float rate = 4.5;

    total = count / 2;
    rate = total / 4.5;

    count += 2;
    total -= 1;

    return 0;
}
```

The compiler is designed to ignore unsupported surrounding C constructs, such as preprocessor directives, function declarations, and `return` statements, while processing supported declarations and assignments.

**Note:** This project is not a complete C compiler. The example illustrates the intended input style and supported statements; unsupported C constructs are not fully parsed or validated.

## Example Output

The output contains three sections.

### 1. Compilation Report

Displays whether compilation succeeded or failed and lists detected errors with line numbers.

```text
========== COMPILATION REPORT ==========
Compilation successful with 0 error(s).
```

### 2. Symbol Table

Lists declared identifiers and their declared types.

```text
========== SYMBOL TABLE ==========
Identifier           Type
total                int
count                int
rate                 float
==================================
```

### 3. Three-Address Code (TAC)

Displays the generated intermediate instructions in numbered order.

For example, an assignment such as:

```c
total = 10 + 20 * 3;
```

can generate:

```text
1: t1 = 20 * 3
2: t2 = 10 + t1
3: total = t2
```

The exact temporary-variable names and instruction numbering depend on the complete input program.

## Error Handling

The compiler reports syntax and semantic errors while attempting to continue parsing subsequent statements.

Examples of invalid input include:

**Missing semicolon**

```c
int a;
a = 10
```

**Malformed expression**

```c
int a;
a = 10 + ;
```

**Use of an undeclared variable**

```c
b = 10;
```

**Duplicate declaration**

```c
int a;
int a;
```

The compiler should report the relevant errors and display its compilation report, symbol table, and TAC section. TAC generated by a malformed statement is discarded.

Error recovery is best-effort and may not recover perfectly from every malformed input.

## Limitations

The project supports a limited subset of C and has the following limitations:

- No complete parsing of function declarations or function bodies.
- No support for `if`, `else`, loops, or comparison and logical expressions.
- No support for increment/decrement operators such as `++` and `--`.
- No support for compound assignments other than `+=` and `-=`.
- No complete C type checking for assignments or initializers.
- No evaluation of expressions or execution of the input program.
- No machine-code generation.
- Unsupported constructs may be ignored rather than validated.

The symbol table records declared types, but the current implementation does not enforce type compatibility between expressions and variables.

## Project Structure

```text
recursive-descent-compiler/
├── lexer.l
├── recursive_descent.c
├── recursive_descent.h
├── lex.yy.c              # Generated by Flex
├── input.txt             # Example input file
└── README.md
```

- `lexer.l` — Flex rules for tokenizing the input.
- `recursive_descent.c` — Recursive-descent parsing, semantic checks, error reporting, symbol-table management, and TAC generation.
- `recursive_descent.h` — Shared token definitions and lexer declarations.
- `lex.yy.c` — Generated lexer source; recreate it using Flex.
- `README.md` — Project documentation.

`lex.yy.c` is generated from `lexer.l` and should not normally be edited manually.

## Build and Run on Ubuntu or WSL

### 1. Install dependencies

```bash
sudo apt update
sudo apt install flex gcc
```

### 2. Generate the lexer and compile

From the project directory, run:

```bash
flex -o lex.yy.c lexer.l
gcc recursive_descent.c lex.yy.c -o compiler
```

### 3. Run interactively

```bash
./compiler
```

Enter supported statements. Press **Ctrl+D** on a new line to signal the end of input in a Linux terminal.

### 4. Run using a file

Create a text file containing the source program, for example `input.txt`, and run:

```bash
./compiler < input.txt
```

The compiler reads the file through standard input and prints the compilation report, symbol table, and TAC to the terminal.

## Build and Run on Windows with MSYS2

Ensure Flex and GCC are installed and available in your MSYS2 UCRT64 terminal.

### 1. Generate the lexer

```bash
flex lexer.l
```

### 2. Compile the program

```bash
gcc recursive_descent.c lex.yy.c -o compiler
```

### 3. Run interactively

```bash
./compiler
```

In a Windows console, use the appropriate end-of-input shortcut for your terminal. In MSYS2's interactive terminal, `Ctrl+D` may behave differently depending on the terminal and input mode. File redirection is a reliable alternative.

### 4. Run using a file

```bash
./compiler < input.txt
```

You can also use a `.c` file as input:

```bash
./compiler < test1.c
```

The extension does not determine what the compiler accepts; the lexer and parser determine which constructs are recognized.

## Rebuilding After Changes

Whenever you modify `lexer.l` or `recursive_descent.c`, regenerate and rebuild the compiler:

```bash
flex lexer.l
gcc recursive_descent.c lex.yy.c -o compiler
```

If you only modify the input file, rerun the compiler with that file:

```bash
./compiler < input.txt
```

## Technologies Used

- **C** — Parser implementation, symbol table, error reporting, and TAC generation.
- **Flex** — Lexical analysis and token generation.
- **GCC** — Compilation of the generated lexer and parser.
- **Recursive-descent parsing** — Syntax analysis using hand-written parsing functions.

## Purpose

This project demonstrates the fundamental stages of a small compiler: lexical analysis, syntax analysis, basic semantic checks, symbol-table management, error recovery, and intermediate-code generation. It is intended for learning and experimenting with compiler design concepts.
