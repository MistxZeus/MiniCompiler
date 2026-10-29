# MiniCompiler — MiniLang Compiler

MiniCompiler is a small compiler and interpreter for **MiniLang**, a C-like teaching language. It demonstrates the major stages of a compiler pipeline: lexical analysis, parsing, abstract syntax tree (AST) construction, semantic analysis, optimization, three-address code (TAC) generation, and execution.

> This is an educational compiler project. Its supported behavior is defined by the grammar, implementation, and regression tests in this repository; it is not intended to implement all of C or C++.

## Features

- **Lexical analysis:** tokenizes keywords, identifiers, integer literals, operators, delimiters, and invalid characters using Flex.
- **Syntax analysis:** parses MiniLang programs using Bison.
- **AST:** represents expressions, declarations, assignments, printing, blocks, conditionals, loops, functions, calls, and returns.
- **Semantic analysis:** checks variable declarations and use, duplicate declarations in the same scope, function declarations/calls, argument counts, and related semantic constraints.
- **Expressions:** integer arithmetic (`+`, `-`, `*`, `/`), comparisons (`<`, `>`, `<=`, `>=`, `==`, `!=`), Boolean operators (`&&`, `||`, `!`), unary minus, and parentheses.
- **Statements and control flow:** `let` declarations, assignments, `print(...)`, blocks, `if/else`, and `while`.
- **Functions:** parameters, calls, return values, and recursive calls.
- **Intermediate representation:** emits three-address code, including temporaries, labels, conditional/unconditional jumps, parameters, calls, and returns.
- **Optimization:** constant folding, constant-branch simplification, and dead-statement removal for supported cases.
- **Interpreter:** executes the optimized AST and reports runtime errors such as division by zero.

## Compiler pipeline

```text
MiniLang source
      |
      v
Flex lexer -> Bison parser -> Abstract Syntax Tree
                                  |
                                  v
                           Semantic analysis
                                  |
                                  v
                            AST optimization
                             /           \
                            v             v
                  Three-address code   AST interpreter
                       (TAC)           / runtime state
```

The implementation performs semantic analysis before optimization so that invalid source programs are rejected before the optimizer runs. The optimizer transforms the AST. The IR generator emits TAC for inspection, while the interpreter separately executes the optimized AST directly; TAC is not currently fed into the interpreter.

## Requirements

On Ubuntu or WSL:

- `g++` with C++17 support
- `make`
- Flex
- Bison

Install the build tools if needed:

```bash
sudo apt update
sudo apt install build-essential flex bison
```

## Build

From the repository root:

```bash
make clean
make
```

The compiler executable is produced at:

```text
src/compiler
```

Run a source program through standard input:

```bash
./src/compiler < tests/valid_program.min
```

Or use a heredoc:

```bash
./src/compiler <<'EOF'
let x = 10;
let y = 20;
print(x + y);
EOF
```

Expected program output includes:

```text
Program Output: 30
```

The compiler also prints its pipeline stages, AST, symbol table, optimization summary, TAC, and execution status.

## Running the regression suite

```bash
make test
```

The suite includes valid programs and intentional error cases. `make test` checks the process exit status for every fixture: valid programs must succeed, while known diagnostic cases must exit non-zero. The complete fixture-by-fixture catalog is in [`tests/README.md`](tests/README.md).

Current test files include:

- `valid_program.min`
- `print_program.min`
- `parentheses.min`
- `tac_basic.min`
- `control_flow.min`
- `functions.min`
- `recursive_function.min`
- `short_circuit.min`
- `scopes.min`
- `optimization.min`
- `undefined_variable.min`
- `duplicate_declaration.min`
- `invalid_assignment.min`
- `syntax_error.min`
- `lexical_error.min`
- `division_by_zero.min`

## MiniLang examples

### Variables and arithmetic

```text
let x = 10;
let y = 20;
let z = x + y * 2;
print(z);
```

Multiplication has higher precedence than addition. Parentheses can be used to override precedence.

### Conditional and loop

```text
let x = 0;

while (x < 5) {
    x = x + 1;
}

if (x == 5 && !(x > 5)) {
    print(x);
} else {
    print(0);
}
```

Expected program output:

```text
Program Output: 5
```

### Functions and recursion

```text
fn fact(n) {
    if (n <= 1) {
        return 1;
    }
    return n * fact(n - 1);
}

print(fact(5));
```

Expected program output:

```text
Program Output: 120
```

### Optimization example

```text
print(2 + 3 * 4);

if (0) {
    print(999);
} else {
    print(10 + 5);
}

while (0) {
    print(888);
}
```

The optimizer can fold the arithmetic expressions, simplify constant branches, and remove the unreachable loop body for these supported cases.

## Intermediate representation

For example, the source:

```text
let x = 10;
let y = 20;
print(x + y);
```

can be represented with TAC similar to:

```text
x = 10
y = 20
t1 = x + y
PRINT t1
```

Control-flow constructs use labels and jumps. Function code uses function markers, parameter declarations, calls, and returns.

## Error handling

The compiler reports errors at different stages:

- **Lexical:** an unknown character is encountered.
- **Syntax:** source text does not match the grammar.
- **Semantic:** an undeclared variable, duplicate declaration, undeclared function, or invalid function argument count is detected.
- **Runtime:** an executed operation such as division by zero fails.

An error inside a branch that is proven unreachable may be removed by constant-branch optimization; an error in an executed branch should remain observable.

## Repository workflow

Check the current branch and changes before committing:

```bash
git status
git log --oneline -5
git diff --check
```

Build and test before committing:

```bash
make clean && make && make test
```

## Known scope and future work

This is a compact educational implementation, so its supported language and optimization rules are intentionally limited. Potential next improvements include:

- Golden-output comparisons and broader regression coverage for the existing pass/fail test runner.
- Differential testing of optimized and unoptimized execution.
- Stronger type and return-path analysis.
- Additional IR-level optimizations and optimization correctness checks.
- Source locations in diagnostics.
- Documentation of exact grammar and operator precedence from `src/parser.y`.

## Project structure

```text
MiniCompiler/
├── Makefile
├── README.md
├── src/
│   ├── lexer.l
│   ├── parser.y
│   ├── ast.h
│   ├── ast.cpp
│   ├── semantic_analyzer.h
│   ├── semantic_analyzer.cpp
│   ├── ir.h
│   ├── ir.cpp
│   ├── optimizer.h
│   ├── optimizer.cpp
│   ├── interpreter.h
│   └── interpreter.cpp
└── tests/
    ├── README.md
    └── *.min
```

Generated parser/lexer files and the compiled executable are build artifacts and are intentionally ignored by Git. The source of truth is `src/lexer.l`, `src/parser.y`, and the handwritten C++ files. Older phase snapshots, an empty stray file, and an unused flat `SymbolTable` implementation were removed; the scope-aware `SemanticAnalyzer` is the symbol table implementation used by the compiler. Use Git history or release tags for version history rather than checking compressed copies into the working tree.
