#pragma once
#include <vector>

#ifndef CFG_H
#define CFG_H

class BasicBlock;

class CFG
{
public:
  CFG();
  BasicBlock *entry;
  std::vector<BasicBlock *> blocks;
  void set_entry(BasicBlock *entry);
};

#endif
