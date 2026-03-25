#include "SymbolVisitor.h"
#include <iostream>
#include <string>
#include <map>

using namespace std;

SymbolVisitor::SymbolVisitor()
{
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

static ReturnType parseReturnType(const string &typeText)
{
    return (typeText == "void") ? ReturnType::Void : ReturnType::Int;
}

string SymbolVisitor::resolveVariable(const string &originalName)
{
    for (int i = scopeTable.size() - 1; i >= 0; i--)
    {
        if (scopeTable[i].find(originalName) != scopeTable[i].end())
        {
            return scopeTable[i][originalName];
        }
    }
    return "";
}

antlrcpp::Any SymbolVisitor::visitProg(ifccParser::ProgContext *ctx)
{
    for (auto fn : ctx->function_decl())
    {
        string name = fn->VAR()->getText();
        if (functionTable.find(name) != functionTable.end())
        {
            cerr << "Erreur: la fonction '" << name << "' est déjà définie." << endl;
            hasError = true;
            continue;
        }

        int arity = 0;
        if (fn->param_list() != nullptr)
        {
            arity = (int)fn->param_list()->param().size();
        }

        functionTable[name] = {parseReturnType(fn->type()->getText()), arity, {}};
    }

    if (functionTable.find("main") == functionTable.end())
    {
        cerr << "Erreur: la fonction main est absente." << endl;
        hasError = true;
    }

    for (auto fn : ctx->function_decl())
    {
        visit(fn);
    }
    return 0;
}

antlrcpp::Any SymbolVisitor::visitFunction_decl(ifccParser::Function_declContext *ctx)
{
    currentFunctionName = ctx->VAR()->getText();
    currentFunctionReturnType = parseReturnType(ctx->type()->getText());

    scopeTable.push_back(map<string, string>());

    auto it = functionTable.find(currentFunctionName);
    if (it != functionTable.end())
    {
        it->second.paramUniqueNames.clear();
    }

    if (ctx->param_list() != nullptr)
    {
        for (auto p : ctx->param_list()->param())
        {
            string originalName = p->VAR()->getText();
            if (scopeTable.back().find(originalName) != scopeTable.back().end())
            {
                cerr << "Erreur: paramètre dupliqué '" << originalName << "' dans la fonction '" << currentFunctionName << "'." << endl;
                hasError = true;
                continue;
            }

            string uniqueName = originalName + "_" + to_string(uniqueVarId++);
            scopeTable.back()[originalName] = uniqueName;
            currentOffset -= 4;
            table[uniqueName] = {currentOffset, false};
            if (it != functionTable.end())
            {
                it->second.paramUniqueNames.push_back(uniqueName);
            }
        }
    }

    visit(ctx->block());
    scopeTable.pop_back();
    return 0;
}

antlrcpp::Any SymbolVisitor::visitBlock(ifccParser::BlockContext *ctx)
{
    scopeTable.push_back(map<string, string>());
    for (auto stmt : ctx->stmt())
    {
        visit(stmt);
    }
    scopeTable.pop_back();
    return 0;
}

antlrcpp::Any SymbolVisitor::visitDeclare_stmt(ifccParser::Declare_stmtContext *ctx)
{
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

antlrcpp::Any SymbolVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
    if (currentFunctionReturnType == ReturnType::Void)
    {
        if (ctx->expr() != nullptr)
        {
            cerr << "Erreur: return avec expression dans une fonction void ('" << currentFunctionName << "')." << endl;
            hasError = true;
            visit(ctx->expr());
        }
        return 0;
    }

    if (ctx->expr() == nullptr)
    {
        cerr << "Erreur: return sans expression dans une fonction int ('" << currentFunctionName << "')." << endl;
        hasError = true;
        return 0;
    }

    visit(ctx->expr());
    return 0;
}

antlrcpp::Any SymbolVisitor::visitVarExpr(ifccParser::VarExprContext *ctx)
{
    string originalName = ctx->VAR()->getText();
    string uniqueName = resolveVariable(originalName);
    if (uniqueName == "")
    {
        cerr << "Erreur: la variable '" << originalName << "' utilisée dans l'expression n'est pas déclarée." << endl;
        hasError = true;
    }
    else
    {
        table[uniqueName].isUsed = true;
    }

    return 0;
}

antlrcpp::Any SymbolVisitor::visitAssignExpr(ifccParser::AssignExprContext *ctx)
{
    string originalName = ctx->VAR()->getText();
    string uniqueName = resolveVariable(originalName);
    if (uniqueName == "")
    {
        cerr << "Erreur: tentative d'assignation sur la variable non déclarée '" << originalName << "'." << endl;
        hasError = true;
    }
    else
    {
        table[uniqueName].isUsed = true;
    }
    visit(ctx->expr());
    return 0;
}

antlrcpp::Any SymbolVisitor::visitMultDivModExpr(ifccParser::MultDivModExprContext *ctx)
{
    visit(ctx->expr(0));
    auto rightConst = dynamic_cast<ifccParser::ConstExprContext *>(ctx->expr(1));
    string op = ctx->OP->getText();
    if (rightConst != nullptr && (op == "/" || op == "%"))
    {
        int value = stoi(rightConst->getText());
        if (value == 0)
        {
            cerr << "Warning: division ou modulo par zéro." << endl;
        }
        else
        {
            visit(ctx->expr(1));
        }
    }
    else
    {
        visit(ctx->expr(1));
    }
    return 0;
}

antlrcpp::Any SymbolVisitor::visitCallExpr(ifccParser::CallExprContext *ctx)
{
    string funcName = ctx->VAR()->getText();
    int argc = (int)ctx->expr().size();

    if (argc > 6)
    {
        cerr << "Erreur: plus de 6 arguments non supportés." << endl;
        hasError = true;
    }

    if (funcName == "putchar" && argc != 1)
    {
        cerr << "Erreur: putchar attend exactement 1 argument." << endl;
        hasError = true;
    }
    else if (funcName == "getchar" && argc != 0)
    {
        cerr << "Erreur: getchar n'attend aucun argument." << endl;
        hasError = true;
    }
    else if (funcName != "putchar" && funcName != "getchar")
    {
        auto it = functionTable.find(funcName);
        if (it == functionTable.end())
        {
            cerr << "Erreur: appel à la fonction non définie '" << funcName << "'." << endl;
            hasError = true;
        }
        else if (it->second.arity != argc)
        {
            cerr << "Erreur: la fonction '" << funcName << "' attend " << it->second.arity
                 << " argument(s), reçu " << argc << "." << endl;
            hasError = true;
        }

        bool isExprStatement = dynamic_cast<ifccParser::StmtContext *>(ctx->parent) != nullptr;
        if (it != functionTable.end() && it->second.returnType == ReturnType::Void && !isExprStatement)
        {
            cerr << "Erreur: la fonction void '" << funcName << "' ne peut pas être utilisée comme expression." << endl;
            hasError = true;
        }
    }

    for (auto argCtx : ctx->expr())
    {
        visit(argCtx);
    }

    return 0;
}