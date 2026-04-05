#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <initializer_list>
#include "CFG.h"

using namespace std;

class BasicBlock;

class DefFonction;

class IRInstr
{

public:
    typedef enum
    {
        ldconst,
        copy,
        add,
        sub,
        mul,
        div,
        mod,
        call, 
        cmp_eq,
        cmp_lt,
        cmp_le,
        cmp_gt,
        cmp_ge,
        cmp_ne,
        and_,
        or_,
        xor_,
        neg,
        not_,
        ret
    } Operation;

    IRInstr(BasicBlock *bb_, Operation op, vector<string> params);
    BasicBlock *getBasicBlock() const { return bb; }
    Operation getOp() const { return op; }
    vector<string> getParams() const { return params; }

private:
    BasicBlock *bb;
    Operation op;
    vector<string> params;
};
