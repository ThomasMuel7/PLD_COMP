#include "IR.h"

IRInstr::IRInstr(BasicBlock *bb, Operation op, vector<string> params)
    : bb(bb), op(op), params(params) {}
