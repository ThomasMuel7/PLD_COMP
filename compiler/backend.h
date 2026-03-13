#pragma once

#include "IR.h"
#include "SymbolTable.h"
<<<<<<< HEAD
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
=======
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
>>>>>>> 6e2b3bbbfee102d95899e4843c55c1b244133002

private:
  std::string loadBinaryOperands(IRInstr *instr);
  std::string saveResultEax(IRInstr *instr);
  std::string generate(IRInstr *instr) override;
  std::string generatePrologue();
};