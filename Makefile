CXX = g++

CXXFLAGS = -std=c++17

TARGET = src/compiler

all: $(TARGET)

$(TARGET): src/parser.tab.c src/lex.yy.c src/ast.cpp src/semantic.cpp src/semantic_analyzer.cpp
	$(CXX) $(CXXFLAGS) src/parser.tab.c src/lex.yy.c src/ast.cpp src/semantic.cpp src/semantic_analyzer.cpp -o $(TARGET)

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
	@echo "[TEST 2] Undefined Variable"
	@./$(TARGET) < tests/undefined_variable.min
	@echo ""
	@echo "[TEST 3] Duplicate Declaration"
	@./$(TARGET) < tests/duplicate_declaration.min
	@echo ""
	@echo "[TEST 4] Invalid Assignment"
	@./$(TARGET) < tests/invalid_assignment.min
	@echo ""
	@echo "[TEST 5] Syntax Error"
	@./$(TARGET) < tests/syntax_error.min
	@echo ""
	@echo "========================================"
	@echo "          TEST SUITE COMPLETE"
	@echo "========================================"

clean:
	rm -f src/compiler
	rm -f src/parser.tab.c src/parser.tab.h
	rm -f src/lex.yy.c
	rm -f src/*.o