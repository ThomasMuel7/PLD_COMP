#pragma once
#include "IR.h"
#include <string>
#include <vector>

/**  The class for a basic block */

/* A few important comments.
	 IRInstr has no jump instructions.
	 cmp_* instructions behaves as an arithmetic two-operand instruction (add or mult),
	  returning a boolean value (as an int)

	 Assembly jumps are generated as follows:
	 BasicBlock::gen_asm() first calls IRInstr::gen_asm() on all its instructions, and then
			if  exit_true  is a  nullptr,
			the epilogue is generated
		else if exit_false is a nullptr,
		  an unconditional jmp to the exit_true branch is generated
				else (we have two successors, hence a branch)
		  an instruction comparing the value of test_var_name to true is generated,
					followed by a conditional branch to the exit_false branch,
					followed by an unconditional branch to the exit_true branch
	 The attribute test_var_name itself is defined when converting
  the if, while, etc of the AST  to IR.

Possible optimization:
	 a cmp_* comparison instructions, if it is the last instruction of its block,
	   generates an actual assembly comparison
	   followed by a conditional jump to the exit_false branch
*/

class BasicBlock
{
public:
	BasicBlock(CFG *cfg, const std::string &entry_label);
	// void gen_asm(ostream &o); /**< x86 assembly code generation for this basic block (very simple) */

	void add_IRInstr(IRInstr::Operation op, std::vector<std::string> params);

	void add_exit(BasicBlock *exit_true, BasicBlock *exit_false);

	// No encapsulation whatsoever here. Feel free to do better.
	BasicBlock *exit_true;		   /**< pointer to the next basic block, true branch. If nullptr, return from procedure */
	BasicBlock *exit_false;		   /**< pointer to the next basic block, false branch. If null_ptr, the basic block ends with an unconditional jump */
	string label;				   /**< label of the BB, also will be the label in the generated code */
	CFG *cfg;					   /** < the CFG where this block belongs */
	std::vector<IRInstr *> instrs; /** < the instructions themselves. */
	std::string test_var_name;	   /** < when generating IR code for an if(expr) or while(expr) etc,
														store here the name of the variable that holds the value of expr */
protected:
};
