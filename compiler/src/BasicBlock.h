#pragma once
#include "IR.h"
#include <string>
#include <vector>

class BasicBlock
{
public:
	BasicBlock(CFG *cfg, const std::string &entry_label);

	void add_IRInstr(IRInstr::Operation op, std::vector<std::string> params);

	void add_exit(BasicBlock *exit_true, BasicBlock *exit_false = nullptr);

	BasicBlock *exit_true;
	BasicBlock *exit_false;
	string label;
	CFG *cfg;
	std::vector<IRInstr *> instrs;
	std::string test_var_name;
};
