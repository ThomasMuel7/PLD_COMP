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



// ===================== ARM BACKEND =====================



ArmBackend::ArmBackend(const vector<CFG *> &cfgs, const SymbolTable &symbolTable)
    : backend(cfgs, symbolTable) {}

string ArmBackend::generatePrologue()
{
  string code = ".text\n";
#ifdef __APPLE__
  code += ".globl _main\n";
  code += "_main:\n";
#else
  code += ".globl main\n";
  code += "main:\n";
#endif
  code += "    push {fp, lr}\n";
  code += "    mov fp, sp\n";
  return code;
}

string ArmBackend::getOffset(const string &varName)
{
  auto it = symbolTable.find(varName);
  if (it != symbolTable.end())
  {
    int offset = -4 * (it->second.index + 1);
    return "#" + std::to_string(offset); 
  }
  return "#0";
}

string ArmBackend::loadBinaryOperands(IRInstr *instr)
{
  // Charge les opérandes dans r0 et r1
  string code = "    ldr r0, [fp, " + getOffset(instr->getParams()[1]) + "]\n";
  code += "    ldr r1, [fp, " + getOffset(instr->getParams()[2]) + "]\n";
  return code;
}

string ArmBackend::saveResultEax(IRInstr *instr)
{
  // Sauvegarde r0 dans la variable cible
  return "    str r0, [fp, " + getOffset(instr->getParams()[0]) + "]\n";
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
  case IRInstr::ldconst:
    code += "    mov r0, #" + instr->getParams()[1] + "\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::copy:
    code += "    ldr r0, [fp, " + getOffset(instr->getParams()[1]) + "]\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::add:
    code += loadBinaryOperands(instr);
    code += "    add r0, r0, r1\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::sub:
    code += loadBinaryOperands(instr);
    code += "    sub r0, r0, r1\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::mul:
    code += loadBinaryOperands(instr);
    code += "    mul r0, r0, r1\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::div:
    code += loadBinaryOperands(instr);
    code += "    bl __aeabi_idiv\n"; // r0 = r0 / r1
    code += saveResultEax(instr);
    break;
  case IRInstr::mod:
    code += loadBinaryOperands(instr);
    code += "    bl __aeabi_idivmod\n"; // r0 = r0 % r1 (résultat dans r1)
    code += "    mov r0, r1\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::and_:
    code += loadBinaryOperands(instr);
    code += "    and r0, r0, r1\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::or_:
    code += loadBinaryOperands(instr);
    code += "    orr r0, r0, r1\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::xor_:
    code += loadBinaryOperands(instr);
    code += "    eor r0, r0, r1\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::neg:
    code += "    ldr r0, [fp, " + getOffset(instr->getParams()[1]) + "]\n";
    code += "    rsb r0, r0, #0\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::not_:
    code += "    ldr r0, [fp, " + getOffset(instr->getParams()[1]) + "]\n";
    code += "    cmp r0, #0\n";
    code += "    moveq r0, #1\n";
    code += "    movne r0, #0\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::cmp_eq:
  case IRInstr::cmp_ne:
  case IRInstr::cmp_lt:
  case IRInstr::cmp_le:
  case IRInstr::cmp_gt:
  case IRInstr::cmp_ge:
    code += loadBinaryOperands(instr);
    code += "    cmp r0, r1\n";
    if (instr->getOp() == IRInstr::cmp_eq)
      code += "    moveq r0, #1\n    movne r0, #0\n";
    else if (instr->getOp() == IRInstr::cmp_ne)
      code += "    movne r0, #1\n    moveq r0, #0\n";
    else if (instr->getOp() == IRInstr::cmp_lt)
      code += "    movlt r0, #1\n    movge r0, #0\n";
    else if (instr->getOp() == IRInstr::cmp_le)
      code += "    movle r0, #1\n    movgt r0, #0\n";
    else if (instr->getOp() == IRInstr::cmp_gt)
      code += "    movgt r0, #1\n    movle r0, #0\n";
    else if (instr->getOp() == IRInstr::cmp_ge)
      code += "    movge r0, #1\n    movlt r0, #0\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::ret:
    code += "    ldr r0, [fp, " + getOffset(instr->getParams()[0]) + "]\n";
    code += "    pop {fp, pc}\n";
    break;
  default:
    break;
  }
  return code;
}