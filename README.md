# MiniCompiler — MiniLang

MiniLang is a small C++ compiler/interpreter built with Flex and Bison. The pipeline performs lexical analysis, parsing, AST construction, semantic checks, three-address-code generation, and AST interpretation.

## Build

Install `g++`, `make`, `flex`, and `bison`, then run:

```bash
make clean
make
make test
```

## Language examples

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

fn add(a, b) {
  return a + b;
}
let result = add(10, 20);
print(result);
```

Supported constructs include integer variables, assignment, `print`, arithmetic (`+ - * /`), unary minus, comparisons (`< > <= >= == !=`), Boolean operators (`&& || !`), nested blocks, `if/else`, `while`, and top-level functions with parameters and integer return values. Boolean values are represented as integers: zero is false and nonzero is true. Function declarations use `fn name(parameters) { ... }`.

Function declarations are top-level only. Blocks create lexical scopes. This is a compact educational language, not a complete C implementation.
