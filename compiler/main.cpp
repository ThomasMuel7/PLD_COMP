#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

#include "IRVisitor.h"
#include "antlr4-runtime.h"
#include "generated/ifccLexer.h"
#include "generated/ifccParser.h"
#include "generated/ifccBaseVisitor.h"

//#include "CodeGenVisitor.h"
#include "SymbolVisitor.h"
#include "IRVisitor.h"
#include "backend.h"

using namespace antlr4;
using namespace std;

int main(int argn, const char **argv) {
  stringstream in;
  if (argn==2) {
     ifstream lecture(argv[1]);
     in << lecture.rdbuf();
  } else {
      cerr << "usage: ifcc path/to/file.c" << endl ;
      exit(1);
  }
  
  ANTLRInputStream input(in.str());

  ifccLexer lexer(&input);
  CommonTokenStream tokens(&lexer);

  tokens.fill();

  ifccParser parser(&tokens);
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

  x86Backend backend({irVisitor.getCFG()}, symbolTable);
  backend.translate();
  
  return 0;
}
