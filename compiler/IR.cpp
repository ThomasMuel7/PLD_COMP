#include "IR.h"

IRInstr::IRInstr(BasicBlock *bb, Operation op, Type t, vector<string> params)
    : bb(bb), op(op), t(t), params(params) {}

