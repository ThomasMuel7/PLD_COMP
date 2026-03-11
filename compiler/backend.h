#pragma once

#include "IR.h"
#include "SymbolTable.h"
#include <vector>
#include <string>

class backend {
public:
  backend(const std::vector<CFG*>& cfgs, const SymbolTable& symbolTable) 
      : cfgs(cfgs), symbolTable(symbolTable) {}
  
  virtual void translate() = 0;
  virtual std::string generate(IRInstr *instr) = 0; 
  virtual ~backend() {}

protected:
  std::vector<CFG*> cfgs;
  SymbolTable symbolTable;
};

class x86Backend : public backend {
public:
  x86Backend(const std::vector<CFG*>& cfgs, const SymbolTable& symbolTable);

  std::string getOffset(const std::string& varName);
  void translate() override;
  std::string generate(IRInstr *instr) override;

private:
  std::string loadBinaryOperands(IRInstr *instr);
  std::string saveResultEax(IRInstr *instr);
};