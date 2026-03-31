#pragma once
#include <map>
#include <string>
#include <vector>

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

using FunctionTable = std::map<std::string, FunctionInfo>;