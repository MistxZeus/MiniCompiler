# MiniLang Compiler

A small compiler frontend developed for the Compiler Design Laboratory.

The project demonstrates the major stages of a compiler frontend using
Flex and Bison, with a custom Abstract Syntax Tree and semantic analysis.

## Current Implementation

The current prototype implements:

- Lexical Analysis using Flex
- Syntax Analysis using Bison
- Abstract Syntax Tree (AST) construction
- Arithmetic expressions
- Variable declarations
- Variable assignments
- Multiple statements
- Operator precedence
- AST visualization
- Symbol Table
- Semantic Analysis
- Undefined variable detection
- Duplicate declaration detection
- Invalid assignment detection

## Compiler Architecture

```text
Source Code
     |
     v
Lexical Analysis
     |
     v
Tokens
     |
     v
Syntax Analysis
     |
     v
Abstract Syntax Tree
     |
     v
Semantic Analysis
     |
     +----> Symbol Table
     |
     v
Validated Program