#include "backend.h"
#include <iostream>

using namespace std;

x86Backend::x86Backend(const vector<CFG *> &cfgs, const SymbolTable &symbolTable)
    : backend(cfgs, symbolTable) {}

string x86Backend::generatePrologue()
{
  string code = "";
  #ifdef __APPLE__
    code += ".globl _main\n";
    cout << " _main: \n";
  #else
    code += ".globl main\n";
    cout << " main: \n";
  #endif
  cout << "    pushq %rbp\n";
  cout << "    movq %rsp, %rbp\n";
  return code;
}

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
  cout << generatePrologue();
  for (CFG *cfg : cfgs)
  {
    for (BasicBlock *bb : cfg->blocks)
    {
      for (IRInstr *instr : bb->instrs)
      {
        cout << generate(instr);
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
    code += "    popq %rbp\n";
    code += "    ret\n";
    break;
  default:
    break;
  }
  return code;
}



// ===================== AARCH64 BACKEND =====================


ArmBackend::ArmBackend(const vector<CFG *> &cfgs, const SymbolTable &symbolTable)
    : backend(cfgs, symbolTable) {}

int ArmBackend::computeFrameSize() const
{
  int numVars = (int)symbolTable.size();
  int size = numVars * 4;
  if (size % 16 != 0) size += 16 - (size % 16);
  return size;
}

string ArmBackend::generatePrologue()
{
  int frameSize = computeFrameSize();

  string code = ".text\n";
#ifdef __APPLE__
  code += ".globl _main\n";
  code += "_main:\n";
#else
  code += ".globl main\n";
  code += "main:\n";
#endif
  code += "    sub sp, sp, #" + to_string(frameSize) + "\n";
  return code;
}

string ArmBackend::getOffset(const string &varName)
{
  auto it = symbolTable.find(varName);
  if (it != symbolTable.end())
  {
    int frameSize = computeFrameSize();
    int spOffset = frameSize + it->second.index;
    return to_string(spOffset);
  }
  return "0";
}

string ArmBackend::loadBinaryOperands(IRInstr *instr)
{
  string code = "    ldr w0, [sp, #" + getOffset(instr->getParams()[1]) + "]\n";
  code        += "    ldr w1, [sp, #" + getOffset(instr->getParams()[2]) + "]\n";
  return code;
}

string ArmBackend::saveResultEax(IRInstr *instr)
{
  return "    str w0, [sp, #" + getOffset(instr->getParams()[0]) + "]\n";
}

void ArmBackend::translate()
{
  cout << generatePrologue();
  for (CFG *cfg : cfgs)
  {
    for (BasicBlock *bb : cfg->blocks)
    {
      for (IRInstr *instr : bb->instrs)
      {
        cout << generate(instr);
      }
    }
  }
}

string ArmBackend::generate(IRInstr *instr)
{
  string code = "";
  switch (instr->getOp())
  {
  case IRInstr::ldconst: {
    int val = stoi(instr->getParams()[1]);
    unsigned int uval = (unsigned int)val;
    if (val >= 0 && val <= 65535) {
      code += "    mov w8, #" + instr->getParams()[1] + "\n";
    } else {
      code += "    movz w8, #" + to_string(uval & 0xFFFF) + "\n";
      if (uval >> 16)
        code += "    movk w8, #" + to_string(uval >> 16) + ", lsl #16\n";
    }
    code += "    str w8, [sp, #" + getOffset(instr->getParams()[0]) + "]\n";
    break;
  }
  case IRInstr::copy:
    code += "    ldr w8, [sp, #" + getOffset(instr->getParams()[1]) + "]\n";
    code += "    str w8, [sp, #" + getOffset(instr->getParams()[0]) + "]\n";
    break;
  case IRInstr::add:
    code += loadBinaryOperands(instr);
    code += "    add w0, w0, w1\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::sub:
    code += loadBinaryOperands(instr);
    code += "    sub w0, w0, w1\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::mul:
    code += loadBinaryOperands(instr);
    code += "    mul w0, w0, w1\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::div:
    code += loadBinaryOperands(instr);
    code += "    sdiv w0, w0, w1\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::mod:
    code += loadBinaryOperands(instr);
    code += "    sdiv w2, w0, w1\n";      
    code += "    msub w0, w2, w1, w0\n"; 
    code += saveResultEax(instr);
    break;
  case IRInstr::and_:
    code += loadBinaryOperands(instr);
    code += "    and w0, w0, w1\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::or_:
    code += loadBinaryOperands(instr);
    code += "    orr w0, w0, w1\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::xor_:
    code += loadBinaryOperands(instr);
    code += "    eor w0, w0, w1\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::neg:
    code += "    ldr w0, [sp, #" + getOffset(instr->getParams()[1]) + "]\n";
    code += "    neg w0, w0\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::not_:
    code += "    ldr w0, [sp, #" + getOffset(instr->getParams()[1]) + "]\n";
    code += "    cmp w0, #0\n";
    code += "    cset w0, eq\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::cmp_eq:
  case IRInstr::cmp_ne:
  case IRInstr::cmp_lt:
  case IRInstr::cmp_le:
  case IRInstr::cmp_gt:
  case IRInstr::cmp_ge:
    code += loadBinaryOperands(instr);
    code += "    cmp w0, w1\n";
    if (instr->getOp() == IRInstr::cmp_eq)      code += "    cset w0, eq\n";
    else if (instr->getOp() == IRInstr::cmp_ne) code += "    cset w0, ne\n";
    else if (instr->getOp() == IRInstr::cmp_lt) code += "    cset w0, lt\n";
    else if (instr->getOp() == IRInstr::cmp_le) code += "    cset w0, le\n";
    else if (instr->getOp() == IRInstr::cmp_gt) code += "    cset w0, gt\n";
    else if (instr->getOp() == IRInstr::cmp_ge) code += "    cset w0, ge\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::ret: {
    int frameSize = computeFrameSize();
    code += "    ldr w0, [sp, #" + getOffset(instr->getParams()[0]) + "]\n";
    code += "    add sp, sp, #" + to_string(frameSize) + "\n";
    code += "    ret\n";
    break;
  }
  default:
    break;
  }
  return code;
}