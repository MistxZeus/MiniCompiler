# MiniLang regression fixtures

Each `.min` file is a small input program for `make test`. The MiniLang lexer does
not currently define source-comment syntax, so these fixtures are kept as valid
language inputs without descriptive comment lines. This catalog documents their
purpose without changing the programs being tested.

| Fixture | Expected result | Purpose |
|---|---|---|
| `valid_program.min` | Success | Variable declarations, precedence, arithmetic, and assignment. |
| `print_program.min` | Success | Parenthesized arithmetic and `print`. |
| `parentheses.min` | Success | Parentheses override normal operator precedence. |
| `tac_basic.min` | Success | Basic expressions and assignment for TAC inspection. |
| `control_flow.min` | Success | `while`, `if/else`, comparisons, and Boolean operators. |
| `functions.min` | Success | Function declaration, parameters, call, and return value. |
| `recursive_function.min` | Success | Recursive function calls and conditional return. |
| `short_circuit.min` | Success | `&&` and `||` skip a right-hand division-by-zero expression when appropriate. |
| `scopes.min` | Success | Inner-scope shadowing does not overwrite the outer variable. |
| `optimization.min` | Success | Constant folding, branch simplification, and removal after return. |
| `undefined_variable.min` | Expected failure | Semantic analysis rejects use of an undeclared variable. |
| `duplicate_declaration.min` | Expected failure | Semantic analysis rejects duplicate declarations in one scope. |
| `invalid_assignment.min` | Expected failure | Semantic analysis rejects assignment to an undeclared variable. |
| `syntax_error.min` | Expected failure | Parser rejects malformed declaration syntax. |
| `lexical_error.min` | Expected failure | Lexer rejects an unsupported character. |
| `division_by_zero.min` | Expected failure | Runtime detects division by zero in an expression that is executed. |

`make test` verifies expected success/failure exit codes. It prints compiler output
for manual inspection; it does not compare complete output text against golden files.
