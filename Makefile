CXX = g++

CXXFLAGS = -std=c++17

TARGET = src/compiler

SOURCES = src/parser.tab.c \
          src/lex.yy.c \
          src/ast.cpp \
          src/semantic.cpp \
          src/semantic_analyzer.cpp \
          src/ir.cpp \
          src/interpreter.cpp

all: $(TARGET)

$(TARGET): src/parser.tab.c src/lex.yy.c $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

src/parser.tab.c src/parser.tab.h: src/parser.y
	bison -d -o src/parser.tab.c src/parser.y

src/lex.yy.c: src/lexer.l src/parser.tab.h
	flex -o src/lex.yy.c src/lexer.l

test: $(TARGET)
	@echo "========================================"
	@echo "           RUNNING TEST SUITE"
	@echo "========================================"
	@echo ""
	@echo "[TEST 1] Valid Program"
	@./$(TARGET) < tests/valid_program.min
	@echo ""
	@echo "[TEST 2] Print + Parentheses"
	@./$(TARGET) < tests/print_program.min
	@echo ""
	@echo "[TEST 3] TAC Generation"
	@./$(TARGET) < tests/tac_basic.min
	@echo ""
	@echo "[TEST 4] Undefined Variable"
	@./$(TARGET) < tests/undefined_variable.min
	@echo ""
	@echo "[TEST 5] Duplicate Declaration"
	@./$(TARGET) < tests/duplicate_declaration.min
	@echo ""
	@echo "[TEST 6] Invalid Assignment"
	@./$(TARGET) < tests/invalid_assignment.min
	@echo ""
	@echo "[TEST 7] Syntax Error"
	@./$(TARGET) < tests/syntax_error.min
	@echo ""
	@echo "[TEST 8] Lexical Error"
	@./$(TARGET) < tests/lexical_error.min
	@echo ""
	@echo "[TEST 9] Runtime Error"
	@./$(TARGET) < tests/division_by_zero.min
	@echo ""
	@echo "========================================"
	@echo "          TEST SUITE COMPLETE"
	@echo "========================================"

clean:
	rm -f src/compiler
	rm -f src/parser.tab.c src/parser.tab.h
	rm -f src/lex.yy.c
	rm -f src/*.o