# Compiler toolchain and flags. Flex/Bison output is generated from src/*.l/.y.
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic
TARGET = src/compiler
SOURCES = src/parser.tab.c src/lex.yy.c src/ast.cpp src/semantic_analyzer.cpp src/ir.cpp src/interpreter.cpp src/optimizer.cpp src/target_codegen.cpp
TEST_FILES = $(sort $(wildcard tests/*.min))
# These fixtures deliberately exercise diagnostics and should exit non-zero.
EXPECTED_FAILURES = tests/division_by_zero.min tests/duplicate_declaration.min tests/invalid_assignment.min tests/lexical_error.min tests/syntax_error.min tests/undefined_variable.min

.PHONY: all clean test test-native

all: $(TARGET)

$(TARGET): $(SOURCES) src/parser.tab.h
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

# Generate parser declarations/implementation from the grammar.
src/parser.tab.c: src/parser.y
	bison -Wall -d -o src/parser.tab.c src/parser.y

# Bison emits the header alongside parser.tab.c. Regenerate if only the header
# was removed, while keeping the rule compatible with older GNU make versions.
src/parser.tab.h: src/parser.tab.c
	@test -f $@ || { rm -f src/parser.tab.c; $(MAKE) src/parser.tab.c; }

# Generate the scanner after the parser token definitions exist.
src/lex.yy.c: src/lexer.l src/parser.tab.h
	flex -o src/lex.yy.c src/lexer.l

# Run every fixture and verify whether its exit status matches its purpose.
# Error cases are expected to fail; `make test` succeeds only if each status matches.
test: $(TARGET)
	@set -eu; \
	for f in $(TEST_FILES); do \
	  echo "===== $$f ====="; \
	  case " $(EXPECTED_FAILURES) " in \
	    *" $$f "*) \
	      if ./$(TARGET) < "$$f"; then \
	        echo "FAIL: expected this test to return a non-zero status: $$f"; exit 1; \
	      else \
	        echo "PASS (expected failure)"; \
	      fi ;; \
	    *) \
	      ./$(TARGET) < "$$f"; \
	      echo "PASS" ;; \
	  esac; \
	  echo; \
	done

# Remove generated/build outputs while keeping the grammar and handwritten source.
clean:
	rm -f $(TARGET) src/parser.tab.c src/parser.tab.h src/lex.yy.c src/*.o minilang_output.s minilang_program


# Compile representative fixtures to native x86-64 executables and compare output.
test-native: $(TARGET)
	@set -eu; \
	tmpdir=$$(mktemp -d); trap 'rm -rf "$$tmpdir"' EXIT; \
	for f in tests/print_program.min tests/control_flow.min tests/functions.min tests/recursive_function.min tests/short_circuit.min tests/scopes.min tests/optimization.min; do \
	  echo "===== native $$f ====="; \
	  ./$(TARGET) --emit-asm "$$tmpdir/program.s" < "$$f" > "$$tmpdir/compiler.log"; \
	  gcc "$$tmpdir/program.s" -o "$$tmpdir/program"; \
	  "$$tmpdir/program" > "$$tmpdir/actual"; \
	  case "$$f" in \
	    tests/print_program.min) expected=60 ;; \
	    tests/control_flow.min) expected=5 ;; \
	    tests/functions.min) expected=30 ;; \
	    tests/recursive_function.min) expected=120 ;; \
	    tests/short_circuit.min) expected=$$(printf '7\n8') ;; \
	    tests/scopes.min) expected=$$(printf '2\n1') ;; \
	    tests/optimization.min) expected=$$(printf '14\n15\n7') ;; \
	  esac; \
	  actual=$$(cat "$$tmpdir/actual"); \
	  if [ "$$actual" != "$$expected" ]; then \
	    echo "FAIL: native output mismatch for $$f"; \
	    echo "Expected: $$expected"; echo "Actual: $$actual"; \
	    cat "$$tmpdir/compiler.log"; exit 1; \
	  fi; \
	  echo "PASS: native output matched"; \
	done
