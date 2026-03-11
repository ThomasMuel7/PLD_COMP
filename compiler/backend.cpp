#include "backend.h"
#include <iostream>

using namespace std;

x86Backend::x86Backend(const vector<CFG*>& cfgs, const SymbolTable& symbolTable) 
    : backend(cfgs, symbolTable) {}

string x86Backend::getOffset(const string& varName) {
  auto it = symbolTable.find(varName);
  if (it != symbolTable.end()) {
    return to_string(it->second.index) + "(%rbp)";
  }
  return "0(%rbp)"; 
}

string x86Backend::loadBinaryOperands(IRInstr *instr) {
  string code = "    movl " + getOffset(instr->params[1]) + ", %eax\n";
  code += "    movl " + getOffset(instr->params[2]) + ", %ebx\n";
  return code;
}

string x86Backend::saveResultEax(IRInstr *instr) {
  return "    movl %eax, " + getOffset(instr->params[0]) + "\n";
}

void x86Backend::translate() {
  for (CFG* cfg : cfgs) {
    for (BasicBlock* bb : cfg->blocks) {
      for (IRInstr* instr : bb->instrs) {
        cout << generate(instr);
      }
    }
  }
}

string x86Backend::generate(IRInstr *instr) {
  string code = "";
  switch (instr->op) {
    case IRInstr::ldconst:
      code += "    movl $" + instr->params[1] + ", " + getOffset(instr->params[0]) + "\n";
      break;
    case IRInstr::copy:
      code += "    movl " + getOffset(instr->params[1]) + ", %eax\n";
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
      code += "    movl %edx, " + getOffset(instr->params[0]) + "\n";
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
      code += "    movl " + getOffset(instr->params[1]) + ", %eax\n";
      code += "    negl %eax\n";
      code += saveResultEax(instr);
      break;
    case IRInstr::not_:
      code += "    movl " + getOffset(instr->params[1]) + ", %eax\n";
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
      if (instr->op == IRInstr::cmp_eq) code += "    sete %al\n";
      else if (instr->op == IRInstr::cmp_ne) code += "    setne %al\n";
      else if (instr->op == IRInstr::cmp_lt) code += "    setl %al\n";
      else if (instr->op == IRInstr::cmp_le) code += "    setle %al\n";
      else if (instr->op == IRInstr::cmp_gt) code += "    setg %al\n";
      else if (instr->op == IRInstr::cmp_ge) code += "    setge %al\n";
      code += "    movzbl %al, %eax\n";
      code += saveResultEax(instr);
      break;
    case IRInstr::ret:
      code += "    movl " + getOffset(instr->params[0]) + ", %eax\n";
      code += "    popq %rbp\n";
      code += "    ret\n";
      break;
    default:
      break;
  }
  return code;
}