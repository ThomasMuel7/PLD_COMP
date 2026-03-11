#include "IR.h"

void BasicBlock::add_IRInstr(IRInstr::Operation op, Type t,
                             vector<string> params) {
  instrs.push_back(new IRInstr(this, op, t, params));
}
