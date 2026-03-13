#pragma once
#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "SymbolTable.h"
#include "IR.h"
#include "BasicBlock.h"
<<<<<<< HEAD


class IRVisitor : public ifccBaseVisitor {
private:
    CFG* cfg;                
    BasicBlock* current_bb;  
    SymbolTable& table;      
    int currentOffset;       
    int tempCounter = 0;     

public:
    IRVisitor(SymbolTable& t, int startoffset);

    CFG* getCFG() { return cfg; }
=======
#include "CFG.h"

class IRVisitor : public ifccBaseVisitor
{
private:
    CFG *cfg;
    BasicBlock *current_bb;
    SymbolTable &table;
    int currentOffset;
    int tempCounter = 0;

public:
    IRVisitor(SymbolTable &t, int startoffset);

    CFG *getCFG() { return cfg; }
>>>>>>> 6e2b3bbbfee102d95899e4843c55c1b244133002
    int getCurrentOffset() const { return currentOffset; }

    std::string createTemp();

    virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;
    virtual antlrcpp::Any visitDeclare_stmt(ifccParser::Declare_stmtContext *ctx) override;
    virtual antlrcpp::Any visitAssign_stmt(ifccParser::Assign_stmtContext *ctx) override;
    virtual antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;
    virtual antlrcpp::Any visitMultDivModExpr(ifccParser::MultDivModExprContext *ctx) override;
    virtual antlrcpp::Any visitAddSubExpr(ifccParser::AddSubExprContext *ctx) override;
    virtual antlrcpp::Any visitParensExpr(ifccParser::ParensExprContext *ctx) override;
    virtual antlrcpp::Any visitConstExpr(ifccParser::ConstExprContext *ctx) override;
    virtual antlrcpp::Any visitVarExpr(ifccParser::VarExprContext *ctx) override;
    virtual antlrcpp::Any visitUnitaryExpr(ifccParser::UnitaryExprContext *ctx) override;
    virtual antlrcpp::Any visitCompareExpr(ifccParser::CompareExprContext *ctx) override;
    virtual antlrcpp::Any visitEqualExpr(ifccParser::EqualExprContext *ctx) override;
    virtual antlrcpp::Any visitLogicBitANDExpr(ifccParser::LogicBitANDExprContext *ctx) override;
    virtual antlrcpp::Any visitLogicBitORExpr(ifccParser::LogicBitORExprContext *ctx) override;
    virtual antlrcpp::Any visitLogicBitXORExpr(ifccParser::LogicBitXORExprContext *ctx) override;
};