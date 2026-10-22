CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic
TARGET = src/compiler
SOURCES = src/parser.tab.c src/lex.yy.c src/ast.cpp src/semantic.cpp src/semantic_analyzer.cpp src/ir.cpp src/interpreter.cpp

.PHONY: all clean test
all: $(TARGET)
$(TARGET): $(SOURCES) src/parser.tab.h
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)
src/parser.tab.c src/parser.tab.h: src/parser.y
	bison -Wall -d -o src/parser.tab.c src/parser.y
src/lex.yy.c: src/lexer.l src/parser.tab.h
	flex -o src/lex.yy.c src/lexer.l

test: $(TARGET)
	@set -e; for f in tests/valid_program.min tests/print_program.min tests/tac_basic.min tests/undefined_variable.min tests/duplicate_declaration.min tests/invalid_assignment.min tests/syntax_error.min tests/lexical_error.min tests/division_by_zero.min tests/control_flow.min tests/functions.min tests/recursive_function.min tests/short_circuit.min tests/scopes.min; do \
	  echo "===== $$f ====="; ./$(TARGET) < $$f || true; echo; \
	done
clean:
	rm -f $(TARGET) src/parser.tab.c src/parser.tab.h src/lex.yy.c src/*.o
