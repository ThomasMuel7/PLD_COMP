#pragma once

#include "IR.h"
#include "SymbolTable.h"
#include "CFG.h"
#include "BasicBlock.h"
#include <vector>
#include <string>

class backend
{
public:
  backend(const std::vector<CFG *> &cfgs, const SymbolTable &symbolTable)
      : cfgs(cfgs), symbolTable(symbolTable) {}

  virtual void translate() = 0;
  virtual ~backend() {}

protected:
  std::vector<CFG *> cfgs;
  SymbolTable symbolTable;
  virtual std::string generate(IRInstr *instr) = 0;
};

class x86Backend : public backend
{
public:
  x86Backend(const std::vector<CFG *> &cfgs, const SymbolTable &symbolTable);

  std::string getOffset(const std::string &varName);
  void translate() override;

private:
  int computeFrameSize() const;
  std::string loadBinaryOperands(IRInstr *instr);
  std::string saveResultEax(IRInstr *instr);
  std::string generate(IRInstr *instr) override;
};

class ArmBackend : public backend
{
public:
  ArmBackend(const std::vector<CFG *> &cfgs, const SymbolTable &symbolTable);

  std::string getOffset(const std::string &varName);
  void translate() override;

private:
  int computeFrameSize() const;
  std::string loadBinaryOperands(IRInstr *instr);
  std::string saveResultEax(IRInstr *instr);
  std::string generate(IRInstr *instr) override;
  void buildLocalOffsets(CFG *cfg);  // Build per-function offset map
  
  CFG *currentCFG = nullptr;
  int currentFrameSize = 0;
  std::map<std::string, int> localOffsets;
};