#include "IRVisitor.h"
#include "IR.h"
#include "SymbolTable.h"

using namespace std;

IRVisitor::IRVisitor(SymbolTable &t, int startoffset) : table(t), currentOffset(startoffset)
{
    cfg = new CFG();
    current_bb = new BasicBlock(cfg, "entry");
    cfg->entry = current_bb;
}

string IRVisitor::createTemp()
{
    string tempName = "tmp" + to_string(tempCounter++);
    currentOffset -= 4;
    table[tempName] = {currentOffset, true};
    return tempName;
}

antlrcpp::Any IRVisitor::visitProg(ifccParser::ProgContext *ctx)
{
    for (auto stmt : ctx->stmt())
    {
        visit(stmt);
    }
    return cfg;
}

antlrcpp::Any IRVisitor::visitDeclare_stmt(ifccParser::Declare_stmtContext *ctx)
{
    return 0;
}

antlrcpp::Any IRVisitor::visitAssign_stmt(ifccParser::Assign_stmtContext *ctx)
{
    string rightVar = std::any_cast<string>(visit(ctx->expr()));
    string varName = ctx->VAR()->getText();
    string op = ctx->OP->getText();

    if (op == "=")
    {
        current_bb->add_IRInstr(IRInstr::copy, {varName, rightVar});
    }
    else
    {
        IRInstr::Operation irOp;
        if (op == "+=")
            irOp = IRInstr::add;
        else if (op == "-=")
            irOp = IRInstr::sub;
        else if (op == "*=")
            irOp = IRInstr::mul;
        else if (op == "/=")
            irOp = IRInstr::div;
        current_bb->add_IRInstr(irOp, {varName, varName, rightVar});
    }
    return varName;
}

antlrcpp::Any IRVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
    string expr = std::any_cast<string>(visit(ctx->expr()));
    current_bb->add_IRInstr(IRInstr::ret, {expr});
    return expr;
}

antlrcpp::Any IRVisitor::visitAddSubExpr(ifccParser::AddSubExprContext *ctx)
{
    string left = std::any_cast<string>(visit(ctx->expr(0)));
    string right = std::any_cast<string>(visit(ctx->expr(1)));
    string dest = createTemp();

    IRInstr::Operation op = (ctx->OP->getText() == "+") ? IRInstr::add : IRInstr::sub;
    current_bb->add_IRInstr(op, {dest, left, right});
    return dest;
}

antlrcpp::Any IRVisitor::visitMultDivModExpr(ifccParser::MultDivModExprContext *ctx)
{
    string left = std::any_cast<string>(visit(ctx->expr(0)));
    string right = std::any_cast<string>(visit(ctx->expr(1)));
    string dest = createTemp();

    string op = ctx->OP->getText();
    char opChar = op[0];
    IRInstr::Operation irOp;

    switch (opChar)
    {
    case '*':
        irOp = IRInstr::mul;
        break;
    case '/':
        irOp = IRInstr::div;
        break;
    case '%':
        irOp = IRInstr::mod;
        break;
    default:
        break;
    }
    current_bb->add_IRInstr(irOp, {dest, left, right});
    return dest;
}

antlrcpp::Any IRVisitor::visitConstExpr(ifccParser::ConstExprContext *ctx)
{
    string val;
    if (ctx->INT())
    {
        val = ctx->INT()->getText();
    }
    else
    {
        val = to_string((int)ctx->CHAR()->getText()[1]);
    }
    string dest = createTemp();
    current_bb->add_IRInstr(IRInstr::ldconst, {dest, val});
    return dest;
}

antlrcpp::Any IRVisitor::visitVarExpr(ifccParser::VarExprContext *ctx)
{
    return ctx->VAR()->getText();
}

antlrcpp::Any IRVisitor::visitParensExpr(ifccParser::ParensExprContext *ctx)
{
    return std::any_cast<string>(visit(ctx->expr()));
}

antlrcpp::Any IRVisitor::visitUnitaryExpr(ifccParser::UnitaryExprContext *ctx)
{
    string expr = std::any_cast<string>(visit(ctx->expr()));
    string dest = createTemp();
    string op = ctx->OP->getText();

    IRInstr::Operation irOp = (op == "-") ? IRInstr::neg : IRInstr::not_;
    current_bb->add_IRInstr(irOp, {dest, expr});
    return dest;
}

antlrcpp::Any IRVisitor::visitCompareExpr(ifccParser::CompareExprContext *ctx)
{
    string left = std::any_cast<string>(visit(ctx->expr(0)));
    string right = std::any_cast<string>(visit(ctx->expr(1)));
    string dest = createTemp();
    string op = ctx->OP->getText();

    IRInstr::Operation irOp;
    if (op == "<")
        irOp = IRInstr::cmp_lt;
    else if (op == "<=")
        irOp = IRInstr::cmp_le;
    else if (op == ">")
        irOp = IRInstr::cmp_gt;
    else if (op == ">=")
        irOp = IRInstr::cmp_ge;

    current_bb->add_IRInstr(irOp, {dest, left, right});
    return dest;
}

antlrcpp::Any IRVisitor::visitEqualExpr(ifccParser::EqualExprContext *ctx)
{
    string left = std::any_cast<string>(visit(ctx->expr(0)));
    string right = std::any_cast<string>(visit(ctx->expr(1)));
    string dest = createTemp();
    string op = ctx->OP->getText();

    IRInstr::Operation irOp = (op == "==") ? IRInstr::cmp_eq : IRInstr::cmp_ne;
    current_bb->add_IRInstr(irOp, {dest, left, right});
    return dest;
}

antlrcpp::Any IRVisitor::visitLogicBitANDExpr(ifccParser::LogicBitANDExprContext *ctx)
{
    string left = std::any_cast<string>(visit(ctx->expr(0)));
    string right = std::any_cast<string>(visit(ctx->expr(1)));
    string dest = createTemp();
    current_bb->add_IRInstr(IRInstr::and_, {dest, left, right});
    return dest;
}

antlrcpp::Any IRVisitor::visitLogicBitORExpr(ifccParser::LogicBitORExprContext *ctx)
{
    string left = std::any_cast<string>(visit(ctx->expr(0)));
    string right = std::any_cast<string>(visit(ctx->expr(1)));
    string dest = createTemp();
    current_bb->add_IRInstr(IRInstr::or_, {dest, left, right});
    return dest;
}

antlrcpp::Any IRVisitor::visitLogicBitXORExpr(ifccParser::LogicBitXORExprContext *ctx)
{
    string left = std::any_cast<string>(visit(ctx->expr(0)));
    string right = std::any_cast<string>(visit(ctx->expr(1)));
    string dest = createTemp();
    current_bb->add_IRInstr(IRInstr::xor_, {dest, left, right});
    return dest;
}