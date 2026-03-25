#include "SymbolVisitor.h"
#include <iostream>
#include <string>
#include <map>
#include <algorithm>
#include <set>
#include <cctype>

using namespace std;

namespace {
    constexpr int TYPE_INT = 0;
    constexpr int TYPE_PTR = 1;
    constexpr int TYPE_INVALID = 2;

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

string SymbolVisitor::resolveVariable(const string& originalName) {
    for (int i = scopeTable.size() - 1; i >= 0; i--) {
        if (scopeTable[i].find(originalName) != scopeTable[i].end()) {
            return scopeTable[i][originalName];
        }
    }
    return "";
}

VariableInfo* SymbolVisitor::lookupVariableInfo(const string& originalName) {
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
            cerr << "Erreur: la fonction '" << name << "' est déjà définie." << endl;
            hasError = true;
            continue;
        }

        int arity = 0;
        vector<bool> paramIsPointer;
        if (fn->param_list() != nullptr) {
            arity = (int)fn->param_list()->param().size();
            for (auto p : fn->param_list()->param()) {
                string ptext = p->getText();
                paramIsPointer.push_back(ptext.find('*') != string::npos);
            }
        }

        functionTable[name] = {parseReturnType(fn->type()->getText()), arity, {}, paramIsPointer};
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
                 << "' est déclarée mais jamais utilisée";
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

    auto it = functionTable.find(currentFunctionName);
    if (it != functionTable.end()) {
        it->second.paramUniqueNames.clear();
        it->second.paramIsPointer.clear();
    }

    if (ctx->param_list() != nullptr) {
        for (auto p : ctx->param_list()->param()) {
            string originalName = p->VAR()->getText();
            bool isPointer = p->getText().find('*') != string::npos;
            if (scopeTable.back().find(originalName) != scopeTable.back().end()) {
                cerr << "Erreur: paramètre dupliqué '" << originalName << "' dans la fonction '" << currentFunctionName << "'." << endl;
                hasError = true;
                continue;
            }

            string uniqueName = originalName + "_" + to_string(uniqueVarId++);
            scopeTable.back()[originalName] = uniqueName;
            int byteSize = isPointer ? 8 : 4;
            int declLine = p->start->getLine();
            currentOffset -= byteSize;
            table[uniqueName] = {currentOffset, false, isPointer, false, 0, byteSize, declLine};
            if (it != functionTable.end()) {
                it->second.paramUniqueNames.push_back(uniqueName);
                it->second.paramIsPointer.push_back(isPointer);
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
        string dtext = decl->getText();
        int ptrCount = (int)count(dtext.begin(), dtext.end(), '*');
        bool isPointer = ptrCount > 0;
        bool isArray = decl->INT() != nullptr;
        int arrayLength = 0;

        if (isArray) {
            arrayLength = stoi(decl->INT()->getText());
            if (arrayLength <= 0) {
                cerr << "Erreur: la taille du tableau '" << originalName << "' doit être strictement positive." << endl;
                hasError = true;
                continue;
            }
        }

        if (isPointer && isArray) {
            cerr << "Erreur: combinaison pointeur+tableau non supportée pour '" << originalName << "'." << endl;
            hasError = true;
            continue;
        }

        if (scopeTable.back().find(originalName) != scopeTable.back().end()) {
            cerr << "Erreur: la variable '" << originalName << "' est déjà déclarée dans ce bloc." << endl;
            hasError = true;
        } else {
            string uniqueName = originalName + "_" + to_string(uniqueVarId++);
            scopeTable.back()[originalName] = uniqueName;
            int byteSize = isPointer ? 8 : (isArray ? (4 * arrayLength) : 4);
            int declLine = decl->start->getLine();
            currentOffset -= byteSize;
            table[uniqueName] = {currentOffset, false, isPointer, isArray, arrayLength, byteSize, declLine};
        }
    }
    return 0;
}

antlrcpp::Any SymbolVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
    if (currentFunctionReturnType == ReturnType::Void) {
        if (ctx->expr() != nullptr) {
            cerr << "Erreur: return avec expression dans une fonction void ('" << currentFunctionName << "')." << endl;
            hasError = true;
            visit(ctx->expr());
        }
        return 0;
    }

    if (ctx->expr() == nullptr) {
        cerr << "Erreur: return sans expression dans une fonction int ('" << currentFunctionName << "')." << endl;
        hasError = true;
        return 0;
    }

    int t = anyToExprType(visit(ctx->expr()));
    if (t != TYPE_INT && t != TYPE_INVALID) {
        cerr << "Erreur: return d'un pointeur dans une fonction int ('" << currentFunctionName << "')." << endl;
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
    if (uniqueName == "") {
        cerr << "Erreur: la variable '" << originalName << "' utilisée dans l'expression n'est pas déclarée." << endl;
        hasError = true;
        return TYPE_INVALID;
    } else {
        table[uniqueName].isUsed = true;
        if (table[uniqueName].isPointer || table[uniqueName].isArray) {
            return TYPE_PTR;
        }
    }

    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitAssignExpr(ifccParser::AssignExprContext *ctx) {
    string originalName = ctx->VAR()->getText();
    string uniqueName = resolveVariable(originalName);
    int rhsType = anyToExprType(visit(ctx->expr()));

    if (uniqueName == "") {
        cerr << "Erreur: tentative d'assignation sur la variable non déclarée '" << originalName << "'." << endl;
        hasError = true;
        return TYPE_INVALID;
    }

    VariableInfo &v = table[uniqueName];
    string op = ctx->OP->getText();

    if (v.isArray) {
        cerr << "Erreur: impossible d'assigner directement le tableau '" << originalName << "'." << endl;
        hasError = true;
        return TYPE_INVALID;
    }

    v.isUsed = true;

    if (op != "=") {
        if (v.isPointer) {
            cerr << "Erreur: opérateur '" << op << "' non supporté sur le pointeur '" << originalName << "'." << endl;
            hasError = true;
            return TYPE_INVALID;
        }
        if (rhsType != TYPE_INT && rhsType != TYPE_INVALID) {
            cerr << "Erreur: affectation composée avec un type incompatible pour '" << originalName << "'." << endl;
            hasError = true;
            return TYPE_INVALID;
        }
        return TYPE_INT;
    }

    if (v.isPointer) {
        if (rhsType != TYPE_PTR && rhsType != TYPE_INVALID) {
            cerr << "Erreur: affectation d'une valeur non pointeur dans '" << originalName << "'." << endl;
            hasError = true;
        }
        return TYPE_PTR;
    }

    if (rhsType != TYPE_INT && rhsType != TYPE_INVALID) {
        cerr << "Erreur: affectation d'un pointeur dans l'entier '" << originalName << "'." << endl;
        hasError = true;
    }
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitArrayAssignExpr(ifccParser::ArrayAssignExprContext *ctx) {
    string originalName = ctx->VAR()->getText();
    string uniqueName = resolveVariable(originalName);
    if (uniqueName.empty()) {
        cerr << "Erreur: tableau/pointeur non déclaré '" << originalName << "'." << endl;
        hasError = true;
        return TYPE_INVALID;
    }

    VariableInfo &v = table[uniqueName];
    if (!v.isArray && !v.isPointer) {
        cerr << "Erreur: l'expression '" << originalName << "[...]' requiert un tableau ou pointeur." << endl;
        hasError = true;
    }

    int indexType = anyToExprType(visit(ctx->expr(0)));
    int rhsType = anyToExprType(visit(ctx->expr(1)));
    if (indexType != TYPE_INT && indexType != TYPE_INVALID) {
        cerr << "Erreur: l'indice de '" << originalName << "' doit être un int." << endl;
        hasError = true;
    }
    if (rhsType != TYPE_INT && rhsType != TYPE_INVALID) {
        cerr << "Erreur: l'écriture dans '" << originalName << "[...]' attend une valeur int." << endl;
        hasError = true;
    }

    v.isUsed = true;
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitDerefAssignExpr(ifccParser::DerefAssignExprContext *ctx) {
    int lhsType = anyToExprType(visit(ctx->expr(0)));
    int rhsType = anyToExprType(visit(ctx->expr(1)));
    if (lhsType != TYPE_PTR) {
        cerr << "Erreur: l'affectation '*expr = ...' requiert un pointeur à gauche." << endl;
        hasError = true;
    }
    if (rhsType != TYPE_INT) {
        cerr << "Erreur: l'écriture via pointeur attend une valeur int." << endl;
        hasError = true;
    }
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitMultDivModExpr(ifccParser::MultDivModExprContext *ctx) {
    int leftType = anyToExprType(visit(ctx->expr(0)));
    if (leftType != TYPE_INT && leftType != TYPE_INVALID) {
        cerr << "Erreur: opérateur arithmétique appliqué à un pointeur." << endl;
        hasError = true;
    }

    auto rightConst = dynamic_cast<ifccParser::ConstExprContext*>(ctx->expr(1));
    string op = ctx->OP->getText();
    if (rightConst != nullptr && (op == "/" || op == "%")) {
        int value = stoi(rightConst->getText());
        if (value == 0) {
            cerr << "Warning: division ou modulo par zéro." << endl;
        }
    }

    int rightType = anyToExprType(visit(ctx->expr(1)));
    if (rightType != TYPE_INT && rightType != TYPE_INVALID) {
        cerr << "Erreur: opérateur arithmétique appliqué à un pointeur." << endl;
        hasError = true;
    }

    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitAddrExpr(ifccParser::AddrExprContext *ctx) {
    string originalName = ctx->VAR()->getText();
    string uniqueName = resolveVariable(originalName);
    if (uniqueName.empty()) {
        cerr << "Erreur: variable non déclarée pour '&' : '" << originalName << "'." << endl;
        hasError = true;
        return TYPE_INVALID;
    }
    table[uniqueName].isUsed = true;
    return TYPE_PTR;
}

antlrcpp::Any SymbolVisitor::visitPreIncDecVarExpr(ifccParser::PreIncDecVarExprContext *ctx) {
    string originalName = ctx->VAR()->getText();
    string uniqueName = resolveVariable(originalName);
    if (uniqueName.empty()) {
        cerr << "Erreur: la variable '" << originalName << "' n'est pas déclarée pour '" << ctx->OP->getText() << "'." << endl;
        hasError = true;
        return TYPE_INVALID;
    }

    VariableInfo &v = table[uniqueName];
    if (v.isPointer || v.isArray) {
        cerr << "Erreur: opérateur '" << ctx->OP->getText() << "' non supporté sur '" << originalName << "'." << endl;
        hasError = true;
        return TYPE_INVALID;
    }
    v.isUsed = true;
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitPostIncDecVarExpr(ifccParser::PostIncDecVarExprContext *ctx) {
    string originalName = ctx->VAR()->getText();
    string uniqueName = resolveVariable(originalName);
    if (uniqueName.empty()) {
        cerr << "Erreur: la variable '" << originalName << "' n'est pas déclarée pour '" << ctx->OP->getText() << "'." << endl;
        hasError = true;
        return TYPE_INVALID;
    }

    VariableInfo &v = table[uniqueName];
    if (v.isPointer || v.isArray) {
        cerr << "Erreur: opérateur '" << ctx->OP->getText() << "' non supporté sur '" << originalName << "'." << endl;
        hasError = true;
        return TYPE_INVALID;
    }
    v.isUsed = true;
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitArrayAccessExpr(ifccParser::ArrayAccessExprContext *ctx) {
    string originalName = ctx->VAR()->getText();
    string uniqueName = resolveVariable(originalName);
    if (uniqueName.empty()) {
        cerr << "Erreur: tableau/pointeur non déclaré '" << originalName << "'." << endl;
        hasError = true;
        visit(ctx->expr());
        return TYPE_INVALID;
    }

    VariableInfo &v = table[uniqueName];
    if (!v.isArray && !v.isPointer) {
        cerr << "Erreur: l'expression '" << originalName << "[...]' requiert un tableau ou pointeur." << endl;
        hasError = true;
    }

    int idxType = anyToExprType(visit(ctx->expr()));
    if (idxType != TYPE_INT && idxType != TYPE_INVALID) {
        cerr << "Erreur: l'indice de '" << originalName << "' doit être un int." << endl;
        hasError = true;
    }
    v.isUsed = true;
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitUnitaryExpr(ifccParser::UnitaryExprContext *ctx) {
    string op = ctx->OP->getText();
    int t = anyToExprType(visit(ctx->expr()));

    if (op == "*") {
        if (t != TYPE_PTR) {
            cerr << "Erreur: déréférencement d'une expression non pointeur." << endl;
            hasError = true;
        }
        return TYPE_INT;
    }

    if (t != TYPE_INT) {
        cerr << "Erreur: opérateur unaire '" << op << "' appliqué à un pointeur." << endl;
        hasError = true;
    }
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitAddSubExpr(ifccParser::AddSubExprContext *ctx) {
    int leftType = anyToExprType(visit(ctx->expr(0)));
    int rightType = anyToExprType(visit(ctx->expr(1)));

    if (leftType == TYPE_INT && rightType == TYPE_INT) {
        return TYPE_INT;
    }
    if ((leftType == TYPE_PTR && rightType == TYPE_INT) || (leftType == TYPE_INT && rightType == TYPE_PTR)) {
        return TYPE_PTR;
    }
    if (leftType == TYPE_PTR && rightType == TYPE_PTR) {
        cerr << "Erreur: addition/soustraction entre deux pointeurs non supportée." << endl;
        hasError = true;
        return TYPE_INVALID;
    }
    return TYPE_INVALID;
}

antlrcpp::Any SymbolVisitor::visitCompareExpr(ifccParser::CompareExprContext *ctx) {
    int leftType = anyToExprType(visit(ctx->expr(0)));
    int rightType = anyToExprType(visit(ctx->expr(1)));
    if (leftType != TYPE_INT && leftType != TYPE_INVALID) {
        cerr << "Erreur: comparaison relationnelle appliquée à un pointeur." << endl;
        hasError = true;
    }
    if (rightType != TYPE_INT && rightType != TYPE_INVALID) {
        cerr << "Erreur: comparaison relationnelle appliquée à un pointeur." << endl;
        hasError = true;
    }
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitEqualExpr(ifccParser::EqualExprContext *ctx) {
    int leftType = anyToExprType(visit(ctx->expr(0)));
    int rightType = anyToExprType(visit(ctx->expr(1)));
    if (leftType != TYPE_INVALID && rightType != TYPE_INVALID && leftType != rightType) {
        cerr << "Erreur: comparaison entre types incompatibles." << endl;
        hasError = true;
    }
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitLogicBitANDExpr(ifccParser::LogicBitANDExprContext *ctx) {
    int leftType = anyToExprType(visit(ctx->expr(0)));
    int rightType = anyToExprType(visit(ctx->expr(1)));
    if (leftType != TYPE_INT && leftType != TYPE_INVALID) {
        cerr << "Erreur: opérateur '&' appliqué à un pointeur." << endl;
        hasError = true;
    }
    if (rightType != TYPE_INT && rightType != TYPE_INVALID) {
        cerr << "Erreur: opérateur '&' appliqué à un pointeur." << endl;
        hasError = true;
    }
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitLogicBitXORExpr(ifccParser::LogicBitXORExprContext *ctx) {
    int leftType = anyToExprType(visit(ctx->expr(0)));
    int rightType = anyToExprType(visit(ctx->expr(1)));
    if (leftType != TYPE_INT && leftType != TYPE_INVALID) {
        cerr << "Erreur: opérateur '^' appliqué à un pointeur." << endl;
        hasError = true;
    }
    if (rightType != TYPE_INT && rightType != TYPE_INVALID) {
        cerr << "Erreur: opérateur '^' appliqué à un pointeur." << endl;
        hasError = true;
    }
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitLogicBitORExpr(ifccParser::LogicBitORExprContext *ctx) {
    int leftType = anyToExprType(visit(ctx->expr(0)));
    int rightType = anyToExprType(visit(ctx->expr(1)));
    if (leftType != TYPE_INT && leftType != TYPE_INVALID) {
        cerr << "Erreur: opérateur '|' appliqué à un pointeur." << endl;
        hasError = true;
    }
    if (rightType != TYPE_INT && rightType != TYPE_INVALID) {
        cerr << "Erreur: opérateur '|' appliqué à un pointeur." << endl;
        hasError = true;
    }
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitLogicANDExpr(ifccParser::LogicANDExprContext *ctx) {
    int leftType = anyToExprType(visit(ctx->expr(0)));
    int rightType = anyToExprType(visit(ctx->expr(1)));
    if (leftType != TYPE_INT && leftType != TYPE_INVALID) {
        cerr << "Erreur: opérateur '&&' appliqué à un pointeur." << endl;
        hasError = true;
    }
    if (rightType != TYPE_INT && rightType != TYPE_INVALID) {
        cerr << "Erreur: opérateur '&&' appliqué à un pointeur." << endl;
        hasError = true;
    }
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitLogicORExpr(ifccParser::LogicORExprContext *ctx) {
    int leftType = anyToExprType(visit(ctx->expr(0)));
    int rightType = anyToExprType(visit(ctx->expr(1)));
    if (leftType != TYPE_INT && leftType != TYPE_INVALID) {
        cerr << "Erreur: opérateur '||' appliqué à un pointeur." << endl;
        hasError = true;
    }
    if (rightType != TYPE_INT && rightType != TYPE_INVALID) {
        cerr << "Erreur: opérateur '||' appliqué à un pointeur." << endl;
        hasError = true;
    }
    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitCallExpr(ifccParser::CallExprContext *ctx) {
    string funcName = ctx->VAR()->getText();
    int argc = (int)ctx->expr().size();

    if (argc > 6) {
        cerr << "Erreur: plus de 6 arguments non supportés." << endl;
        hasError = true;
    }

    if (funcName == "putchar" && argc != 1) {
        cerr << "Erreur: putchar attend exactement 1 argument." << endl;
        hasError = true;
    }
    else if (funcName == "getchar" && argc != 0) {
        cerr << "Erreur: getchar n'attend aucun argument." << endl;
        hasError = true;
    }
    else if (funcName != "putchar" && funcName != "getchar") {
        auto it = functionTable.find(funcName);
        if (it == functionTable.end()) {
            cerr << "Erreur: appel à la fonction non définie '" << funcName << "'." << endl;
            hasError = true;
        } else if (it->second.arity != argc) {
            cerr << "Erreur: la fonction '" << funcName << "' attend " << it->second.arity
                 << " argument(s), reçu " << argc << "." << endl;
            hasError = true;
        }

        bool isExprStatement = dynamic_cast<ifccParser::StmtContext *>(ctx->parent) != nullptr;
        if (it != functionTable.end() && it->second.returnType == ReturnType::Void && !isExprStatement) {
            cerr << "Erreur: la fonction void '" << funcName << "' ne peut pas être utilisée comme expression." << endl;
            hasError = true;
        }

        if (it != functionTable.end() && (int)it->second.paramIsPointer.size() == argc) {
            for (int i = 0; i < argc; i++) {
                int argType = anyToExprType(visit(ctx->expr(i)));
                bool expectedPtr = it->second.paramIsPointer[i];
                if (argType == TYPE_INVALID) {
                    continue;
                }
                if (expectedPtr && argType != TYPE_PTR) {
                    cerr << "Erreur: argument " << (i + 1) << " de '" << funcName << "' doit être un pointeur." << endl;
                    hasError = true;
                }
                if (!expectedPtr && argType != TYPE_INT) {
                    cerr << "Erreur: argument " << (i + 1) << " de '" << funcName << "' doit être un int." << endl;
                    hasError = true;
                }
            }
            return TYPE_INT;
        }
    }

    for (auto argCtx : ctx->expr()) {
        visit(argCtx);
    }

    return TYPE_INT;
}

antlrcpp::Any SymbolVisitor::visitBreak_stmt(ifccParser::Break_stmtContext *ctx) {
    (void)ctx;
    if (loopDepth == 0 && switchDepth == 0) {
        cerr << "Erreur: 'break' utilisé hors d'une boucle ou d'un switch." << endl;
        hasError = true;
    }
    return 0;
}

antlrcpp::Any SymbolVisitor::visitContinue_stmt(ifccParser::Continue_stmtContext *ctx) {
    (void)ctx;
    if (loopDepth == 0) {
        cerr << "Erreur: 'continue' utilisé hors d'une boucle." << endl;
        hasError = true;
    }
    return 0;
}

antlrcpp::Any SymbolVisitor::visitWhile_stmt(ifccParser::While_stmtContext *ctx) {
    int condType = anyToExprType(visit(ctx->expr()));
    if (condType != TYPE_INT && condType != TYPE_INVALID) {
        cerr << "Erreur: condition de while non entière." << endl;
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
        cerr << "Erreur: expression de switch doit être de type int." << endl;
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
                cerr << "Erreur: case dupliqué dans switch." << endl;
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