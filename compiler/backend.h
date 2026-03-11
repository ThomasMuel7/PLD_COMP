#include "IR.h"
#include "SymbolTable.h"
#include <iostream>

class backend {
public:
  
  backend(const vector<CFG*>& cfgs, const SymbolTable symbolTable ) : cfgs(cfgs), symbolTable(symbolTable) {}
  virtual void translate() = 0;
  virtual void generate(IRInstr *instr) = 0; 
  virtual ~backend() {}
private :
  vector<CFG*> cfgs;
  SymbolTable symbolTable;
};

class x86Backend : public backend {
public:
  void translate() override {
    for (CFG* cfg : cfgs) {
      for (BasicBlock* bb : cfg->getBasicBlocks()) {
        for (IRInstr* instr : bb->getInstrs()) {
          std::cout << generate(instr) << std::endl;
        }
      }
    }

  }

  std::string generate(IRInstr *instr) override {
    switch (instr->op) {
      case IRInstr::ldconst:
        // Générer le code pour charger une constante
        break;
      case IRInstr::copy:
        // Générer le code pour copier une variable
        break;
      case IRInstr::add:
        // Générer le code pour l'addition
        break;
      case IRInstr::sub:
        // Générer le code pour la soustraction
        break;
      case IRInstr::mul:
        // Générer le code pour la multiplication
        break;
      case IRInstr::div:
        // Générer le code pour la division
        break;
      case IRInstr::mod:
        // Générer le code pour le modulo
        break;
      case IRInstr::rmem:
        // Générer le code pour lire depuis la mémoire
        break;
      case IRInstr::wmem:
        // Générer le code pour écrire dans la mémoire
        break;
      case IRInstr::call:
        // Générer le code pour un appel de fonction
        break;
      case IRInstr::cmp_eq:
        // Générer le code pour comparer l'égalité
        break;
      case IRInstr::cmp_lt:
        // Générer le code pour comparer si inférieur
        break;
      case IRInstr::cmp_le:
        // Générer le code pour comparer si inférieur ou égal
        break;
    }

  }
};
