#include "IRVisitor.h"
#include "IR.h"
#include "BasicBlock.h"
#include "SymbolTable.h"

using namespace std;

IRVisitor::IRVisitor(SymbolTable &st, int offset)
    : table(st), currentOffset(offset) {
  cfg = new CFG();
  current_bb = new BasicBlock(cfg, "main_entry");
  cfg->set_entry(current_bb);
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

antlrcpp::Any
IRVisitor::visitDeclare_stmt(ifccParser::Declare_stmtContext *ctx) {
  return 0;
}

antlrcpp::Any IRVisitor::visitAssign_stmt(ifccParser::Assign_stmtContext *ctx) {
  string rightVar = std::any_cast<string>(visit(ctx->expr()));
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

antlrcpp::Any IRVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
    string expr = visit(ctx->expr()).as<string>();
    current_bb->add_IRInstr(IRInstr::ret, Type::Int, {expr});
    return expr;
}

antlrcpp::Any IRVisitor::visitAddSubExpr(ifccParser::AddSubExprContext *ctx) {
  string left = std::any_cast<string>(visit(ctx->expr(0)));
  string right = std::any_cast<string>(visit(ctx->expr(1)));
  string dest = createTemp();

  IRInstr::Operation op =
      (ctx->OP->getText() == "+") ? IRInstr::add : IRInstr::sub;
  current_bb->add_IRInstr(op, Type::Int, {dest, left, right});

  return dest;
}

antlrcpp::Any
IRVisitor::visitMultDivModExpr(ifccParser::MultDivModExprContext *ctx) {
  string left = std::any_cast<string>(visit(ctx->expr(0)));
  string right = std::any_cast<string>(visit(ctx->expr(1)));
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

antlrcpp::Any IRVisitor::visitConstExpr(ifccParser::ConstExprContext *ctx)
{
    string val;
    if (ctx->INT())
    {
        val = ctx->INT()->getText();
    }
    else
    {
        val = ctx->CHAR()->getText();
    }
    string dest = createTemp();
    current_bb->add_IRInstr(IRInstr::ldconst, {dest, val});
    return dest;
}

antlrcpp::Any IRVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
  return ctx->VAR()->getText();
}

antlrcpp::Any IRVisitor::visitParensExpr(ifccParser::ParensExprContext *ctx) {
  return visit(ctx->expr());
}

antlrcpp::Any IRVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
  string res = std::any_cast<string>(visit(ctx->expr()));
  // Vous pouvez utiliser 'copy' vers une variable spéciale ou ajouter 'ret' à
  // votre IR
  current_bb->add_IRInstr(IRInstr::copy, Type::Int, {"return_value", res});
  return 0;
}

antlrcpp::Any IRVisitor::visitUnitaryExpr(ifccParser::UnitaryExprContext *ctx) {
  string operand = std::any_cast<string>(visit(ctx->expr()));
  string dest = createTemp();

  if (ctx->OP->getText() == "-") {
    current_bb->add_IRInstr(IRInstr::sub, Type::Int, {dest, "0", operand});
  } else if (ctx->OP->getText() == "!") {
    current_bb->add_IRInstr(IRInstr::neg, Type::Int, {dest, operand, "1"});
  }

  return dest;
}

antlrcpp::Any IRVisitor::visitCompareExpr(ifccParser::CompareExprContext *ctx) {
  string left = std::any_cast<string>(visit(ctx->expr(0)));
  string right = std::any_cast<string>(visit(ctx->expr(1)));
  string dest = createTemp();

  string op = ctx->OP->getText();
  IRInstr::Operation irOp;

  if (op == "<") {
    irOp = IRInstr::cmp_lt;
  } else if (op == ">") {
    irOp = IRInstr::cmp_gt;
  } else if (op == "<=") {
    irOp = IRInstr::cmp_le;
  } else if (op == ">=") {
    irOp = IRInstr::cmp_ge;
  }

  current_bb->add_IRInstr(irOp, Type::Int, {dest, left, right});
  return dest;
}

antlrcpp::Any IRVisitor::visitEqualExpr(ifccParser::EqualExprContext *ctx) {
  string left = std::any_cast<string>(visit(ctx->expr(0)));
  string right = std::any_cast<string>(visit(ctx->expr(1)));
  string dest = createTemp();

  string op = ctx->OP->getText();
  IRInstr::Operation irOp;

  if (op == "==") {
    irOp = IRInstr::eq;
  } else if (op == "!=") {
    irOp = IRInstr::neq;
  }

  current_bb->add_IRInstr(irOp, Type::Int, {dest, left, right});
  return dest;
}

antlrcpp::Any
IRVisitor::visitLogicBitANDExpr(ifccParser::LogicBitANDExprContext *ctx) {
  string left = std::any_cast<string>(visit(ctx->expr(0)));
  string right = std::any_cast<string>(visit(ctx->expr(1)));
  string dest = createTemp();

  current_bb->add_IRInstr(IRInstr::and_op, Type::Int, {dest, left, right});
  return dest;
}

antlrcpp::Any
IRVisitor::visitLogicBitORExpr(ifccParser::LogicBitORExprContext *ctx) {
  string left = std::any_cast<string>(visit(ctx->expr(0)));
  string right = std::any_cast<string>(visit(ctx->expr(1)));
  string dest = createTemp();

  current_bb->add_IRInstr(IRInstr::or_op, Type::Int, {dest, left, right});
  return dest;
}

antlrcpp::Any
IRVisitor::visitLogicBitXORExpr(ifccParser::LogicBitXORExprContext *ctx) {
  string left = std::any_cast<string>(visit(ctx->expr(0)));
  string right = std::any_cast<string>(visit(ctx->expr(1)));
  string dest = createTemp();

  current_bb->add_IRInstr(IRInstr::xor_op, Type::Int, {dest, left, right});
  return dest;
}
