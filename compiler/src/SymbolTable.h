#pragma once
#include <map>
#include <string>
#include <vector>

struct VariableInfo
{
    int index;
    bool isUsed;
    int declLine;
};

enum class ReturnType
{
    Int,
    Void
};

struct FunctionInfo
{
    ReturnType returnType;
    int arity;
    std::vector<std::string> paramUniqueNames;
};

using SymbolTable = std::map<std::string, VariableInfo>;
using FunctionTable = std::map<std::string, FunctionInfo>;
