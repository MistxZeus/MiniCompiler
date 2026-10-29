#ifndef TARGET_CODEGEN_H
#define TARGET_CODEGEN_H

#include "ast.h"

#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Emits GNU AT&T x86-64 assembly for the integer-only MiniLang language.
// The generated file can be assembled and linked by GCC on 64-bit Linux.
class TargetCodeGenerator {
    std::ostringstream out;
    std::unordered_map<const VariableDeclaration*, int> localSlots;
    std::unordered_set<const VariableDeclaration*> globalDeclarations;
    std::map<std::string, std::string> globalBindings;
    std::vector<std::map<std::string, std::string>> scopes;
    int nextSlot = 0;
    int stackDepth = 0;
    int labelCounter = 0;
    std::string returnLabel;
    std::string errorMessage;

    static std::string globalSymbol(const std::string& name);
    static std::string functionSymbol(const std::string& name);
    static int frameSizeForSlots(int slots);
    std::string newLabel(const std::string& prefix);
    void emit(const std::string& line);
    void collectLocalSlots(const Statement* statement, bool topLevel = false);
    std::string resolveVariable(const std::string& name);
    void emitExpression(const Expression* expression);
    void emitStatement(const Statement* statement);
    void emitCall(const CallExpr* call);
    void emitFunction(const FunctionDeclaration* function);
    void emitMain(const Program* program);

public:
    // Returns true and fills assembly on success; on unsupported/invalid input,
    // returns false and provides a diagnostic in error.
    bool generate(const Program* program, std::string& assembly, std::string& error);
};

#endif // TARGET_CODEGEN_H
