#include "IRVisitor.h"
#include "IR.h"
#include "BasicBlock.h"
#include "SymbolTable.h"

using namespace std;

IRVisitor::IRVisitor(SymbolTable &t, int startoffset) : table(t), currentOffset(startoffset) {
    cfg = new CFG();
    current_bb = new BasicBlock(cfg, "entry");
    cfg->entry = current_bb;
    scopeTable.push_back(map<string, string>());
}

string IRVisitor::resolveVariable(const string& originalName) {
    for (int i = scopeTable.size() - 1; i >= 0; i--) {
        if (scopeTable[i].find(originalName) != scopeTable[i].end()) {
            return scopeTable[i][originalName];
        }
    }
    return originalName;
}

string IRVisitor::createTemp() {
    string tempName = "tmp" + to_string(tempCounter++);
    currentOffset -= 4;
    table[tempName] = {currentOffset, true};
    return tempName;
}

antlrcpp::Any IRVisitor::visitProg(ifccParser::ProgContext *ctx) {
    visit(ctx->block());
    return cfg;
}

antlrcpp::Any IRVisitor::visitBlock(ifccParser::BlockContext *ctx) {
    scopeTable.push_back(map<string, string>());
    for (auto stmt : ctx->stmt()) {
        visit(stmt);
    }
    scopeTable.pop_back();
    return 0;
}

antlrcpp::Any IRVisitor::visitDeclare_stmt(ifccParser::Declare_stmtContext *ctx) {
    for (auto var : ctx->VAR()) {
        string originalName = var->getText();
        string uniqueName = originalName + "_" + to_string(uniqueVarId++);
        scopeTable.back()[originalName] = uniqueName;
    }
    return 0;
}

antlrcpp::Any IRVisitor::visitAssignExpr(ifccParser::AssignExprContext *ctx) {
    string rightVar = std::any_cast<string>(visit(ctx->expr()));
    string originalName = ctx->VAR()->getText();
    string uniqueName = resolveVariable(originalName);
    string op = ctx->OP->getText();

    if (op == "=") {
        current_bb->add_IRInstr(IRInstr::copy, {uniqueName, rightVar});
    } else {
        IRInstr::Operation irOp;
        if (op == "+=") irOp = IRInstr::add;
        else if (op == "-=") irOp = IRInstr::sub;
        else if (op == "*=") irOp = IRInstr::mul;
        else if (op == "/=") irOp = IRInstr::div;
        
        current_bb->add_IRInstr(irOp, {uniqueName, uniqueName, rightVar});
    }
    return uniqueName;
}

antlrcpp::Any IRVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
    string expr = std::any_cast<string>(visit(ctx->expr()));
    current_bb->add_IRInstr(IRInstr::ret, {expr});
    return expr;
}

antlrcpp::Any IRVisitor::visitAddSubExpr(ifccParser::AddSubExprContext *ctx) {
    string left = std::any_cast<string>(visit(ctx->expr(0)));
    string right = std::any_cast<string>(visit(ctx->expr(1)));
    string dest = createTemp();
    IRInstr::Operation op = (ctx->OP->getText() == "+") ? IRInstr::add : IRInstr::sub;
    current_bb->add_IRInstr(op, {dest, left, right});
    return dest;
}

antlrcpp::Any IRVisitor::visitMultDivModExpr(ifccParser::MultDivModExprContext *ctx) {
    string left = std::any_cast<string>(visit(ctx->expr(0)));
    string right = std::any_cast<string>(visit(ctx->expr(1)));
    string dest = createTemp();
    string op = ctx->OP->getText();
    IRInstr::Operation irOp;
    if (op == "*") irOp = IRInstr::mul;
    else if (op == "/") irOp = IRInstr::div;
    else if (op == "%") irOp = IRInstr::mod;
    current_bb->add_IRInstr(irOp, {dest, left, right});
    return dest;
}

antlrcpp::Any IRVisitor::visitConstExpr(ifccParser::ConstExprContext *ctx) {
    string val;
    if (ctx->INT()) val = ctx->INT()->getText();
    else val = to_string((int)ctx->CHAR()->getText()[1]);
    string dest = createTemp();
    current_bb->add_IRInstr(IRInstr::ldconst, {dest, val});
    return dest;
}

antlrcpp::Any IRVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
    string originalName = ctx->VAR()->getText();
    return resolveVariable(originalName);
}

antlrcpp::Any IRVisitor::visitParensExpr(ifccParser::ParensExprContext *ctx) {
    return std::any_cast<string>(visit(ctx->expr()));
}

antlrcpp::Any IRVisitor::visitUnitaryExpr(ifccParser::UnitaryExprContext *ctx) {
    string expr = std::any_cast<string>(visit(ctx->expr()));
    string dest = createTemp();
    string op = ctx->OP->getText();
    IRInstr::Operation irOp = (op == "-") ? IRInstr::neg : IRInstr::not_;
    current_bb->add_IRInstr(irOp, {dest, expr});
    return dest;
}

antlrcpp::Any IRVisitor::visitCompareExpr(ifccParser::CompareExprContext *ctx) {
    string left = std::any_cast<string>(visit(ctx->expr(0)));
    string right = std::any_cast<string>(visit(ctx->expr(1)));
    string dest = createTemp();
    string op = ctx->OP->getText();
    IRInstr::Operation irOp;
    if (op == "<") irOp = IRInstr::cmp_lt;
    else if (op == "<=") irOp = IRInstr::cmp_le;
    else if (op == ">") irOp = IRInstr::cmp_gt;
    else if (op == ">=") irOp = IRInstr::cmp_ge;
    current_bb->add_IRInstr(irOp, {dest, left, right});
    return dest;
}

antlrcpp::Any IRVisitor::visitEqualExpr(ifccParser::EqualExprContext *ctx) {
    string left = std::any_cast<string>(visit(ctx->expr(0)));
    string right = std::any_cast<string>(visit(ctx->expr(1)));
    string dest = createTemp();
    string op = ctx->OP->getText();
    IRInstr::Operation irOp = (op == "==") ? IRInstr::cmp_eq : IRInstr::cmp_ne;
    current_bb->add_IRInstr(irOp, {dest, left, right});
    return dest;
}

antlrcpp::Any IRVisitor::visitLogicBitANDExpr(ifccParser::LogicBitANDExprContext *ctx) {
    string left = std::any_cast<string>(visit(ctx->expr(0)));
    string right = std::any_cast<string>(visit(ctx->expr(1)));
    string dest = createTemp();
    current_bb->add_IRInstr(IRInstr::and_, {dest, left, right});
    return dest;
}

antlrcpp::Any IRVisitor::visitLogicBitORExpr(ifccParser::LogicBitORExprContext *ctx) {
    string left = std::any_cast<string>(visit(ctx->expr(0)));
    string right = std::any_cast<string>(visit(ctx->expr(1)));
    string dest = createTemp();
    current_bb->add_IRInstr(IRInstr::or_, {dest, left, right});
    return dest;
}

antlrcpp::Any IRVisitor::visitLogicBitXORExpr(ifccParser::LogicBitXORExprContext *ctx) {
    string left = std::any_cast<string>(visit(ctx->expr(0)));
    string right = std::any_cast<string>(visit(ctx->expr(1)));
    string dest = createTemp();
    current_bb->add_IRInstr(IRInstr::xor_, {dest, left, right});
    return dest;
}