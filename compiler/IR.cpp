#include "IR.h"

<<<<<<< HEAD
IRInstr::IRInstr(BasicBlock *bb, Operation op, Type t, vector<string> params)
    : bb(bb), op(op), t(t), params(params) {}

=======
IRInstr::IRInstr(BasicBlock *bb, Operation op, vector<string> params)
    : bb(bb), op(op), params(params) {}
>>>>>>> 6e2b3bbbfee102d95899e4843c55c1b244133002
