#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

#include "IRVisitor.h"
#include "antlr4-runtime.h"
#include "generated/ifccLexer.h"
#include "generated/ifccParser.h"
#include "generated/ifccBaseVisitor.h"

<<<<<<< HEAD
//#include "CodeGenVisitor.h"
=======
>>>>>>> 6e2b3bbbfee102d95899e4843c55c1b244133002
#include "SymbolVisitor.h"
#include "IRVisitor.h"
#include "backend.h"

using namespace antlr4;
using namespace std;

int main(int argn, const char **argv) {
  stringstream in;
<<<<<<< HEAD
  if (argn==2) {
     ifstream lecture(argv[1]);
     in << lecture.rdbuf();
  } else {
      cerr << "usage: ifcc path/to/file.c" << endl ;
      exit(1);
=======
  if (argn == 2)
  {
    ifstream lecture(argv[1]);
    in << lecture.rdbuf();
  }
  else
  {
    cerr << "usage: ifcc path/to/file.c" << endl;
    exit(1);
>>>>>>> 6e2b3bbbfee102d95899e4843c55c1b244133002
  }

  ANTLRInputStream input(in.str());

  ifccLexer lexer(&input);
  CommonTokenStream tokens(&lexer);

  tokens.fill();

  ifccParser parser(&tokens);
<<<<<<< HEAD
  tree::ParseTree* tree = parser.prog();

  if(parser.getNumberOfSyntaxErrors() != 0) {
      cerr << "Erreur de syntaxe détectée lors du parsing." << endl;
      exit(1);
  }

  SymbolTableVisitor visitor;
  visitor.visit(tree);

  if (visitor.hasError()) {
    return 1;
  }

  SymbolTable symbolTable = visitor.getSymbolTable();

  IRVisitor v(symbolVisitor.table, symbolVisitor.currentOffset);
  v.visit(tree);
=======
  tree::ParseTree *tree = parser.prog();

  if (parser.getNumberOfSyntaxErrors() != 0)
  {
    cerr << "Erreur de syntaxe détectée lors du parsing." << endl;
    exit(1);
  }

  SymbolVisitor visitor;
  visitor.visit(tree);

  if (visitor.hasError)
  {
    cerr << "Erreur de sémantique détectée lors de la visite du symbole." << endl;
    return 1;
  }

  IRVisitor irVisitor(visitor.table, visitor.currentOffset);
  irVisitor.visit(tree);

  x86Backend backend({irVisitor.getCFG()}, visitor.table);
  backend.translate();
>>>>>>> 6e2b3bbbfee102d95899e4843c55c1b244133002

  x86Backend backend({irVisitor.getCFG()}, symbolTable);
  backend.translate();
  
  return 0;
}