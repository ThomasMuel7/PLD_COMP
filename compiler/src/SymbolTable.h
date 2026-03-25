#pragma once
#include <map>
#include <string>
#include <vector>

struct VariableInfo
{
    int index;
    bool isUsed;
    bool isPointer;
    bool isArray;
    int arrayLength;
    int byteSize;
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
    std::vector<bool> paramIsPointer;
};

using SymbolTable = std::map<std::string, VariableInfo>;
using FunctionTable = std::map<std::string, FunctionInfo>;
