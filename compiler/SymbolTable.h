#pragma once
#include <map>
#include <string>

struct VariableInfo {
    int index;
    bool isUsed; 
};

using SymbolTable = std::map<std::string, VariableInfo>;