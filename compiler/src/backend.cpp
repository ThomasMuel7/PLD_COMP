#include "backend.h"
#include <algorithm>
#include <iostream>

using namespace std;

x86Backend::x86Backend(const vector<CFG *> &cfgs, const SymbolTable &symbolTable)
    : backend(cfgs, symbolTable) {}

int x86Backend::computeFrameSize() const
{
  int minOffset = 0;
  for (const auto &entry : symbolTable)
  {
    minOffset = std::min(minOffset, entry.second.index);
  }
  int size = -minOffset;
  if (size % 16 != 0)
  {
    size += 16 - (size % 16);
  }
  return size;
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
  int frameSize = computeFrameSize();
  for (CFG *cfg : cfgs)
  {
    string labelPrefix = cfg->functionName + "_";
    auto asmLabel = [&](BasicBlock *bb) {
      if (bb->label == "prologue") {
        return cfg->functionName;
      }
      return labelPrefix + bb->label;
    };

#ifdef __APPLE__
    cout << ".globl _" << cfg->functionName << "\n";
#else
    cout << ".globl " << cfg->functionName << "\n";
#endif

    for (BasicBlock *bb : cfg->blocks)
    {
      if (bb->label == "prologue") {
#ifdef __APPLE__
        cout << " _" << cfg->functionName << ":\n";
#else
        cout << " " << cfg->functionName << ":\n";
#endif
        cout << "    pushq %rbp\n";
        cout << "    movq %rsp, %rbp\n";
        if (frameSize > 0) {
          cout << "    subq $" << frameSize << ", %rsp\n";
        }
        static const vector<string> argRegs32 = {
          "%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"
        };
        static const vector<string> argRegs64 = {
          "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"
        };
        int paramCount = (int)cfg->paramVarNames.size();
        for (int i = 0; i < paramCount && i < (int)argRegs32.size(); i++) {
          bool isPtr = (i < (int)cfg->paramIsPointer.size()) ? cfg->paramIsPointer[i] : false;
          if (isPtr) {
            cout << "    movq " << argRegs64[i] << ", " << getOffset(cfg->paramVarNames[i]) << "\n";
          } else {
            cout << "    movl " << argRegs32[i] << ", " << getOffset(cfg->paramVarNames[i]) << "\n";
          }
        }
      }
      else if (bb->label == "epilogue") {
        cout << asmLabel(bb) << ":\n";
        cout << "    leave\n";
        cout << "    ret\n";
        continue;
      }
      else {
        cout << asmLabel(bb) << ":\n";
        for (IRInstr *instr : bb->instrs)
        {
          cout << generate(instr);
        }
      }
      if (bb->exit_true != nullptr && bb->exit_false != nullptr)
      {
        cout << "    cmpl $0, " << getOffset(bb->test_var_name) << "\n";
        cout << "    je " << asmLabel(bb->exit_false) << "\n";
        cout << "    jmp " << asmLabel(bb->exit_true) << "\n";
      }
      else if (bb->exit_true != nullptr)
      {
        cout << "    jmp " << asmLabel(bb->exit_true) << "\n";
      }
    }
  }
}

string x86Backend::generate(IRInstr *instr) {
  string code = "";
  auto isPtr = [&](const string &v) {
    auto it = symbolTable.find(v);
    if (it == symbolTable.end()) {
      return false;
    }
    return it->second.isPointer || it->second.isArray;
  };

  switch (instr->getOp())
  {
  case IRInstr::ldconst:
    if (isPtr(instr->getParams()[0])) {
      code += "    movq $" + instr->getParams()[1] + ", %rax\n";
      code += "    movq %rax, " + getOffset(instr->getParams()[0]) + "\n";
    } else {
      code += "    movl $" + instr->getParams()[1] + ", " + getOffset(instr->getParams()[0]) + "\n";
    }
    break;
  case IRInstr::copy:
    if (isPtr(instr->getParams()[0]) || isPtr(instr->getParams()[1])) {
      code += "    movq " + getOffset(instr->getParams()[1]) + ", %rax\n";
      code += "    movq %rax, " + getOffset(instr->getParams()[0]) + "\n";
    } else {
      code += "    movl " + getOffset(instr->getParams()[1]) + ", %eax\n";
      code += saveResultEax(instr);
    }
    break;
  case IRInstr::add:
  case IRInstr::sub: {
    bool ptrDest = isPtr(instr->getParams()[0]);
    if (ptrDest) {
      code += "    movq " + getOffset(instr->getParams()[1]) + ", %rax\n";
      code += "    movl " + getOffset(instr->getParams()[2]) + ", %ebx\n";
      code += "    movslq %ebx, %rbx\n";
      code += (instr->getOp() == IRInstr::add) ? "    addq %rbx, %rax\n" : "    subq %rbx, %rax\n";
      code += "    movq %rax, " + getOffset(instr->getParams()[0]) + "\n";
    } else {
      code += loadBinaryOperands(instr);
      code += (instr->getOp() == IRInstr::add) ? "    addl %ebx, %eax\n" : "    subl %ebx, %eax\n";
      code += saveResultEax(instr);
    }
    break;
  }
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
  case IRInstr::addr:
    code += "    leaq " + getOffset(instr->getParams()[1]) + ", %rax\n";
    code += "    movq %rax, " + getOffset(instr->getParams()[0]) + "\n";
    break;
  case IRInstr::rmem:
    code += "    movq " + getOffset(instr->getParams()[1]) + ", %rax\n";
    code += "    movl (%rax), %ebx\n";
    code += "    movl %ebx, " + getOffset(instr->getParams()[0]) + "\n";
    break;
  case IRInstr::wmem:
    code += "    movq " + getOffset(instr->getParams()[0]) + ", %rax\n";
    code += "    movl " + getOffset(instr->getParams()[1]) + ", %ebx\n";
    code += "    movl %ebx, (%rax)\n";
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
  case IRInstr::cmp_ge: {
    bool ptrCmp = isPtr(instr->getParams()[1]) || isPtr(instr->getParams()[2]);
    if (ptrCmp) {
      code += "    movq " + getOffset(instr->getParams()[1]) + ", %rax\n";
      code += "    movq " + getOffset(instr->getParams()[2]) + ", %rbx\n";
      code += "    cmpq %rbx, %rax\n";
    } else {
      code += loadBinaryOperands(instr);
      code += "    cmpl %ebx, %eax\n";
    }
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
  }
  case IRInstr::call: {
    const vector<string> &p = instr->getParams();
    string funcName = p[0];
    string dest = p[1];
    int argc = (int)p.size() - 2;
    static const vector<string> argRegs32 = {
      "%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"
    };
    static const vector<string> argRegs64 = {
      "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"
    };
    for (int i = 0; i < argc; i++) {
      if (isPtr(p[i + 2])) {
        code += "    movq " + getOffset(p[i + 2]) + ", " + argRegs64[i] + "\n";
      } else {
        code += "    movl " + getOffset(p[i + 2]) + ", " + argRegs32[i] + "\n";
      }
    }
#ifdef __APPLE__
    code += "    call _" + funcName + "\n";
#else
    code += "    call " + funcName + "\n";
#endif
    code += "    movl %eax, " + getOffset(dest) + "\n";
    break;
  }
  case IRInstr::ret:
    code += "    movl " + getOffset(instr->getParams()[0]) + ", %eax\n";
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
  int minOffset = 0;
  for (const auto &entry : symbolTable)
  {
    minOffset = std::min(minOffset, entry.second.index);
  }
  int size = -minOffset;
  if (size % 16 != 0) size += 16 - (size % 16);
  return size;
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
  int frameSize = computeFrameSize();
  for (CFG *cfg : cfgs)
  {
    string labelPrefix = cfg->functionName + "_";
    auto asmLabel = [&](BasicBlock *bb) {
      if (bb->label == "prologue") {
        return cfg->functionName;
      }
      return labelPrefix + bb->label;
    };

    for (BasicBlock *bb : cfg->blocks)
    {
      if (bb->label == "prologue") {
        cout << ".text\n";
#ifdef __APPLE__
        cout << ".globl _" << cfg->functionName << "\n";
        cout << "_" << cfg->functionName << ":\n";
#else
        cout << ".globl " << cfg->functionName << "\n";
        cout << cfg->functionName << ":\n";
#endif
        cout << "    sub sp, sp, #" << frameSize << "\n";
        static const vector<string> argRegsW = {
          "w0", "w1", "w2", "w3", "w4", "w5", "w6", "w7"
        };
        static const vector<string> argRegsX = {
          "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7"
        };
        int paramCount = (int)cfg->paramVarNames.size();
        for (int i = 0; i < paramCount && i < (int)argRegsW.size(); i++) {
          bool isPtr = (i < (int)cfg->paramIsPointer.size()) ? cfg->paramIsPointer[i] : false;
          if (isPtr) {
            cout << "    str " << argRegsX[i] << ", [sp, #" << getOffset(cfg->paramVarNames[i]) << "]\n";
          } else {
            cout << "    str " << argRegsW[i] << ", [sp, #" << getOffset(cfg->paramVarNames[i]) << "]\n";
          }
        }
      }
      else if (bb->label == "epilogue") {
        cout << asmLabel(bb) << ":\n";
        cout << "    add sp, sp, #" << frameSize << "\n";
        cout << "    ret\n";
        continue;
      }
      else {
        cout << asmLabel(bb) << ":\n";
        for (IRInstr *instr : bb->instrs)
        {
          cout << generate(instr);
        }
      }

      if (bb->exit_true != nullptr && bb->exit_false != nullptr)
      {
        cout << "    ldr w8, [sp, #" << getOffset(bb->test_var_name) << "]\n";
        cout << "    cmp w8, #0\n";
        cout << "    beq " << asmLabel(bb->exit_false) << "\n";
        cout << "    b " << asmLabel(bb->exit_true) << "\n";
      }
      else if (bb->exit_true != nullptr)
      {
        cout << "    b " << asmLabel(bb->exit_true) << "\n";
      }
    }
  }
}

string ArmBackend::generate(IRInstr *instr)
{
  string code = "";
  auto isPtr = [&](const string &v) {
    auto it = symbolTable.find(v);
    if (it == symbolTable.end()) {
      return false;
    }
    return it->second.isPointer || it->second.isArray;
  };

  switch (instr->getOp())
  {
  case IRInstr::ldconst: {
    int val = stoi(instr->getParams()[1]);
    unsigned int uval = (unsigned int)val;
    if (isPtr(instr->getParams()[0])) {
      code += "    mov x8, #" + instr->getParams()[1] + "\n";
      code += "    str x8, [sp, #" + getOffset(instr->getParams()[0]) + "]\n";
      break;
    }
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
  case IRInstr::copy: {
    if (isPtr(instr->getParams()[0]) || isPtr(instr->getParams()[1])) {
      code += "    ldr x8, [sp, #" + getOffset(instr->getParams()[1]) + "]\n";
      code += "    str x8, [sp, #" + getOffset(instr->getParams()[0]) + "]\n";
    } else {
      code += "    ldr w8, [sp, #" + getOffset(instr->getParams()[1]) + "]\n";
      code += "    str w8, [sp, #" + getOffset(instr->getParams()[0]) + "]\n";
    }
    break;
  }
  case IRInstr::add:
  case IRInstr::sub: {
    bool ptrDest = isPtr(instr->getParams()[0]);
    if (ptrDest) {
      code += "    ldr x0, [sp, #" + getOffset(instr->getParams()[1]) + "]\n";
      code += "    ldr w1, [sp, #" + getOffset(instr->getParams()[2]) + "]\n";
      code += "    sxtw x1, w1\n";
      code += (instr->getOp() == IRInstr::add) ? "    add x0, x0, x1\n" : "    sub x0, x0, x1\n";
      code += "    str x0, [sp, #" + getOffset(instr->getParams()[0]) + "]\n";
    } else {
      code += loadBinaryOperands(instr);
      code += (instr->getOp() == IRInstr::add) ? "    add w0, w0, w1\n" : "    sub w0, w0, w1\n";
      code += saveResultEax(instr);
    }
    break;
  }
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
  case IRInstr::addr:
    code += "    add x8, sp, #" + getOffset(instr->getParams()[1]) + "\n";
    code += "    str x8, [sp, #" + getOffset(instr->getParams()[0]) + "]\n";
    break;
  case IRInstr::rmem:
    code += "    ldr x8, [sp, #" + getOffset(instr->getParams()[1]) + "]\n";
    code += "    ldr w9, [x8]\n";
    code += "    str w9, [sp, #" + getOffset(instr->getParams()[0]) + "]\n";
    break;
  case IRInstr::wmem:
    code += "    ldr x8, [sp, #" + getOffset(instr->getParams()[0]) + "]\n";
    code += "    ldr w9, [sp, #" + getOffset(instr->getParams()[1]) + "]\n";
    code += "    str w9, [x8]\n";
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
    if (isPtr(instr->getParams()[1]) || isPtr(instr->getParams()[2])) {
      code += "    ldr x0, [sp, #" + getOffset(instr->getParams()[1]) + "]\n";
      code += "    ldr x1, [sp, #" + getOffset(instr->getParams()[2]) + "]\n";
      code += "    cmp x0, x1\n";
    } else {
      code += loadBinaryOperands(instr);
      code += "    cmp w0, w1\n";
    }
    if (instr->getOp() == IRInstr::cmp_eq)      code += "    cset w0, eq\n";
    else if (instr->getOp() == IRInstr::cmp_ne) code += "    cset w0, ne\n";
    else if (instr->getOp() == IRInstr::cmp_lt) code += "    cset w0, lt\n";
    else if (instr->getOp() == IRInstr::cmp_le) code += "    cset w0, le\n";
    else if (instr->getOp() == IRInstr::cmp_gt) code += "    cset w0, gt\n";
    else if (instr->getOp() == IRInstr::cmp_ge) code += "    cset w0, ge\n";
    code += saveResultEax(instr);
    break;
  case IRInstr::call: {
    const vector<string> &p = instr->getParams();
    string funcName = p[0];
    string dest = p[1];
    int argc = (int)p.size() - 2;
    static const vector<string> argRegsW = {
      "w0", "w1", "w2", "w3", "w4", "w5", "w6", "w7"
    };
    static const vector<string> argRegsX = {
      "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7"
    };
    for (int i = 0; i < argc && i < (int)argRegsW.size(); i++) {
      if (isPtr(p[i + 2])) {
        code += "    ldr " + argRegsX[i] + ", [sp, #" + getOffset(p[i + 2]) + "]\n";
      } else {
        code += "    ldr " + argRegsW[i] + ", [sp, #" + getOffset(p[i + 2]) + "]\n";
      }
    }
#ifdef __APPLE__
    code += "    bl _" + funcName + "\n";
#else
    code += "    bl " + funcName + "\n";
#endif
    code += "    str w0, [sp, #" + getOffset(dest) + "]\n";
    break;
  }
  case IRInstr::ret:
    code += "    ldr w0, [sp, #" + getOffset(instr->getParams()[0]) + "]\n";
    break;
  default:
    break;
  }
  return code;
}
