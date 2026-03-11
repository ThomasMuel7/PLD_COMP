#include "IRVisitor.h"
#include "IR.h"
#include "SymbolTable.h"

using namespace std;

IRVisitor::IRVisitor(SymbolTable& st, int offset) : table(st), currentOffset(offset) {
    cfg = new CFG(); 
    current_bb = new BasicBlock(cfg, "main_entry");
}

string IRVisitor::createTemp() {
    string tempName = "tmp" + to_string(tempCounter++);
    currentOffset -= 4; 
    table[tempName] = {currentOffset, true};
    return tempName;
}

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
      case '=':
        irOp = IRInstr::copy;
        break;
      case '+':
        irOp = IRInstr::add;
        break;
      case '-':
        irOp = IRInstr::sub;
        break;
      case '*':
        irOp = IRInstr::mul;
        break;
       default:
        break;
    }
        current_bb->add_IRInstr(irOp, Type::Int, {varName, varName, rightVar});

    return varName;
}

antlrcpp::Any IRVisitor::visitAddSubExpr(ifccParser::AddSubExprContext *ctx) {
    string left = visit(ctx->expr(0)).as<string>();
    string right = visit(ctx->expr(1)).as<string>();
    string dest = createTemp();

    IRInstr::Operation op = (ctx->OP->getText() == "+") ? IRInstr::add : IRInstr::sub;
    current_bb->add_IRInstr(op, Type::Int, {dest, left, right});
    
    return dest;
}

// Visite des multiplications/divisions/modulos
antlrcpp::Any IRVisitor::visitMultDivModExpr(ifccParser::MultDivModExprContext *ctx) {
    string left = visit(ctx->expr(0)).as<string>();
    string right = visit(ctx->expr(1)).as<string>();
    string dest = createTemp();

    string op = ctx->OP->getText();
    char opChar = op[0];
    IRInstr::Operation irOp;


    switch (opChar) {
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

// Visite des variables utilisées dans une expression
antlrcpp::Any IRVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
    return ctx->VAR()->getText();
}

// Visite des parenthèses
antlrcpp::Any IRVisitor::visitParensExpr(ifccParser::ParensExprContext *ctx) {
    return visit(ctx->expr());
}

// Visite du return
antlrcpp::Any IRVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
    string res = visit(ctx->expr()).as<string>();
    // Vous pouvez utiliser 'copy' vers une variable spéciale ou ajouter 'ret' à votre IR
    current_bb->add_IRInstr(IRInstr::copy, Type::Int, {"return_value", res});
    return 0;
}
