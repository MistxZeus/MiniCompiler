#include "ast.h"
#include <iostream>

namespace {
void indent(int depth) { while (depth--) std::cout << "  "; }
void printExpr(const Expression* e, int d) {
    indent(d);
    if (auto n = dynamic_cast<const NumberExpr*>(e)) std::cout << "Number: " << n->value << '\n';
    else if (auto v = dynamic_cast<const VariableExpr*>(e)) std::cout << "Variable: " << v->name << '\n';
    else if (auto b = dynamic_cast<const BinaryExpr*>(e)) { std::cout << "Binary: " << b->op << '\n'; printExpr(b->left.get(), d+1); printExpr(b->right.get(), d+1); }
    else if (auto u = dynamic_cast<const UnaryExpr*>(e)) { std::cout << "Unary: " << u->op << '\n'; printExpr(u->operand.get(), d+1); }
    else if (auto c = dynamic_cast<const CallExpr*>(e)) { std::cout << "Call: " << c->name << '\n'; for (const auto& a : c->arguments) printExpr(a.get(), d+1); }
    else std::cout << "<unknown expression>\n";
}
void printStmt(const Statement* s, int d) {
    indent(d);
    if (auto n = dynamic_cast<const VariableDeclaration*>(s)) { std::cout << "Declare: " << n->name << '\n'; printExpr(n->value.get(),d+1); }
    else if (auto a = dynamic_cast<const Assignment*>(s)) { std::cout << "Assign: " << a->name << '\n'; printExpr(a->value.get(),d+1); }
    else if (auto p = dynamic_cast<const PrintStatement*>(s)) { std::cout << "Print\n"; printExpr(p->value.get(),d+1); }
    else if (auto b = dynamic_cast<const BlockStatement*>(s)) { std::cout << "Block\n"; for (const auto& x:b->statements) printStmt(x.get(),d+1); }
    else if (auto i = dynamic_cast<const IfStatement*>(s)) { std::cout << "If\n"; printExpr(i->condition.get(),d+1); printStmt(i->thenBlock.get(),d+1); if(i->elseBlock) printStmt(i->elseBlock.get(),d+1); }
    else if (auto w = dynamic_cast<const WhileStatement*>(s)) { std::cout << "While\n"; printExpr(w->condition.get(),d+1); printStmt(w->body.get(),d+1); }
    else if (auto r = dynamic_cast<const ReturnStatement*>(s)) { std::cout << "Return\n"; printExpr(r->value.get(),d+1); }
    else if (auto f = dynamic_cast<const FunctionDeclaration*>(s)) { std::cout << "Function: " << f->name << '('; for(size_t i=0;i<f->parameters.size();++i){if(i)std::cout<<", ";std::cout<<f->parameters[i];} std::cout << ")\n"; printStmt(f->body.get(),d+1); }
    else std::cout << "<unknown statement>\n";
}
}
void printAST(const Program* p) {
    std::cout << "\n=== ABSTRACT SYNTAX TREE ===\n\nProgram\n";
    if (p) for (const auto& s : p->statements) printStmt(s.get(),1);
    std::cout << '\n';
}
