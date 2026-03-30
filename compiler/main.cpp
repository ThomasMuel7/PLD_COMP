#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

#include "antlr4-runtime.h"
#include "generated/ifccLexer.h"
#include "generated/ifccParser.h"

#include "src/IRVisitor.h"
#include "src/SymbolVisitor.h"
#include "src/IRVisitor.h"
#include "src/backend.h"

using namespace antlr4;
using namespace std;
int main(int argc, const char **argv)
{
  stringstream in;
  string target = "x86";
  string inputFile;

  for (int i = 1; i < argc; i++)
  {
    string arg = argv[i];

    if (arg == "-target" && i + 1 < argc)
    {
      target = argv[++i];
    }
    else
    {
      inputFile = arg;
    }
  }

  if (inputFile.empty())
  {
    cerr << "usage: ifcc [-target x86|arm] path/to/file.c" << endl;
    exit(1);
  }

  //  Lecture fichier
  ifstream lecture(inputFile);
  if (!lecture)
  {
    cerr << "Erreur ouverture fichier" << endl;
    exit(1);
  }
  in << lecture.rdbuf();

  //  ANTLR
  ANTLRInputStream input(in.str());
  ifccLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  tokens.fill();

  ifccParser parser(&tokens);
  ifccParser::AxiomContext *tree = parser.axiom();

  if (parser.getNumberOfSyntaxErrors() != 0)
  {
    cerr << "Erreur de syntaxe détectée." << endl;
    exit(1);
  }

  //  Analyse sémantique
  SymbolVisitor visitor;
  visitor.visit(tree->prog());

  if (visitor.hasError)
  {
    cerr << "Erreur sémantique." << endl;
    return 1;
  }

  //  IR
  IRVisitor irVisitor(visitor.table, visitor.functionTable, visitor.currentOffset);
  irVisitor.visit(tree->prog());

  //  Backend
  backend *backendInstance = nullptr;

  if (target == "x86")
  {
    backendInstance = new x86Backend(irVisitor.getCFGs(), visitor.table);
  }
  else if (target == "arm")
  {
    backendInstance = new ArmBackend(irVisitor.getCFGs(), visitor.table);
  }
  else
  {
    cerr << "Target inconnue: " << target << endl;
    return 1;
  }

  backendInstance->translate();

  return 0;
}
