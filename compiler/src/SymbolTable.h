#pragma once
#include <map>
#include <string>
#include <vector>

struct VariableInfo
{
    std::string name;
    int index;
    bool isUsed;
    int declLine;
};

using SymbolTable = std::map<std::string, VariableInfo>;
