#pragma once
#include <vector>

#ifndef CFG_H
#define CFG_H

class BasicBlock;

<<<<<<< HEAD
class CFG {
 public:

  CFG();
  BasicBlock* entry;
  std::vector<BasicBlock*> blocks;
  void set_entry(BasicBlock* entry);
=======
class CFG
{
public:
  CFG();
  BasicBlock *entry;
  std::vector<BasicBlock *> blocks;
  void set_entry(BasicBlock *entry);
>>>>>>> 6e2b3bbbfee102d95899e4843c55c1b244133002
};

#endif
