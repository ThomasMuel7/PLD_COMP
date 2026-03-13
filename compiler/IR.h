#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <initializer_list>
#include "CFG.h"

using namespace std;

class BasicBlock;

// Declarations from the parser -- replace with your own
<<<<<<< HEAD
//#include "type.h"
//#include "symbole.h"
class DefFonction;

using namespace std;

enum class Type { Int, Char };

class IRInstr {
public:
    typedef enum {
=======
// #include "type.h"
// #include "symbole.h"
class DefFonction;

class IRInstr
{

public:
    /** The instructions themselves -- feel free to subclass instead */
    typedef enum
    {
>>>>>>> 6e2b3bbbfee102d95899e4843c55c1b244133002
        ldconst,
        copy,
        add,
        sub,
        mul,
        div,
        mod,
        rmem,
        wmem,
<<<<<<< HEAD
        call, 
=======
        call,
>>>>>>> 6e2b3bbbfee102d95899e4843c55c1b244133002
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

<<<<<<< HEAD
    IRInstr(BasicBlock* bb_, Operation op, Type t, vector<string> params);
    void gen_asm(ostream &o); 
    
    // Attributs accessibles pour le backend
    BasicBlock* bb; 
    Operation op;
    Type t;
    vector<string> params; 
};

class BasicBlock {
public:
    BasicBlock(CFG* cfg, string entry_label);
    void gen_asm(ostream &o); 
    void add_IRInstr(IRInstr::Operation op, Type t, vector<string> params);

    BasicBlock* exit_true;  
    BasicBlock* exit_false; 
    string label; 
    CFG* cfg; 
    vector<IRInstr*> instrs; 
    string test_var_name;  
};

class CFG {
public:
    CFG();
    BasicBlock* entry;
    vector<BasicBlock*> blocks;
};
=======
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
    vector<string> params; /**< For 3-op instrs: d, x, y; for ldconst: d, c;  For call: label, d, params;  for wmem and rmem: choose yourself */
                           // if you subclass IRInstr, each IRInstr subclass has its parameters and the previous (very important) comment becomes useless: it would be a better design.
};
>>>>>>> 6e2b3bbbfee102d95899e4843c55c1b244133002
