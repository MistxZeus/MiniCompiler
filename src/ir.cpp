#include "ir.h"
#include <iostream>
std::string IRGenerator::newTemp(){return "t"+std::to_string(++tempCount);}
std::string IRGenerator::newLabel(){return "L"+std::to_string(++labelCount);}
std::string IRGenerator::generateExpression(const Expression* e){
 if(auto n=dynamic_cast<const NumberExpr*>(e))return std::to_string(n->value);
 if(auto v=dynamic_cast<const VariableExpr*>(e))return v->name;
 if(auto u=dynamic_cast<const UnaryExpr*>(e)){auto x=generateExpression(u->operand.get());auto t=newTemp();instructions.push_back(t+" = "+u->op+x);return t;}
 if(auto c=dynamic_cast<const CallExpr*>(e)){for(const auto& a:c->arguments)instructions.push_back("PARAM "+generateExpression(a.get()));auto t=newTemp();instructions.push_back(t+" = CALL "+c->name+", "+std::to_string(c->arguments.size()));return t;}
 if(auto b=dynamic_cast<const BinaryExpr*>(e)){
  if(b->op=="&&"||b->op=="||") {auto t=newTemp(), shortLabel=newLabel(), endLabel=newLabel();auto l=generateExpression(b->left.get());instructions.push_back(t+" = "+(b->op=="&&"?"0":"1"));instructions.push_back(std::string(b->op=="&&"?"IF_FALSE ":"IF_TRUE ")+l+" GOTO "+shortLabel);auto r=generateExpression(b->right.get());instructions.push_back(std::string(b->op=="&&"?"IF_FALSE ":"IF_TRUE ")+r+" GOTO "+shortLabel);instructions.push_back(t+" = "+(b->op=="&&"?"1":"0"));instructions.push_back("GOTO "+endLabel);instructions.push_back("LABEL "+shortLabel);instructions.push_back("LABEL "+endLabel);return t;}
  auto l=generateExpression(b->left.get()),r=generateExpression(b->right.get()),t=newTemp();instructions.push_back(t+" = "+l+" "+b->op+" "+r);return t;
 }
 return "0";
}
void IRGenerator::generateStatement(const Statement* s){
 if(auto d=dynamic_cast<const VariableDeclaration*>(s)){instructions.push_back(d->name+" = "+generateExpression(d->value.get()));}
 else if(auto a=dynamic_cast<const Assignment*>(s)){instructions.push_back(a->name+" = "+generateExpression(a->value.get()));}
 else if(auto p=dynamic_cast<const PrintStatement*>(s)){instructions.push_back("PRINT "+generateExpression(p->value.get()));}
 else if(auto b=dynamic_cast<const BlockStatement*>(s)){for(const auto& x:b->statements)generateStatement(x.get());}
 else if(auto i=dynamic_cast<const IfStatement*>(s)){auto cond=generateExpression(i->condition.get()),otherwise=newLabel(),end=newLabel();instructions.push_back("IF_FALSE "+cond+" GOTO "+otherwise);generateStatement(i->thenBlock.get());if(i->elseBlock){instructions.push_back("GOTO "+end);instructions.push_back("LABEL "+otherwise);generateStatement(i->elseBlock.get());instructions.push_back("LABEL "+end);}else instructions.push_back("LABEL "+otherwise);}
 else if(auto w=dynamic_cast<const WhileStatement*>(s)){auto start=newLabel(),end=newLabel();instructions.push_back("LABEL "+start);auto cond=generateExpression(w->condition.get());instructions.push_back("IF_FALSE "+cond+" GOTO "+end);generateStatement(w->body.get());instructions.push_back("GOTO "+start);instructions.push_back("LABEL "+end);}
 else if(auto r=dynamic_cast<const ReturnStatement*>(s)){instructions.push_back("RETURN "+generateExpression(r->value.get()));}
 else if(auto f=dynamic_cast<const FunctionDeclaration*>(s)){instructions.push_back("FUNC "+f->name);for(const auto& p:f->parameters)instructions.push_back("PARAM_DECL "+p);generateStatement(f->body.get());instructions.push_back("END_FUNC "+f->name);}
}
void IRGenerator::generate(const Program* p){instructions.clear();tempCount=labelCount=0;if(p)for(const auto& s:p->statements)generateStatement(s.get());}
void IRGenerator::printTAC()const{std::cout<<"\n=== THREE-ADDRESS CODE ===\n\n";if(instructions.empty())std::cout<<"No intermediate code generated.\n";else for(const auto& i:instructions)std::cout<<i<<'\n';std::cout<<'\n';}
