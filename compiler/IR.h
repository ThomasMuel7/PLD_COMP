#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <initializer_list>
#include "CFG.h"

using namespace std;

class BasicBlock;

// Declarations from the parser -- replace with your own
//#include "type.h"
//#include "symbole.h"
class DefFonction;

using namespace std;

enum class Type { Int, Char };

class IRInstr {
public:
    typedef enum {
        ldconst,
        copy,
        add,
        sub,
        mul,
        div,
        mod,
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