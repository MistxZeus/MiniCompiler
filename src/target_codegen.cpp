#include "target_codegen.h"

#include <algorithm>
#include <cstddef>
#include <utility>

std::string TargetCodeGenerator::globalSymbol(const std::string& name) {
    return "__ml_global_" + name;
}

std::string TargetCodeGenerator::functionSymbol(const std::string& name) {
    return "__ml_function_" + name;
}

int TargetCodeGenerator::frameSizeForSlots(int slots) {
    const int bytes = slots * 8;
    return (bytes + 15) / 16 * 16;
}

std::string TargetCodeGenerator::newLabel(const std::string& prefix) {
    return ".Lml_" + prefix + "_" + std::to_string(++labelCounter);
}

void TargetCodeGenerator::emit(const std::string& line) {
    out << line << '\n';
}

void TargetCodeGenerator::collectLocalSlots(const Statement* statement, bool topLevel) {
    if (!statement) return;

    if (const auto* declaration = dynamic_cast<const VariableDeclaration*>(statement)) {
        if (topLevel && globalDeclarations.count(declaration)) return;
        localSlots[declaration] = ++nextSlot;
        return;
    }
    if (const auto* block = dynamic_cast<const BlockStatement*>(statement)) {
        for (const auto& child : block->statements) collectLocalSlots(child.get());
        return;
    }
    if (const auto* conditional = dynamic_cast<const IfStatement*>(statement)) {
        collectLocalSlots(conditional->thenBlock.get());
        if (conditional->elseBlock) collectLocalSlots(conditional->elseBlock.get());
        return;
    }
    if (const auto* loop = dynamic_cast<const WhileStatement*>(statement)) {
        collectLocalSlots(loop->body.get());
    }
}

std::string TargetCodeGenerator::resolveVariable(const std::string& name) {
    for (auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope) {
        const auto found = scope->find(name);
        if (found != scope->end()) return found->second;
    }
    errorMessage = "Target code generation: no storage location for variable '" + name + "'.";
    return "$0";
}

void TargetCodeGenerator::emitExpression(const Expression* expression) {
    if (!expression) {
        errorMessage = "Target code generation: encountered an empty expression.";
        emit("\txorl %eax, %eax");
        return;
    }

    if (const auto* number = dynamic_cast<const NumberExpr*>(expression)) {
        emit("\tmovl $" + std::to_string(number->value) + ", %eax");
        return;
    }
    if (const auto* variable = dynamic_cast<const VariableExpr*>(expression)) {
        emit("\tmovl " + resolveVariable(variable->name) + ", %eax");
        return;
    }
    if (const auto* unary = dynamic_cast<const UnaryExpr*>(expression)) {
        emitExpression(unary->operand.get());
        if (unary->op == "-") {
            emit("\tnegl %eax");
        } else if (unary->op == "!") {
            emit("\ttestl %eax, %eax");
            emit("\tsete %al");
            emit("\tmovzbl %al, %eax");
        } else {
            errorMessage = "Target code generation: unsupported unary operator '" + unary->op + "'.";
        }
        return;
    }
    if (const auto* call = dynamic_cast<const CallExpr*>(expression)) {
        emitCall(call);
        return;
    }
    if (const auto* binary = dynamic_cast<const BinaryExpr*>(expression)) {
        const std::string op = binary->op;
        if (op == "&&" || op == "||") {
            const std::string shortLabel = newLabel(op == "&&" ? "and_false" : "or_true");
            const std::string endLabel = newLabel("logic_end");
            emitExpression(binary->left.get());
            emit("\ttestl %eax, %eax");
            emit(std::string("\t") + (op == "&&" ? "je " : "jne ") + shortLabel);
            emitExpression(binary->right.get());
            emit("\ttestl %eax, %eax");
            emit("\tsetne %al");
            emit("\tmovzbl %al, %eax");
            emit("\tjmp " + endLabel);
            emit(shortLabel + ":");
            emit(std::string("\tmovl $") + (op == "&&" ? "0" : "1") + ", %eax");
            emit(endLabel + ":");
            return;
        }

        emitExpression(binary->left.get());
        emit("\tpushq %rax");
        stackDepth += 8;
        emitExpression(binary->right.get());
        emit("\tmovl %eax, %ecx");
        emit("\tpopq %rax");
        stackDepth -= 8;

        if (op == "+") emit("\taddl %ecx, %eax");
        else if (op == "-") emit("\tsubl %ecx, %eax");
        else if (op == "*") emit("\timull %ecx, %eax");
        else if (op == "/") {
            const std::string safeLabel = newLabel("div_safe");
            emit("\ttestl %ecx, %ecx");
            emit("\tjne " + safeLabel);
            // Avoid a processor trap with a deterministic non-zero process exit.
            // This path is intentionally small; the interpreter provides the
            // richer runtime diagnostic when running through the default mode.
            emit("\tmovl $1, %edi");
            const int exitPadding = (16 - (stackDepth % 16)) % 16;
            if (exitPadding) emit("\tsubq $" + std::to_string(exitPadding) + ", %rsp");
            emit("\tcall exit@PLT");
            if (exitPadding) emit("\taddq $" + std::to_string(exitPadding) + ", %rsp");
            emit(safeLabel + ":");
            emit("\tcltd");
            emit("\tidivl %ecx");
        } else {
            emit("\tcmpl %ecx, %eax");
            if (op == "<") emit("\tsetl %al");
            else if (op == ">") emit("\tsetg %al");
            else if (op == "<=") emit("\tsetle %al");
            else if (op == ">=") emit("\tsetge %al");
            else if (op == "==") emit("\tsete %al");
            else if (op == "!=") emit("\tsetne %al");
            else errorMessage = "Target code generation: unsupported binary operator '" + op + "'.";
            emit("\tmovzbl %al, %eax");
        }
        return;
    }

    errorMessage = "Target code generation: unknown expression node.";
    emit("\txorl %eax, %eax");
}

void TargetCodeGenerator::emitCall(const CallExpr* call) {
    // Save evaluated arguments in stack slots. This preserves source evaluation
    // order and lets nested calls use the same caller-saved registers safely.
    const std::size_t count = call->arguments.size();
    for (const auto& argument : call->arguments) {
        emitExpression(argument.get());
        emit("\tpushq %rax");
        stackDepth += 8;
    }

    const std::size_t stackArgCount = count > 6 ? count - 6 : 0;
    const int outgoingBytes = static_cast<int>(stackArgCount * 8);
    const int padding = (16 - ((stackDepth + outgoingBytes) % 16)) % 16;
    const int reserveBytes = outgoingBytes + padding;
    if (reserveBytes > 0) {
        emit("\tsubq $" + std::to_string(reserveBytes) + ", %rsp");
        stackDepth += reserveBytes;
    }

    // The original argument array begins above the outgoing stack-argument area.
    // SysV AMD64 passes integer arguments 1-6 in registers and later arguments
    // on the stack, with argument 7 at the top of the outgoing area.
    static const char* argRegisters[] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
    for (std::size_t i = 6; i < count; ++i) {
        const std::size_t sourceOffset = static_cast<std::size_t>(reserveBytes) + 8 * (count - 1 - i);
        const std::size_t destinationOffset = 8 * (i - 6);
        emit("\tmovl " + std::to_string(sourceOffset) + "(%rsp), %r10d");
        emit("\tmovl %r10d, " + std::to_string(destinationOffset) + "(%rsp)");
    }
    const std::size_t registerCount = std::min<std::size_t>(count, 6);
    for (std::size_t i = 0; i < registerCount; ++i) {
        const std::size_t sourceOffset = static_cast<std::size_t>(reserveBytes) + 8 * (count - 1 - i);
        emit("\tmovl " + std::to_string(sourceOffset) + "(%rsp), " + argRegisters[i]);
    }

    emit("\tcall " + functionSymbol(call->name));
    const int cleanup = reserveBytes + static_cast<int>(count * 8);
    if (cleanup > 0) {
        emit("\taddq $" + std::to_string(cleanup) + ", %rsp");
        stackDepth -= cleanup;
    }
}

void TargetCodeGenerator::emitStatement(const Statement* statement) {
    if (!statement) return;
    if (const auto* declaration = dynamic_cast<const VariableDeclaration*>(statement)) {
        emitExpression(declaration->value.get());
        std::string destination;
        if (globalDeclarations.count(declaration)) {
            destination = globalSymbol(declaration->name) + "(%rip)";
            if (!scopes.empty()) scopes.front()[declaration->name] = destination;
        } else {
            const auto slot = localSlots.find(declaration);
            if (slot == localSlots.end()) {
                errorMessage = "Target code generation: missing stack slot for '" + declaration->name + "'.";
                return;
            }
            destination = "-" + std::to_string(slot->second * 8) + "(%rbp)";
            if (scopes.empty()) scopes.emplace_back();
            scopes.back()[declaration->name] = destination;
        }
        emit("\tmovl %eax, " + destination);
        return;
    }
    if (const auto* assignment = dynamic_cast<const Assignment*>(statement)) {
        emitExpression(assignment->value.get());
        emit("\tmovl %eax, " + resolveVariable(assignment->name));
        return;
    }
    if (const auto* print = dynamic_cast<const PrintStatement*>(statement)) {
        emitExpression(print->value.get());
        emit("\tmovl %eax, %esi");
        emit("\tleaq .Lml_print_format(%rip), %rdi");
        emit("\txorl %eax, %eax");
        // The expression has balanced its temporary stack usage, so stackDepth
        // reflects only any surrounding expression evaluation (normally zero).
        const int padding = (16 - (stackDepth % 16)) % 16;
        if (padding) { emit("\tsubq $" + std::to_string(padding) + ", %rsp"); stackDepth += padding; }
        emit("\tcall printf@PLT");
        if (padding) { emit("\taddq $" + std::to_string(padding) + ", %rsp"); stackDepth -= padding; }
        return;
    }
    if (const auto* block = dynamic_cast<const BlockStatement*>(statement)) {
        scopes.emplace_back();
        for (const auto& child : block->statements) emitStatement(child.get());
        scopes.pop_back();
        return;
    }
    if (const auto* conditional = dynamic_cast<const IfStatement*>(statement)) {
        const std::string otherwiseLabel = newLabel("if_else");
        const std::string endLabel = newLabel("if_end");
        emitExpression(conditional->condition.get());
        emit("\ttestl %eax, %eax");
        emit("\tje " + otherwiseLabel);
        emitStatement(conditional->thenBlock.get());
        if (conditional->elseBlock) {
            emit("\tjmp " + endLabel);
            emit(otherwiseLabel + ":");
            emitStatement(conditional->elseBlock.get());
            emit(endLabel + ":");
        } else {
            emit(otherwiseLabel + ":");
        }
        return;
    }
    if (const auto* loop = dynamic_cast<const WhileStatement*>(statement)) {
        const std::string startLabel = newLabel("while_start");
        const std::string endLabel = newLabel("while_end");
        emit(startLabel + ":");
        emitExpression(loop->condition.get());
        emit("\ttestl %eax, %eax");
        emit("\tje " + endLabel);
        emitStatement(loop->body.get());
        emit("\tjmp " + startLabel);
        emit(endLabel + ":");
        return;
    }
    if (const auto* ret = dynamic_cast<const ReturnStatement*>(statement)) {
        emitExpression(ret->value.get());
        if (returnLabel.empty()) {
            errorMessage = "Target code generation: return statement outside a function.";
        } else {
            emit("\tjmp " + returnLabel);
        }
        return;
    }
    if (dynamic_cast<const FunctionDeclaration*>(statement)) {
        // Functions are emitted separately and are not executed as top-level statements.
        return;
    }
    errorMessage = "Target code generation: unknown statement node.";
}

void TargetCodeGenerator::emitFunction(const FunctionDeclaration* function) {
    nextSlot = static_cast<int>(function->parameters.size());
    for (const auto& parameter : function->parameters) {
        (void)parameter;
        // Parameter i receives stack slot i+1; each slot is 8 bytes for alignment.
    }
    localSlots.clear();
    collectLocalSlots(function->body.get());
    const int frameBytes = frameSizeForSlots(nextSlot);
    stackDepth = 0;
    returnLabel = newLabel("return_" + function->name);

    emit("\n.globl " + functionSymbol(function->name));
    emit(".type " + functionSymbol(function->name) + ", @function");
    emit(functionSymbol(function->name) + ":");
    emit("\tpushq %rbp");
    emit("\tmovq %rsp, %rbp");
    if (frameBytes) emit("\tsubq $" + std::to_string(frameBytes) + ", %rsp");

    scopes.clear();
    scopes.emplace_back(globalBindings);
    std::map<std::string, std::string> parameterScope;
    static const char* argRegisters[] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
    for (std::size_t i = 0; i < function->parameters.size(); ++i) {
        const int offset = -static_cast<int>((i + 1) * 8);
        parameterScope[function->parameters[i]] = "-" + std::to_string((i + 1) * 8) + "(%rbp)";
        if (i < 6) {
            emit("\tmovl " + std::string(argRegisters[i]) + ", " + parameterScope[function->parameters[i]]);
        } else {
            emit("\tmovl " + std::to_string(16 + (i - 6) * 8) + "(%rbp), %eax");
            emit("\tmovl %eax, " + parameterScope[function->parameters[i]]);
        }
        (void)offset;
    }
    scopes.push_back(std::move(parameterScope));
    emitStatement(function->body.get());
    emit("\txorl %eax, %eax"); // A function that falls through returns zero.
    emit(returnLabel + ":");
    emit("\tleave");
    emit("\tret");
    emit(".size " + functionSymbol(function->name) + ", .-" + functionSymbol(function->name));
    returnLabel.clear();
}

void TargetCodeGenerator::emitMain(const Program* program) {
    nextSlot = 0;
    localSlots.clear();
    for (const auto& statement : program->statements) {
        if (!dynamic_cast<const FunctionDeclaration*>(statement.get())) {
            collectLocalSlots(statement.get(), true);
        }
    }
    const int frameBytes = frameSizeForSlots(nextSlot);
    stackDepth = 0;
    returnLabel.clear();

    emit("\n.globl main");
    emit(".type main, @function");
    emit("main:");
    emit("\tpushq %rbp");
    emit("\tmovq %rsp, %rbp");
    if (frameBytes) emit("\tsubq $" + std::to_string(frameBytes) + ", %rsp");

    scopes.clear();
    scopes.emplace_back(globalBindings);
    for (const auto& statement : program->statements) {
        if (!dynamic_cast<const FunctionDeclaration*>(statement.get())) emitStatement(statement.get());
    }
    emit("\txorl %eax, %eax");
    emit("\tleave");
    emit("\tret");
    emit(".size main, .-main");
}

bool TargetCodeGenerator::generate(const Program* program, std::string& assembly, std::string& error) {
    out.str("");
    out.clear();
    localSlots.clear();
    globalDeclarations.clear();
    globalBindings.clear();
    scopes.clear();
    nextSlot = 0;
    stackDepth = 0;
    labelCounter = 0;
    returnLabel.clear();
    errorMessage.clear();

    if (!program) {
        error = "Target code generation: no parsed program was supplied.";
        return false;
    }

    // Top-level declarations have static storage; declarations inside blocks and
    // functions receive stack slots unique to each declaration node.
    for (const auto& statement : program->statements) {
        if (const auto* declaration = dynamic_cast<const VariableDeclaration*>(statement.get())) {
            globalDeclarations.insert(declaration);
            globalBindings[declaration->name] = globalSymbol(declaration->name) + "(%rip)";
        }
    }

    emit("# MiniLang target code generated by MiniCompiler");
    emit("# Target: x86-64 System V ABI (GNU assembler syntax)");
    emit(".section .rodata");
    emit(".Lml_print_format:");
    emit("\t.string \"%d\\n\"");
    for (const auto& entry : globalBindings) {
        emit(".comm " + globalSymbol(entry.first) + ", 4, 4");
    }
    emit(".text");
    emit(".extern printf");
    emit(".extern exit");

    for (const auto& statement : program->statements) {
        if (const auto* function = dynamic_cast<const FunctionDeclaration*>(statement.get())) {
            emitFunction(function);
        }
    }
    emitMain(program);
    emit("\n.section .note.GNU-stack,\"\",@progbits");

    if (!errorMessage.empty()) {
        error = errorMessage;
        return false;
    }
    assembly = out.str();
    error.clear();
    return true;
}
