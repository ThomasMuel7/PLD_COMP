#pragma once
#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "SymbolTable.h"

class CodeGenVisitor : public ifccBaseVisitor {
private:
    SymbolTable table;
    int currentOffset;

public:
    CodeGenVisitor(SymbolTable t, int startoffset) : table(t), currentOffset(startoffset) {}

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

    int getNewTempOffset() {
        currentOffset -= 4;
        return currentOffset;
    }

    void freeTempOffset() {
        currentOffset += 4;
    }
};