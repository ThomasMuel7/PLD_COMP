#include "BasicBlock.h"
#include "CFG.h"
#include "IR.h"

BasicBlock::BasicBlock(CFG *cfg, const string &label) : cfg(cfg), label(label)
{
  cfg->blocks.push_back(this);
}

void BasicBlock::add_IRInstr(IRInstr::Operation op,
                             vector<string> params)
{
  instrs.push_back(new IRInstr(this, op, params));
}
void BasicBlock::add_exit(BasicBlock *exit_true, BasicBlock *exit_false)
{
  this->exit_true = exit_true;
  this->exit_false = exit_false;
}
