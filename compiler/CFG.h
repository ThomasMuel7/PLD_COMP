#pragma once
#include <vector>

class BasicBlock;

class CFG
{
public:
  CFG();
  BasicBlock *entry;
  std::vector<BasicBlock *> blocks;
  void set_entry(BasicBlock *entry);
};