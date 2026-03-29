#include "backend.h"
#include <algorithm>
#include <iostream>
#include <set>

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
        int paramCount = (int)cfg->paramVarNames.size();
        for (int i = 0; i < paramCount && i < (int)argRegs32.size(); i++) {
          cout << "    movl " << argRegs32[i] << ", " << getOffset(cfg->paramVarNames[i]) << "\n";
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
  case IRInstr::sub: {
    code += loadBinaryOperands(instr);
    code += (instr->getOp() == IRInstr::add) ? "    addl %ebx, %eax\n" : "    subl %ebx, %eax\n";
    code += saveResultEax(instr);
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
  }
  case IRInstr::call: {
    const vector<string> &p = instr->getParams();
    string funcName = p[0];
    string dest = p[1];
    int argc = (int)p.size() - 2;
    static const vector<string> argRegs32 = {
      "%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"
    };
    for (int i = 0; i < argc; i++) {
      code += "    movl " + getOffset(p[i + 2]) + ", " + argRegs32[i] + "\n";
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





ArmBackend::ArmBackend(const vector<CFG *> &cfgs, const SymbolTable &symbolTable)
    : backend(cfgs, symbolTable) {}

void ArmBackend::buildLocalOffsets(CFG *cfg)
{
  currentCFG = cfg;
  localOffsets.clear();
  
  std::set<std::string> usedVars;
  for (BasicBlock *bb : cfg->blocks)
  {
    for (IRInstr *instr : bb->instrs)
    {
      for (const auto &param : instr->getParams())
      {
        if (symbolTable.find(param) != symbolTable.end())
        {
          usedVars.insert(param);
        }
      }
    }
  }
  
  for (const auto &paramName : cfg->paramVarNames)
  {
    usedVars.insert(paramName);
  }

  int offset = -4;
  for (const auto &var : usedVars)
  {
    localOffsets[var] = offset;
    offset -= 4;
  }
}

int ArmBackend::computeFrameSize() const
{
  if (currentCFG == nullptr) return 0;
  
  int totalVars = localOffsets.size();
  int size = totalVars * 4;
  
  if (size % 16 != 0) size += 16 - (size % 16);
  return size;
}

string ArmBackend::getOffset(const string &varName)
{
  if (localOffsets.find(varName) != localOffsets.end())
  {
    int frameSize = computeFrameSize();
    int spOffset = frameSize + localOffsets[varName];
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
  for (CFG *cfg : cfgs)
  {
    buildLocalOffsets(cfg);
    int frameSize = computeFrameSize();
    
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
        cout << "    stp fp, lr, [sp, #-16]!\n";
        cout << "    mov fp, sp\n";
        if (frameSize > 0) {
          cout << "    sub sp, sp, #" << frameSize << "\n";
        }
        static const vector<string> argRegsW = {
          "w0", "w1", "w2", "w3", "w4", "w5", "w6", "w7"
        };
        int paramCount = (int)cfg->paramVarNames.size();
        for (int i = 0; i < paramCount && i < (int)argRegsW.size(); i++) {
          cout << "    str " << argRegsW[i] << ", [sp, #" << getOffset(cfg->paramVarNames[i]) << "]\n";
        }
      }
      else if (bb->label == "epilogue") {
        cout << asmLabel(bb) << ":\n";
        cout << "    mov sp, fp\n";
        cout << "    ldp fp, lr, [sp], #16\n";
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
  case IRInstr::copy: {
    code += "    ldr w8, [sp, #" + getOffset(instr->getParams()[1]) + "]\n";
    code += "    str w8, [sp, #" + getOffset(instr->getParams()[0]) + "]\n";
    break;
  }
  case IRInstr::add:
  case IRInstr::sub: {
    code += loadBinaryOperands(instr);
    code += (instr->getOp() == IRInstr::add) ? "    add w0, w0, w1\n" : "    sub w0, w0, w1\n";
    code += saveResultEax(instr);
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
  case IRInstr::call: {
    const vector<string> &p = instr->getParams();
    string funcName = p[0];
    string dest = p[1];
    int argc = (int)p.size() - 2;
    static const vector<string> argRegsW = {
      "w0", "w1", "w2", "w3", "w4", "w5", "w6", "w7"
    };
    for (int i = 0; i < argc && i < (int)argRegsW.size(); i++) {
      code += "    ldr " + argRegsW[i] + ", [sp, #" + getOffset(p[i + 2]) + "]\n";
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
