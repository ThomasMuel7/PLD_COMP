#include "SymbolVisitor.h"
#include <iostream>
#include <string>
#include <map>

using namespace std;

SymbolVisitor::SymbolVisitor() {
    scopeTable.push_back(map<string, string>());
}
void SymbolVisitor::registerVariable(const string &originalName)
{
    if (scopeTable.back().find(originalName) != scopeTable.back().end())
    {
        cerr << "Erreur sémantique : la variable '" << originalName << "' est déjà déclarée dans ce bloc." << endl;
        hasError = true;
        return;
    }

    string uniqueName = originalName + "_" + to_string(uniqueVarId++);
    scopeTable.back()[originalName] = uniqueName;
    currentOffset -= 4;
    table[uniqueName] = {currentOffset, false};
}

string SymbolVisitor::resolveVariable(const string& originalName) {
    for (int i = scopeTable.size() - 1; i >= 0; i--) {
        if (scopeTable[i].find(originalName) != scopeTable[i].end()) {
            return scopeTable[i][originalName];
        }
    }
    return "";
}

antlrcpp::Any SymbolVisitor::visitBlock(ifccParser::BlockContext *ctx) {
    scopeTable.push_back(map<string, string>());
    for (auto stmt : ctx->stmt()) {
        visit(stmt);
    }
    scopeTable.pop_back();
    return 0;
}

antlrcpp::Any SymbolVisitor::visitDeclare_stmt(ifccParser::Declare_stmtContext *ctx) {
    for (auto elmtCtx : ctx->declare_elmt())
    {
        visit(elmtCtx);
    }
    return 0;
}

antlrcpp::Any SymbolVisitor::visitDeclare_elmt(ifccParser::Declare_elmtContext *ctx)
{
    if (ctx->VAR())
    {
        string name = ctx->VAR()->getText();
        registerVariable(name);
    }
    else if (ctx->assign_stmt())
    {
        string name = ctx->assign_stmt()->VAR()->getText();
        registerVariable(name);

        visit(ctx->assign_stmt());
    }
    return 0;
}

antlrcpp::Any SymbolVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
    string originalName = ctx->VAR()->getText();
    string uniqueName = resolveVariable(originalName);
    if (uniqueName == "") {
        cerr << "Erreur: la variable '" << originalName << "' utilisée dans l'expression n'est pas déclarée." << endl;
        hasError = true;
    } else {
        table[uniqueName].isUsed = true;
    }
    
    return 0;
}

antlrcpp::Any SymbolVisitor::visitAssignExpr(ifccParser::AssignExprContext *ctx) {
    string originalName = ctx->VAR()->getText();
    string uniqueName = resolveVariable(originalName);
    if (uniqueName == "") {
        cerr << "Erreur: tentative d'assignation sur la variable non déclarée '" << originalName << "'." << endl;
        hasError = true;
    } else {
        table[uniqueName].isUsed = true;
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

antlrcpp::Any SymbolVisitor::visitCallExpr(ifccParser::CallExprContext *ctx) {
    string funcName = ctx->VAR()->getText();
    int argc = (int)ctx->expr().size();

    if (funcName == "putchar" && argc != 1) {
        cerr << "Erreur: putchar attend exactement 1 argument." << endl;
        hasError = true;
    }
    else if (funcName == "getchar" && argc != 0) {
        cerr << "Erreur: getchar n'attend aucun argument." << endl;
        hasError = true;
    }
    else if (argc > 6) {
        cerr << "Erreur: plus de 6 arguments non supportés." << endl;
        hasError = true;
    }

    for (auto argCtx : ctx->expr()) {
        visit(argCtx);
    }

    return 0;
}