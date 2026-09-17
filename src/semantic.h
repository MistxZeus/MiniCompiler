#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <string>
#include <map>

class SymbolTable {
private:
    std::map<std::string, std::string> symbols;

public:
    bool declare(const std::string& name);

    bool exists(const std::string& name) const;

    void print() const;
};

#endif