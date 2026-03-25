#pragma once
#include <vector>
#include <string>

class BasicBlock;

class CFG
{
public:
  CFG(const std::string &functionName = "main", bool isVoidReturn = false);
  BasicBlock *entry;
  std::vector<BasicBlock *> blocks;
  std::string functionName;
  bool isVoidReturn;
  std::vector<std::string> paramVarNames;
  void set_entry(BasicBlock *entry);
};