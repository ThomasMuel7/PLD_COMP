#include "SymbolVisitor.h"
#include <iostream>
#include <string>

using namespace std;

antlrcpp::Any SymbolVisitor::visitDeclare_stmt(ifccParser::Declare_stmtContext *ctx) {
    for (auto var : ctx->VAR()) {
        string varName = var->getText();
            
        if (table.find(varName) != table.end()) {
            cerr << "Erreur: la variable '" << varName << "' est déjà déclarée." << endl;
            hasError = true;
        } else {
            currentOffset -= 4;
            table[varName] = {currentOffset, false};
        }
    }
    return 0;
}

antlrcpp::Any SymbolVisitor::visitAssignExpr(ifccParser::AssignExprContext *ctx)
{
    string varName = ctx->VAR()->getText();
    
    if (table.find(varName) == table.end()) {
        cerr << "Erreur: la variable '" << varName << "' n'est pas déclarée." << endl;
        hasError = true;
    } else {
        table[varName].isUsed = true;
    }
    
    visit(ctx->expr()); 
    return 0;
}

antlrcpp::Any SymbolVisitor::visitMultDivModExpr(ifccParser::MultDivModExprContext *ctx) {
    visit(ctx->expr(0));
    auto rightConst = dynamic_cast<ifccParser::ConstExprContext*>(ctx->expr(1));
    string op = ctx->OP->getText();
    if (rightConst != nullptr && (op == "/" || op == "%")) {
        int value = stoi(rightConst->getText());
        if (value == 0) {
            cerr << "Warning: division ou modulo par zéro." << endl;
        }
        else {
            visit(ctx->expr(1));
        }
    }
    else {
        visit(ctx->expr(1));
    }
    return 0;
}

antlrcpp::Any SymbolVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
    string varName = ctx->VAR()->getText();
    if (table.find(varName) == table.end()) {
        cerr << "Erreur: la variable '" << varName << "' utilisée dans l'expression n'est pas déclarée." << endl;
        hasError = true;
    } else {
        table[varName].isUsed = true;
    }
    return 0;
}

void SymbolVisitor::checkUnusedVariables() {
    for (const auto& pair : table) {
        if (!pair.second.isUsed) {
            cerr << "Warning: la variable '" << pair.first << "' est déclarée mais non utilisée." << endl;
        }
    }
}