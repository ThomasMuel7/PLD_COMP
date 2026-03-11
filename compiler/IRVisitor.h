#pragma once
#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "SymbolTable.h"
#include "IR.h"

static SymbolTable;

class IRVisitor : public ifccBaseVisitor {
private:
    CFG* cfg;                
    BasicBlock* current_bb;  
    SymbolTable& table;      
    int currentOffset;       
    int tempCounter = 0;     

public:
    IRVisitor(SymbolTable& t, int startoffset) : table(t), currentOffset(startoffset) {
        cfg = new CFG();
        current_bb = new BasicBlock(cfg, "entry");
    }

    CFG* getCFG() { return cfg; }

    std::string createTemp() {
        std::string name = "tmp" + std::to_string(tempCounter++);
        currentOffset -= 4; 
        table[name] = {currentOffset, true};
        return name;
    }

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
