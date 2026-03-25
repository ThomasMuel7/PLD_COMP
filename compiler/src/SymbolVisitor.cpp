#include "SymbolVisitor.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <string>

using namespace std;

namespace {
constexpr int TYPE_INT = 0;
constexpr int TYPE_INVALID = 1;

int anyToExprType(const antlrcpp::Any &v) {
    try {
        return std::any_cast<int>(v);
    } catch (...) {
        return TYPE_INVALID;
    }
}
}

SymbolVisitor::SymbolVisitor() {
    scopeTable.push_back(map<string, string>());
}

static ReturnType parseReturnType(const string &typeText) {
    return (typeText == "void") ? ReturnType::Void : ReturnType::Int;
}

static string baseNameFromUnique(const string &uniqueName) {
    size_t pos = uniqueName.rfind('_');
    if (pos == string::npos || pos + 1 >= uniqueName.size()) {
        return uniqueName;
    }
    for (size_t i = pos + 1; i < uniqueName.size(); i++) {
        if (!isdigit(static_cast<unsigned char>(uniqueName[i]))) {
            return uniqueName;
        }
    }
    return uniqueName.substr(0, pos);
}

string SymbolVisitor::resolveVariable(const string &originalName) {
    for (int i = (int)scopeTable.size() - 1; i >= 0; i--) {
        auto it = scopeTable[i].find(originalName);
        if (it != scopeTable[i].end()) {
            return it->second;
        }
    }
    return "";
}

VariableInfo *SymbolVisitor::lookupVariableInfo(const string &originalName) {
    string uniqueName = resolveVariable(originalName);
    if (uniqueName.empty()) {
        return nullptr;
    }
    auto it = table.find(uniqueName);
    if (it == table.end()) {
        return nullptr;
    }
    return &it->second;
}

antlrcpp::Any SymbolVisitor::visitProg(ifccParser::ProgContext *ctx) {
    for (auto fn : ctx->function_decl()) {
        string name = fn->VAR()->getText();
        if (functionTable.find(name) != functionTable.end()) {
            cerr << "Erreur: la fonction '" << name << "' est deja definie." << endl;
            hasError = true;
            continue;
        }

        int arity = 0;
        if (fn->param_list() != nullptr) {
            arity = (int)fn->param_list()->param().size();
        }

        functionTable[name] = {parseReturnType(fn->type()->getText()), arity, {}};
    }

    if (functionTable.find("main") == functionTable.end()) {
        cerr << "Erreur: la fonction main est absente." << endl;
        hasError = true;
    }

    for (auto fn : ctx->function_decl()) {
        visit(fn);
    }

    checkUnusedVariables();
    return 0;
}

void SymbolVisitor::checkUnusedVariables() {
    for (const auto &entry : table) {
        const string &uniqueName = entry.first;
        const VariableInfo &info = entry.second;
        if (!info.isUsed) {
            cerr << "Warning: la variable '" << baseNameFromUnique(uniqueName)
                 << "' est declaree mais jamais utilisee";
            if (info.declLine > 0) {
                cerr << " (ligne " << info.declLine << ")";
            }
            cerr << "." << endl;
        }
    }
}

antlrcpp::Any SymbolVisitor::visitFunction_decl(ifccParser::Function_declContext *ctx) {
    currentFunctionName = ctx->VAR()->getText();
    currentFunctionReturnType = parseReturnType(ctx->type()->getText());

    scopeTable.push_back(map<string, string>());

    auto fit = functionTable.find(currentFunctionName);
    if (fit != functionTable.end()) {
        fit->second.paramUniqueNames.clear();
    }

    if (ctx->param_list() != nullptr) {
        for (auto p : ctx->param_list()->param()) {
            string originalName = p->VAR()->getText();
            if (scopeTable.back().find(originalName) != scopeTable.back().end()) {
                cerr << "Erreur: parametre duplique '" << originalName << "' dans la fonction '"
                     << currentFunctionName << "'." << endl;
                hasError = true;
                continue;
            }

            string uniqueName = originalName + "_" + to_string(uniqueVarId++);
            scopeTable.back()[originalName] = uniqueName;
            int declLine = p->start->getLine();
            currentOffset -= 4;
            table[uniqueName] = {currentOffset, false, declLine};

            if (fit != functionTable.end()) {
                fit->second.paramUniqueNames.push_back(uniqueName);
            }
        }
    }

    visit(ctx->block());
    scopeTable.pop_back();
    return 0;
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
    for (auto decl : ctx->declarator()) {
        string originalName = decl->VAR()->getText();
        if (scopeTable.back().find(originalName) != scopeTable.back().end()) {
            cerr << "Erreur: la variable '" << originalName << "' est deja declaree dans ce bloc." << endl;
            hasError = true;
            continue;
        }

        string uniqueName = originalName + "_" + to_string(uniqueVarId++);
        scopeTable.back()[originalName] = uniqueName;
        int declLine = decl->start->getLine();
        currentOffset -= 4;
        table[uniqueName] = {currentOffset, false, declLine};

        if (decl->expr() != nullptr) {
            int rhsType = anyToExprType(visit(decl->expr()));
            if (rhsType != TYPE_INT && rhsType != TYPE_INVALID) {
                cerr << "Erreur: initialisation de type incompatible pour '" << originalName << "'." << endl;
                hasError = true;
            }
            table[uniqueName].isUsed = true;
        }
    }
    return 0;
}

antlrcpp::Any SymbolVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
    if (currentFunctionReturnType == ReturnType::Void) {
        if (ctx->expr() != nullptr) {
            cerr << "Erreur: return avec expression dans une fonction void ('"
                 << currentFunctionName << "')." << endl;
            hasError = true;
            visit(ctx->expr());
        }
        return 0;
    }

    if (ctx->expr() == nullptr) {
        cerr << "Erreur: return sans expression dans une fonction int ('"
             << currentFunctionName << "')." << endl;
        hasError = true;
        return 0;
    }

    int t = anyToExprType(visit(ctx->expr()));
    if (t != TYPE_INT && t != TYPE_INVALID) {
        cerr << "Erreur: return d'une expression non entiere dans une fonction int ('"
             << currentFunctionName << "')." << endl;
        hasError = true;
    }
    return 0;
}

antlrcpp::Any SymbolVisitor::visitParensExpr(ifccParser::ParensExprContext *ctx) {
    return anyToExprType(visit(ctx->expr()));
}

antlrcpp::Any SymbolVisitor::visitConstExpr(ifccParser::ConstExprContext *ctx) {
    (void)ctx;
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
    string originalName = ctx->VAR()->getText();
    string uniqueName = resolveVariable(originalName);
    if (uniqueName.empty()) {
        cerr << "Erreur: la variable '" << originalName << "' utilisee dans l'expression n'est pas declaree." << endl;
        hasError = true;
        return TYPE_INVALID;
    }
    table[uniqueName].isUsed = true;
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitAssignExpr(ifccParser::AssignExprContext *ctx) {
    string originalName = ctx->VAR()->getText();
    string uniqueName = resolveVariable(originalName);
    int rhsType = anyToExprType(visit(ctx->expr()));

    if (uniqueName.empty()) {
        cerr << "Erreur: tentative d'assignation sur la variable non declaree '" << originalName << "'." << endl;
        hasError = true;
        return TYPE_INVALID;
    }

    if (rhsType != TYPE_INT && rhsType != TYPE_INVALID) {
        cerr << "Erreur: affectation avec un type incompatible pour '" << originalName << "'." << endl;
        hasError = true;
    }

    table[uniqueName].isUsed = true;
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitMultDivModExpr(ifccParser::MultDivModExprContext *ctx) {
    int leftType = anyToExprType(visit(ctx->expr(0)));
    int rightType = anyToExprType(visit(ctx->expr(1)));

    if (leftType != TYPE_INT && leftType != TYPE_INVALID) {
        cerr << "Erreur: operateur arithmetique applique a un type incompatible." << endl;
        hasError = true;
    }
    if (rightType != TYPE_INT && rightType != TYPE_INVALID) {
        cerr << "Erreur: operateur arithmetique applique a un type incompatible." << endl;
        hasError = true;
    }

    auto rightConst = dynamic_cast<ifccParser::ConstExprContext *>(ctx->expr(1));
    string op = ctx->OP->getText();
    if (rightConst != nullptr && (op == "/" || op == "%")) {
        int value = stoi(rightConst->getText());
        if (value == 0) {
            cerr << "Warning: division ou modulo par zero." << endl;
        }
    }

    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitPreIncDecVarExpr(ifccParser::PreIncDecVarExprContext *ctx) {
    string originalName = ctx->VAR()->getText();
    string uniqueName = resolveVariable(originalName);
    if (uniqueName.empty()) {
        cerr << "Erreur: la variable '" << originalName << "' n'est pas declaree pour '"
             << ctx->OP->getText() << "'." << endl;
        hasError = true;
        return TYPE_INVALID;
    }

    table[uniqueName].isUsed = true;
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitPostIncDecVarExpr(ifccParser::PostIncDecVarExprContext *ctx) {
    string originalName = ctx->VAR()->getText();
    string uniqueName = resolveVariable(originalName);
    if (uniqueName.empty()) {
        cerr << "Erreur: la variable '" << originalName << "' n'est pas declaree pour '"
             << ctx->OP->getText() << "'." << endl;
        hasError = true;
        return TYPE_INVALID;
    }

    table[uniqueName].isUsed = true;
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitUnitaryExpr(ifccParser::UnitaryExprContext *ctx) {
    int t = anyToExprType(visit(ctx->expr()));
    if (t != TYPE_INT && t != TYPE_INVALID) {
        cerr << "Erreur: operateur unaire applique a un type incompatible." << endl;
        hasError = true;
    }
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitAddSubExpr(ifccParser::AddSubExprContext *ctx) {
    int leftType = anyToExprType(visit(ctx->expr(0)));
    int rightType = anyToExprType(visit(ctx->expr(1)));
    if (leftType != TYPE_INT && leftType != TYPE_INVALID) {
        cerr << "Erreur: operateur arithmetique applique a un type incompatible." << endl;
        hasError = true;
    }
    if (rightType != TYPE_INT && rightType != TYPE_INVALID) {
        cerr << "Erreur: operateur arithmetique applique a un type incompatible." << endl;
        hasError = true;
    }
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitCompareExpr(ifccParser::CompareExprContext *ctx) {
    int leftType = anyToExprType(visit(ctx->expr(0)));
    int rightType = anyToExprType(visit(ctx->expr(1)));
    if (leftType != TYPE_INT && leftType != TYPE_INVALID) {
        cerr << "Erreur: comparaison appliquee a un type incompatible." << endl;
        hasError = true;
    }
    if (rightType != TYPE_INT && rightType != TYPE_INVALID) {
        cerr << "Erreur: comparaison appliquee a un type incompatible." << endl;
        hasError = true;
    }
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitEqualExpr(ifccParser::EqualExprContext *ctx) {
    int leftType = anyToExprType(visit(ctx->expr(0)));
    int rightType = anyToExprType(visit(ctx->expr(1)));
    if (leftType != TYPE_INT && leftType != TYPE_INVALID) {
        cerr << "Erreur: comparaison appliquee a un type incompatible." << endl;
        hasError = true;
    }
    if (rightType != TYPE_INT && rightType != TYPE_INVALID) {
        cerr << "Erreur: comparaison appliquee a un type incompatible." << endl;
        hasError = true;
    }
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitLogicBitANDExpr(ifccParser::LogicBitANDExprContext *ctx) {
    int leftType = anyToExprType(visit(ctx->expr(0)));
    int rightType = anyToExprType(visit(ctx->expr(1)));
    if (leftType != TYPE_INT && leftType != TYPE_INVALID) {
        cerr << "Erreur: operateur '&' applique a un type incompatible." << endl;
        hasError = true;
    }
    if (rightType != TYPE_INT && rightType != TYPE_INVALID) {
        cerr << "Erreur: operateur '&' applique a un type incompatible." << endl;
        hasError = true;
    }
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitLogicBitXORExpr(ifccParser::LogicBitXORExprContext *ctx) {
    int leftType = anyToExprType(visit(ctx->expr(0)));
    int rightType = anyToExprType(visit(ctx->expr(1)));
    if (leftType != TYPE_INT && leftType != TYPE_INVALID) {
        cerr << "Erreur: operateur '^' applique a un type incompatible." << endl;
        hasError = true;
    }
    if (rightType != TYPE_INT && rightType != TYPE_INVALID) {
        cerr << "Erreur: operateur '^' applique a un type incompatible." << endl;
        hasError = true;
    }
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitLogicBitORExpr(ifccParser::LogicBitORExprContext *ctx) {
    int leftType = anyToExprType(visit(ctx->expr(0)));
    int rightType = anyToExprType(visit(ctx->expr(1)));
    if (leftType != TYPE_INT && leftType != TYPE_INVALID) {
        cerr << "Erreur: operateur '|' applique a un type incompatible." << endl;
        hasError = true;
    }
    if (rightType != TYPE_INT && rightType != TYPE_INVALID) {
        cerr << "Erreur: operateur '|' applique a un type incompatible." << endl;
        hasError = true;
    }
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitLogicANDExpr(ifccParser::LogicANDExprContext *ctx) {
    int leftType = anyToExprType(visit(ctx->expr(0)));
    int rightType = anyToExprType(visit(ctx->expr(1)));
    if (leftType != TYPE_INT && leftType != TYPE_INVALID) {
        cerr << "Erreur: operateur '&&' applique a un type incompatible." << endl;
        hasError = true;
    }
    if (rightType != TYPE_INT && rightType != TYPE_INVALID) {
        cerr << "Erreur: operateur '&&' applique a un type incompatible." << endl;
        hasError = true;
    }
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitLogicORExpr(ifccParser::LogicORExprContext *ctx) {
    int leftType = anyToExprType(visit(ctx->expr(0)));
    int rightType = anyToExprType(visit(ctx->expr(1)));
    if (leftType != TYPE_INT && leftType != TYPE_INVALID) {
        cerr << "Erreur: operateur '||' applique a un type incompatible." << endl;
        hasError = true;
    }
    if (rightType != TYPE_INT && rightType != TYPE_INVALID) {
        cerr << "Erreur: operateur '||' applique a un type incompatible." << endl;
        hasError = true;
    }
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitCallExpr(ifccParser::CallExprContext *ctx) {
    string funcName = ctx->VAR()->getText();
    int argc = (int)ctx->expr().size();

    if (argc > 6) {
        cerr << "Erreur: plus de 6 arguments non supportes." << endl;
        hasError = true;
    }

    if (funcName == "putchar") {
        if (argc != 1) {
            cerr << "Erreur: putchar attend exactement 1 argument." << endl;
            hasError = true;
        }
    } else if (funcName == "getchar") {
        if (argc != 0) {
            cerr << "Erreur: getchar n'attend aucun argument." << endl;
            hasError = true;
        }
    } else {
        auto it = functionTable.find(funcName);
        if (it == functionTable.end()) {
            cerr << "Erreur: appel a la fonction non definie '" << funcName << "'." << endl;
            hasError = true;
        } else if (it->second.arity != argc) {
            cerr << "Erreur: la fonction '" << funcName << "' attend " << it->second.arity
                 << " argument(s), recu " << argc << "." << endl;
            hasError = true;
        }

        bool isExprStatement = dynamic_cast<ifccParser::StmtContext *>(ctx->parent) != nullptr;
        if (it != functionTable.end() && it->second.returnType == ReturnType::Void && !isExprStatement) {
            cerr << "Erreur: la fonction void '" << funcName << "' ne peut pas etre utilisee comme expression." << endl;
            hasError = true;
        }
    }

    for (auto argCtx : ctx->expr()) {
        int argType = anyToExprType(visit(argCtx));
        if (argType != TYPE_INT && argType != TYPE_INVALID) {
            cerr << "Erreur: argument de type incompatible dans l'appel de fonction." << endl;
            hasError = true;
        }
    }

    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitBreak_stmt(ifccParser::Break_stmtContext *ctx) {
    (void)ctx;
    if (loopDepth == 0 && switchDepth == 0) {
        cerr << "Erreur: 'break' utilise hors d'une boucle ou d'un switch." << endl;
        hasError = true;
    }
    return 0;
}

antlrcpp::Any SymbolVisitor::visitContinue_stmt(ifccParser::Continue_stmtContext *ctx) {
    (void)ctx;
    if (loopDepth == 0) {
        cerr << "Erreur: 'continue' utilise hors d'une boucle." << endl;
        hasError = true;
    }
    return 0;
}

antlrcpp::Any SymbolVisitor::visitWhile_stmt(ifccParser::While_stmtContext *ctx) {
    int condType = anyToExprType(visit(ctx->expr()));
    if (condType != TYPE_INT && condType != TYPE_INVALID) {
        cerr << "Erreur: condition de while non entiere." << endl;
        hasError = true;
    }

    loopDepth++;
    visit(ctx->stmt());
    loopDepth--;
    return 0;
}

antlrcpp::Any SymbolVisitor::visitSwitch_stmt(ifccParser::Switch_stmtContext *ctx) {
    int swType = anyToExprType(visit(ctx->expr()));
    if (swType != TYPE_INT && swType != TYPE_INVALID) {
        cerr << "Erreur: expression de switch doit etre de type int." << endl;
        hasError = true;
    }

    set<int> seenCases;
    bool hasDefault = false;

    switchDepth++;
    for (auto part : ctx->switch_part()) {
        if (part->case_label() != nullptr) {
            int value = 0;
            auto c = part->case_label();
            if (c->INT() != nullptr) {
                value = stoi(c->INT()->getText());
            } else if (c->CHAR() != nullptr) {
                value = (int)c->CHAR()->getText()[1];
            }

            if (seenCases.find(value) != seenCases.end()) {
                cerr << "Erreur: case duplique dans switch." << endl;
                hasError = true;
            }
            seenCases.insert(value);
            continue;
        }

        if (part->default_label() != nullptr) {
            if (hasDefault) {
                cerr << "Erreur: plusieurs labels default dans un switch." << endl;
                hasError = true;
            }
            hasDefault = true;
            continue;
        }

        if (part->stmt() != nullptr) {
            visit(part->stmt());
        }
    }
    switchDepth--;
    return 0;
}
