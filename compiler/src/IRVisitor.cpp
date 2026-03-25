#include "IRVisitor.h"
#include "IR.h"
#include "BasicBlock.h"
#include "SymbolTable.h"

using namespace std;

string IRVisitor::resolveVariable(const string& originalName) {
    for (int i = scopeTable.size() - 1; i >= 0; i--) {
        if (scopeTable[i].find(originalName) != scopeTable[i].end()) {
            return scopeTable[i][originalName];
        }
    }
    return "";
}

string IRVisitor::createTemp() {
    string tempName = "tmp" + to_string(tempCounter++);
    currentOffset -= 4;
    table[tempName] = {currentOffset, true};
    return tempName;
}

string IRVisitor::gen_unique_id(antlr4::ParserRuleContext *ctx){
        return std::to_string(ctx->start->getLine()) + "_" + std::to_string(ctx->start->getCharPositionInLine());
    }

IRVisitor::IRVisitor(SymbolTable &t, int startoffset) : table(t), currentOffset(startoffset) {
    cfg = new CFG();
    BasicBlock* bb_prologue = new BasicBlock(cfg, "prologue");
    bb_epilogue = new BasicBlock(cfg, "epilogue");
    BasicBlock* bb_body = new BasicBlock(cfg, "body");
    
    cfg->entry = bb_prologue;
    bb_prologue->add_exit(bb_body);
    
    current_bb = bb_body;
    scopeTable.push_back(map<string, string>());
}

antlrcpp::Any IRVisitor::visitProg(ifccParser::ProgContext *ctx) {
    visit(ctx->block());
    if (current_bb->exit_true == nullptr) {
        string dest = createTemp();
        current_bb->add_IRInstr(IRInstr::ldconst, {dest, "0"});
        current_bb->add_IRInstr(IRInstr::ret, {dest});
        current_bb->add_exit(bb_epilogue);
    }
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
    for (auto elmtCtx : ctx->declare_elmt())
    {
        visit(elmtCtx);
    }
    return 0;
}

antlrcpp::Any IRVisitor::visitDeclare_elmt(ifccParser::Declare_elmtContext *ctx)
{
    if (ctx->VAR())
    {
        string originalName = ctx->VAR()->getText();
        string uniqueName = originalName + "_" + to_string(uniqueVarId++);
        scopeTable.back()[originalName] = uniqueName;
    }
    if (ctx->assign_stmt())
    {
        string originalName = ctx->assign_stmt()->VAR()->getText();
        string uniqueName = originalName + "_" + to_string(uniqueVarId++);
        scopeTable.back()[originalName] = uniqueName;
        visit(ctx->assign_stmt());
    }
    return 0;
}

antlrcpp::Any IRVisitor::visitAssign_stmt(ifccParser::Assign_stmtContext *ctx)
{
    string rightVar = std::any_cast<string>(visit(ctx->expr()));
    string originalName = ctx->VAR()->getText();
    string uniqueName = resolveVariable(originalName);
    current_bb->add_IRInstr(IRInstr::copy, {uniqueName, rightVar});

    return uniqueName;
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

antlrcpp::Any IRVisitor::visitCallExpr(ifccParser::CallExprContext *ctx) {
    string funcName = ctx->VAR()->getText();

    vector<string> params;
    string dest = createTemp();

    params.push_back(funcName);
    params.push_back(dest);

    for (auto argCtx : ctx->expr()) {
        string argVar = std::any_cast<string>(visit(argCtx));
        params.push_back(argVar);
    }
    int argc = (int)ctx->expr().size();
    current_bb->add_IRInstr(IRInstr::call, params);
    return dest;
}

antlrcpp::Any IRVisitor::visitLogicANDExpr(ifccParser::LogicANDExprContext *ctx) {
    string dest = createTemp();
    string zero = createTemp();
    current_bb->add_IRInstr(IRInstr::ldconst, {zero, "0"});

    string left = std::any_cast<string>(visit(ctx->expr(0)));
    string leftBool = createTemp();
    current_bb->add_IRInstr(IRInstr::cmp_ne, {leftBool, left, zero});

    string uid = gen_unique_id(ctx);
    BasicBlock* bb_rhs = new BasicBlock(cfg, "land_rhs" + uid);
    BasicBlock* bb_false = new BasicBlock(cfg, "land_false" + uid);
    BasicBlock* bb_end = new BasicBlock(cfg, "land_end" + uid);

    current_bb->test_var_name = leftBool;
    current_bb->add_exit(bb_rhs, bb_false);

    current_bb = bb_false;
    current_bb->add_IRInstr(IRInstr::ldconst, {dest, "0"});
    current_bb->add_exit(bb_end);

    current_bb = bb_rhs;
    string right = std::any_cast<string>(visit(ctx->expr(1)));
    string rightBool = createTemp();
    current_bb->add_IRInstr(IRInstr::cmp_ne, {rightBool, right, zero});
    current_bb->add_IRInstr(IRInstr::copy, {dest, rightBool});
    current_bb->add_exit(bb_end);

    current_bb = bb_end;
    return dest;
}

antlrcpp::Any IRVisitor::visitLogicORExpr(ifccParser::LogicORExprContext *ctx) {
    string dest = createTemp();
    string zero = createTemp();
    current_bb->add_IRInstr(IRInstr::ldconst, {zero, "0"});

    string left = std::any_cast<string>(visit(ctx->expr(0)));
    string leftBool = createTemp();
    current_bb->add_IRInstr(IRInstr::cmp_ne, {leftBool, left, zero});

    string uid = gen_unique_id(ctx);
    BasicBlock* bb_true = new BasicBlock(cfg, "lor_true" + uid);
    BasicBlock* bb_rhs = new BasicBlock(cfg, "lor_rhs" + uid);
    BasicBlock* bb_end = new BasicBlock(cfg, "lor_end" + uid);

    current_bb->test_var_name = leftBool;
    current_bb->add_exit(bb_true, bb_rhs);

    current_bb = bb_true;
    current_bb->add_IRInstr(IRInstr::ldconst, {dest, "1"});
    current_bb->add_exit(bb_end);

    current_bb = bb_rhs;
    string right = std::any_cast<string>(visit(ctx->expr(1)));
    string rightBool = createTemp();
    current_bb->add_IRInstr(IRInstr::cmp_ne, {rightBool, right, zero});
    current_bb->add_IRInstr(IRInstr::copy, {dest, rightBool});
    current_bb->add_exit(bb_end);

    current_bb = bb_end;
    return dest;
}

antlrcpp::Any IRVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
    string expr = std::any_cast<string>(visit(ctx->expr()));
    current_bb->add_IRInstr(IRInstr::ret, {expr});
    current_bb->add_exit(bb_epilogue);
    return expr;
}

antlrcpp::Any IRVisitor::visitIf_stmt(ifccParser::If_stmtContext *ctx) {
    BasicBlock* bb_cond = new BasicBlock(cfg, "if_cond" + gen_unique_id(ctx));
    BasicBlock* bb_then = new BasicBlock(cfg, "if_then" + gen_unique_id(ctx));
    BasicBlock* bb_end = new BasicBlock(cfg, "if_end" + gen_unique_id(ctx));
    BasicBlock* bb_else = nullptr;
    
    current_bb->add_exit(bb_cond);
    current_bb = bb_cond;
    current_bb->test_var_name = std::any_cast<string>(visit(ctx->expr()));

    if (ctx->stmt().size() > 1) {
        bb_else = new BasicBlock(cfg, "if_else" + gen_unique_id(ctx));
        current_bb->add_exit(bb_then, bb_else); 
    } else {
        current_bb->add_exit(bb_then, bb_end);
    }

    current_bb = bb_then;
    visit(ctx->stmt(0));
    if (current_bb->exit_true == nullptr) {
        current_bb->add_exit(bb_end);
    }

    if (ctx->stmt().size() > 1) {
        current_bb = bb_else;
        visit(ctx->stmt(1));
        if (current_bb->exit_true == nullptr) {
            current_bb->add_exit(bb_end);
        }
    }

    current_bb = bb_end;
    return 0;
}

antlrcpp::Any IRVisitor::visitWhile_stmt(ifccParser::While_stmtContext *ctx) {
    BasicBlock* bb_cond = new BasicBlock(cfg, "while_cond" + gen_unique_id(ctx));
    BasicBlock* bb_body = new BasicBlock(cfg, "while_body" + gen_unique_id(ctx));
    BasicBlock* bb_end = new BasicBlock(cfg, "while_end" + gen_unique_id(ctx));

    current_bb->add_exit(bb_cond);
    current_bb = bb_cond;
    current_bb->test_var_name = std::any_cast<string>(visit(ctx->expr()));
    current_bb->add_exit(bb_body, bb_end);

    current_bb = bb_body;
    visit(ctx->stmt());
    if (current_bb->exit_true == nullptr) {
        current_bb->add_exit(bb_cond);
    }

    current_bb = bb_end;
    return 0;
}