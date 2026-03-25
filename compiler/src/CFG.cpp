#include "CFG.h"

CFG::CFG(const std::string &functionName, bool isVoidReturn)
    : entry(nullptr), functionName(functionName), isVoidReturn(isVoidReturn) {}

void CFG::set_entry(BasicBlock *entry)
{
  this->entry = entry;
}
