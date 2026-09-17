#include "semantic.h"
#include <iostream>

using namespace std;

bool SymbolTable::declare(const string& name)
{
    if (symbols.find(name) != symbols.end())
    {
        return false;
    }

    symbols[name] = "int";
    return true;
}

bool SymbolTable::exists(const string& name) const
{
    return symbols.find(name) != symbols.end();
}

void SymbolTable::print() const
{
    cout << "\n=== SYMBOL TABLE ===\n\n";

    cout << "Variable       Type" << endl;
    cout << "---------------------" << endl;

    if (symbols.empty())
    {
        cout << "No variables declared." << endl;
        return;
    }

    for (const auto& entry : symbols)
    {
        cout << entry.first;

        // Formatting
        for (size_t i = entry.first.length(); i < 15; i++)
        {
            cout << ' ';
        }

        cout << entry.second << endl;
    }

    cout << endl;
}