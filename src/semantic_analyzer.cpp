#include "semantic_analyzer.h"
#include <iostream>

SemanticAnalyzer::SemanticAnalyzer() { scopes.emplace_back(); }
bool SemanticAnalyzer::declareVariable(const std::string& n) {
    auto& scope=scopes.back(); if(scope.count(n)) return false; scope[n]="int"; return true;
}
bool SemanticAnalyzer::variableExists(const std::string& n) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->count(n)) return true;
    }
    return false;
}
bool SemanticAnalyzer::analyze(const Program* program) {
    hasErrors=false; scopes.clear(); scopes.emplace_back(); functions.clear(); currentFunction.clear();
    if(!program) { hasErrors=true; return false; }
    for(const auto& s:program->statements) if(auto f=dynamic_cast<const FunctionDeclaration*>(s.get())) {
        if(functions.count(f->name)) { std::cout<<"Semantic Error: Function '"<<f->name<<"' is already declared.\n"; hasErrors=true; }
        else functions[f->name]=f;
    }
    // Analyze global statements first so function bodies can resolve global names.
    for(const auto& s:program->statements) if(!dynamic_cast<const FunctionDeclaration*>(s.get())) analyzeStatement(s.get());
    for(const auto& s:program->statements) if(auto f=dynamic_cast<const FunctionDeclaration*>(s.get())) {
        currentFunction=f->name; scopes.emplace_back();
        for(const auto& p:f->parameters) if(!declareVariable(p)) { std::cout<<"Semantic Error: Duplicate parameter '"<<p<<"' in function '"<<f->name<<"'.\n"; hasErrors=true; }
        analyzeStatement(f->body.get()); scopes.pop_back(); currentFunction.clear();
    }
    return !hasErrors;
}
void SemanticAnalyzer::analyzeStatement(const Statement* s) {
    if(auto d=dynamic_cast<const VariableDeclaration*>(s)) {
        analyzeExpression(d->value.get());
        if(!declareVariable(d->name)) { std::cout<<"Semantic Error: Variable '"<<d->name<<"' is already declared in this scope.\n"; hasErrors=true; }
    } else if(auto a=dynamic_cast<const Assignment*>(s)) {
        if(!variableExists(a->name)) { std::cout<<"Semantic Error: Variable '"<<a->name<<"' is not declared.\n"; hasErrors=true; }
        analyzeExpression(a->value.get());
    } else if(auto p=dynamic_cast<const PrintStatement*>(s)) analyzeExpression(p->value.get());
    else if(auto b=dynamic_cast<const BlockStatement*>(s)) { scopes.emplace_back(); for(const auto& x:b->statements) analyzeStatement(x.get()); scopes.pop_back(); }
    else if(auto i=dynamic_cast<const IfStatement*>(s)) { analyzeExpression(i->condition.get()); analyzeStatement(i->thenBlock.get()); if(i->elseBlock) analyzeStatement(i->elseBlock.get()); }
    else if(auto w=dynamic_cast<const WhileStatement*>(s)) { analyzeExpression(w->condition.get()); analyzeStatement(w->body.get()); }
    else if(auto r=dynamic_cast<const ReturnStatement*>(s)) { if(currentFunction.empty()) { std::cout<<"Semantic Error: 'return' is only valid inside a function.\n"; hasErrors=true; } analyzeExpression(r->value.get()); }
    else if(auto f=dynamic_cast<const FunctionDeclaration*>(s)) { (void)f; std::cout<<"Semantic Error: Nested function declarations are not supported.\n"; hasErrors=true; }
}
void SemanticAnalyzer::analyzeExpression(const Expression* e) {
    if(auto v=dynamic_cast<const VariableExpr*>(e)) {
        if(!variableExists(v->name)) { std::cout<<"Semantic Error: Variable '"<<v->name<<"' is not declared.\n"; hasErrors=true; }
    } else if(auto b=dynamic_cast<const BinaryExpr*>(e)) { analyzeExpression(b->left.get()); analyzeExpression(b->right.get()); }
    else if(auto u=dynamic_cast<const UnaryExpr*>(e)) analyzeExpression(u->operand.get());
    else if(auto c=dynamic_cast<const CallExpr*>(e)) {
        for(const auto& a:c->arguments) analyzeExpression(a.get());
        auto it=functions.find(c->name);
        if(it==functions.end()) { std::cout<<"Semantic Error: Function '"<<c->name<<"' is not declared.\n"; hasErrors=true; }
        else if(it->second->parameters.size()!=c->arguments.size()) { std::cout<<"Semantic Error: Function '"<<c->name<<"' expects "<<it->second->parameters.size()<<" argument(s), got "<<c->arguments.size()<<".\n"; hasErrors=true; }
    }
}
void SemanticAnalyzer::printSymbolTable() const {
    std::cout<<"\n=== SYMBOL TABLE (GLOBAL SCOPE) ===\n\nVariable       Type\n---------------------\n";
    if(scopes.empty()||scopes.front().empty()) { std::cout<<"No global variables declared.\n\n"; return; }
    for(const auto& e:scopes.front()) std::cout<<e.first<<std::string(e.first.size()<15?15-e.first.size():1,' ')<<e.second<<'\n';
    std::cout<<'\n';
}
