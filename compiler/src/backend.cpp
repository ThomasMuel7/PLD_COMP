#include "backend.h"
#include <iostream>

using namespace std;

x86Backend::x86Backend(const vector<CFG *> &cfgs, const SymbolTable &symbolTable)
    : backend(cfgs, symbolTable) {}

string x86Backend::getOffset(const string &varName)
{
  auto it = symbolTable.find(varName);
  if (it != symbolTable.end())
  {
    return to_string(it->second.index) + "(%rbp)";
  }
  return "0(%rbp)";
}

string x86Backend::loadBinaryOperands(IRInstr *instr)
{
  string code = "    movl " + getOffset(instr->getParams()[1]) + ", %eax\n";
  code += "    movl " + getOffset(instr->getParams()[2]) + ", %ebx\n";
  return code;
}

string x86Backend::saveResultEax(IRInstr *instr)
{
  return "    movl %eax, " + getOffset(instr->getParams()[0]) + "\n";
}

void x86Backend::translate()
{
  #ifdef __APPLE__
    cout << ".globl _main\n";
  #else
    cout << ".globl main\n";
  #endif

  for (CFG *cfg : cfgs)
  {
    for (BasicBlock *bb : cfg->blocks)
    {
      if (bb->label == "prologue") {
        #ifdef __APPLE__
          cout << " _main:\n";
        #else
          cout << " main:\n";
        #endif
        cout << "    pushq %rbp\n";
        cout << "    movq %rsp, %rbp\n";
      } 
      else if (bb->label == "epilogue") {
        cout << bb->label << ":\n";
        cout << "    popq %rbp\n";
        cout << "    ret\n";
        continue;
      } 
      else {
        cout << bb->label << ":\n";
        for (IRInstr *instr : bb->instrs)
        {
          cout << generate(instr);
        }
      }
      if (bb->exit_true != nullptr && bb->exit_false != nullptr)
      {
        cout << "    cmpl $0, " << getOffset(bb->test_var_name) << "\n";
        cout << "    je " << bb->exit_false->label << "\n";
        cout << "    jmp " << bb->exit_true->label << "\n";
      }
      else if (bb->exit_true != nullptr)
      {
        cout << "    jmp " << bb->exit_true->label << "\n";
      }
    }
  }
}

string x86Backend::generate(IRInstr *instr)
{
  string code = "";
  switch (instr->getOp())
  {
  case IRInstr::ldconst:
    code += "    movl $" + instr->getParams()[1] + ", " + getOffset(instr->getParams()[0]) + "\n";
    break;
  case IRInstr::copy:
    code += "    movl " + getOffset(instr->getParams()[1]) + ", %eax\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::add:
    code += loadBinaryOperands(instr);
    code += "    addl %ebx, %eax\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::sub:
    code += loadBinaryOperands(instr);
    code += "    subl %ebx, %eax\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::mul:
    code += loadBinaryOperands(instr);
    code += "    imull %ebx, %eax\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::div:
    code += loadBinaryOperands(instr);
    code += "    cltd\n";
    code += "    idivl %ebx\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::mod:
    code += loadBinaryOperands(instr);
    code += "    cltd\n";
    code += "    idivl %ebx\n";
    code += "    movl %edx, " + getOffset(instr->getParams()[0]) + "\n";
    break;
  case IRInstr::and_:
    code += loadBinaryOperands(instr);
    code += "    andl %ebx, %eax\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::or_:
    code += loadBinaryOperands(instr);
    code += "    orl %ebx, %eax\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::xor_:
    code += loadBinaryOperands(instr);
    code += "    xorl %ebx, %eax\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::neg:
    code += "    movl " + getOffset(instr->getParams()[1]) + ", %eax\n";
    code += "    negl %eax\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::not_:
    code += "    movl " + getOffset(instr->getParams()[1]) + ", %eax\n";
    code += "    cmpl $0, %eax\n";
    code += "    sete %al\n";
    code += "    movzbl %al, %eax\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::cmp_eq:
  case IRInstr::cmp_ne:
  case IRInstr::cmp_lt:
  case IRInstr::cmp_le:
  case IRInstr::cmp_gt:
  case IRInstr::cmp_ge:
    code += loadBinaryOperands(instr);
    code += "    cmpl %ebx, %eax\n";
    if (instr->getOp() == IRInstr::cmp_eq)
      code += "    sete %al\n";
    else if (instr->getOp() == IRInstr::cmp_ne)
      code += "    setne %al\n";
    else if (instr->getOp() == IRInstr::cmp_lt)
      code += "    setl %al\n";
    else if (instr->getOp() == IRInstr::cmp_le)
      code += "    setle %al\n";
    else if (instr->getOp() == IRInstr::cmp_gt)
      code += "    setg %al\n";
    else if (instr->getOp() == IRInstr::cmp_ge)
      code += "    setge %al\n";
    code += "    movzbl %al, %eax\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::ret:
    code += "    movl " + getOffset(instr->getParams()[0]) + ", %eax\n";
    break;
  default:
    break;
  }
  return code;
}