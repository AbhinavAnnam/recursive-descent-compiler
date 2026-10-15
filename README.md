# Recursive Descent Compiler

A small C project that uses Flex to tokenize a source program and a hand-written recursive-descent parser to check its syntax. It also performs basic variable-declaration checks and prints a symbol table. It does not generate machine code.

## How it works

1. `lexer.l` describes the tokens. Flex turns this file into `lex.yy.c`.
2. `recursive_descent.c` calls `yylex()` to read tokens one at a time.
3. The parser functions mirror the grammar below. Each function consumes the tokens for one grammar rule; expression parsing preserves the usual precedence of `*` and `/` over `+` and `-`.
4. While parsing, the program records declarations, reports duplicate declarations and uses of undeclared variables, and attempts to continue after errors so it can report more than one issue.
5. At the end, it prints a compilation report and the symbol table.

## Supported grammar

```text
program     -> statement*
statement   -> declaration | assignment
declaration -> ("int" | "float") identifier [ "=" expression ] ";"
assignment  -> identifier "=" expression ";"
expression  -> term { ("+" | "-") term }
term        -> factor { ("*" | "/") factor }
factor      -> identifier | number | "(" expression ")"
```

Identifiers start with a letter or underscore and may contain letters, digits, and underscores. Numbers are integers or decimals, such as `10` or `4.5`.

Declarations can include an initial value, or be followed by a separate assignment. For example:

```text
int total;
int count = 10 + 2 * (3 - 1);
float rate = 4.5;
total = count / 2;
rate = total / 4.5;
```

The current grammar does not include `if` statements, comparisons, or unary operators such as `-5`. The semantic checks do not check initializer or assignment types, or evaluate expressions.

## Build and run on Ubuntu or WSL

Install Flex and GCC if needed:

```bash
sudo apt update
sudo apt install flex build-essential
```

From the project directory, generate the lexer and compile the program:

```bash
flex -o lex.yy.c lexer.l
gcc recursive_descent.c lex.yy.c -o parser
./parser
```

Enter statements, then press **Ctrl+D** to signal end of input. To parse a file instead:

```bash
./parser < input.txt
```

`lex.yy.c` is generated from `lexer.l` and is ignored by Git. Regenerate it with `flex lexer.l` after changing the lexer rules.

## Build and run in PowerShell

PowerShell needs Flex and GCC available on `PATH` (for example, through an installed MinGW/MSYS2 toolchain). From the project directory, run:

```powershell
flex lexer.l
gcc recursive_descent.c lex.yy.c -o parser
```

Run interactively with:

```powershell
.\parser.exe
```

Press **Ctrl+Z**, then **Enter**, to signal end of input in the Windows console. To parse a file, use `.\parser.exe < input.txt`.

## Project files

- `lexer.l` - Flex token rules.
- `recursive_descent.c` - recursive-descent parser, semantic checks, error reporting, and `main`.
- `recursive_descent.h` - token definitions and lexer declarations shared by the C files.
- `lex.yy.c` - generated lexer source; recreate it with Flex.
