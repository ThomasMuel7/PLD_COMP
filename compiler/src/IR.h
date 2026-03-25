#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <initializer_list>
#include "CFG.h"

using namespace std;

class BasicBlock;

// Declarations from the parser -- replace with your own
// #include "type.h"
// #include "symbole.h"
class DefFonction;

class IRInstr
{

public:
    /** The instructions themselves -- feel free to subclass instead */
    typedef enum
    {
        ldconst,
        copy,
        add,
        sub,
        mul,
        div,
        mod,
        addr,
        rmem,
        wmem,
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

    /**  constructor */
    IRInstr(BasicBlock *bb_, Operation op, vector<string> params);
    BasicBlock *getBasicBlock() const { return bb; }
    Operation getOp() const { return op; }
    vector<string> getParams() const { return params; }

    /** Actual code generation */
    // void gen_asm(ostream &o); /**< x86 assembly code generation for this IR instruction */

private:
    BasicBlock *bb; /**< The BB this instruction belongs to, which provides a pointer to the CFG this instruction belong to */
    Operation op;
    vector<string> params; /**< For 3-op instrs: d, x, y For ldconst: d, c For ret: x For call: 
        params[0] = function label / function name
        params[1] = destination variable for return value
        params[2...] = argument variables
    */
                           // if you subclass IRInstr, each IRInstr subclass has its parameters and the previous (very important) comment becomes useless: it would be a better design.
};
