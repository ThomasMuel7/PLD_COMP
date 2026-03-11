#include "IRVisitor.h"
#include "IR.h"
#include "SymbolTable.h"

using namespace std;

antlrcpp::Any IRVisitor::visitProg(ifccParser::ProgContext *ctx) {
    for (auto stmt : ctx->stmt()) {
        visit(stmt);
    }
    return cfg;
}

antlrcpp::Any IRVisitor::visitDeclare_stmt(ifccParser::Declare_stmtContext *ctx) {
    return 0;
}

antlrcpp::Any IRVisitor::visitAssign_stmt(ifccParser::Assign_stmtContext *ctx) {
    string rightVar = visit(ctx->expr()).as<string>();
    string varName = ctx->VAR()->getText();
    string op = ctx->OP->getText();
    char opChar = op[0];

    IRInstr::Operation irOp;
    switch (opChar) {
      case '=': irOp = IRInstr::copy; break;
      case '+': irOp = IRInstr::add; break;
      case '-': irOp = IRInstr::sub; break;
      case '*': irOp = IRInstr::mul; break;
      default: break;
    }
    current_bb->add_IRInstr(irOp, Type::Int, {varName, varName, rightVar});
    return varName;
}

antlrcpp::Any IRVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
    string expr = visit(ctx->expr()).as<string>();
    current_bb->add_IRInstr(IRInstr::ret, Type::Int, {expr});
    return expr;
}

antlrcpp::Any IRVisitor::visitAddSubExpr(ifccParser::AddSubExprContext *ctx) {
    string left = visit(ctx->expr(0)).as<string>();
    string right = visit(ctx->expr(1)).as<string>();
    string dest = createTemp();

    IRInstr::Operation op = (ctx->OP->getText() == "+") ? IRInstr::add : IRInstr::sub;
    current_bb->add_IRInstr(op, Type::Int, {dest, left, right});
    return dest;
}

antlrcpp::Any IRVisitor::visitMultDivModExpr(ifccParser::MultDivModExprContext *ctx) {
    string left = visit(ctx->expr(0)).as<string>();
    string right = visit(ctx->expr(1)).as<string>();
    string dest = createTemp();

    string op = ctx->OP->getText();
    char opChar = op[0];
    IRInstr::Operation irOp;

    switch (opChar) {
      case '*': irOp = IRInstr::mul; break;
      case '/': irOp = IRInstr::div; break;
      case '%': irOp = IRInstr::mod; break;
      default: break;
    }
    current_bb->add_IRInstr(irOp, Type::Int, {dest, left, right});
    return dest;
}

antlrcpp::Any IRVisitor::visitConstExpr(ifccParser::ConstExprContext *ctx) {
    string val;
    if (ctx->INT()) {
        val = ctx->INT()->getText();
    } else {
        val = to_string((int)ctx->CHAR()->getText()[1]);
    }
    string dest = createTemp();
    current_bb->add_IRInstr(IRInstr::ldconst, Type::Int, {dest, val});
    return dest;
}

antlrcpp::Any IRVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
    return ctx->VAR()->getText();
}

antlrcpp::Any IRVisitor::visitParensExpr(ifccParser::ParensExprContext *ctx) {
    return visit(ctx->expr());
}

antlrcpp::Any IRVisitor::visitUnitaryExpr(ifccParser::UnitaryExprContext *ctx) {
    string expr = visit(ctx->expr()).as<string>();
    string dest = createTemp();
    string op = ctx->OP->getText();
    IRInstr::Operation irOp = (op == "-") ? IRInstr::neg : IRInstr::not_;
    current_bb->add_IRInstr(irOp, Type::Int, {dest, expr});
    return dest;
}

antlrcpp::Any IRVisitor::visitCompareExpr(ifccParser::CompareExprContext *ctx) {
    string left = visit(ctx->expr(0)).as<string>();
    string right = visit(ctx->expr(1)).as<string>();
    string dest = createTemp();
    string op = ctx->OP->getText();
    IRInstr::Operation irOp;
    if (op == "<") irOp = IRInstr::cmp_lt;
    else if (op == "<=") irOp = IRInstr::cmp_le;
    else if (op == ">") irOp = IRInstr::cmp_gt;
    else if (op == ">=") irOp = IRInstr::cmp_ge;
    current_bb->add_IRInstr(irOp, Type::Int, {dest, left, right});
    return dest;
}

antlrcpp::Any IRVisitor::visitEqualExpr(ifccParser::EqualExprContext *ctx) {
    string left = visit(ctx->expr(0)).as<string>();
    string right = visit(ctx->expr(1)).as<string>();
    string dest = createTemp();
    string op = ctx->OP->getText();
    IRInstr::Operation irOp = (op == "==") ? IRInstr::cmp_eq : IRInstr::cmp_ne;
    current_bb->add_IRInstr(irOp, Type::Int, {dest, left, right});
    return dest;
}

antlrcpp::Any IRVisitor::visitLogicBitANDExpr(ifccParser::LogicBitANDExprContext *ctx) {
    string left = visit(ctx->expr(0)).as<string>();
    string right = visit(ctx->expr(1)).as<string>();
    string dest = createTemp();
    current_bb->add_IRInstr(IRInstr::and_, Type::Int, {dest, left, right});
    return dest;
}

antlrcpp::Any IRVisitor::visitLogicBitORExpr(ifccParser::LogicBitORExprContext *ctx) {
    string left = visit(ctx->expr(0)).as<string>();
    string right = visit(ctx->expr(1)).as<string>();
    string dest = createTemp();
    current_bb->add_IRInstr(IRInstr::or_, Type::Int, {dest, left, right});
    return dest;
}

antlrcpp::Any IRVisitor::visitLogicBitXORExpr(ifccParser::LogicBitXORExprContext *ctx) {
    string left = visit(ctx->expr(0)).as<string>();
    string right = visit(ctx->expr(1)).as<string>();
    string dest = createTemp();
    current_bb->add_IRInstr(IRInstr::xor_, Type::Int, {dest, left, right});
    return dest;
}