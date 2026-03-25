#pragma once
#include "antlr4-runtime.h"
#include "../generated/ifccBaseVisitor.h"
#include "SymbolTable.h"
#include "ScopeTable.h"

class SymbolVisitor : public ifccBaseVisitor {
public:
    SymbolVisitor();

    SymbolTable table;
    FunctionTable functionTable;
    ScopeTable scopeTable; 
    
    int currentOffset = 0;
    int uniqueVarId = 0;
    bool hasError = false;
    std::string currentFunctionName;
    ReturnType currentFunctionReturnType = ReturnType::Int;
    int loopDepth = 0;
    int switchDepth = 0;

    std::string resolveVariable(const std::string& originalName);
    void registerVariable(const std::string &originalName, int declLine);
    VariableInfo* lookupVariableInfo(const std::string& originalName);

    virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;
    virtual antlrcpp::Any visitFunction_decl(ifccParser::Function_declContext *ctx) override;
    virtual antlrcpp::Any visitBlock(ifccParser::BlockContext *ctx) override;
    virtual antlrcpp::Any visitDeclare_stmt(ifccParser::Declare_stmtContext *ctx) override;
    virtual antlrcpp::Any visitDeclare_elmt(ifccParser::Declare_elmtContext *ctx) override;
    virtual antlrcpp::Any visitAssign_stmt(ifccParser::Assign_stmtContext *ctx) override;
    virtual antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;
    virtual antlrcpp::Any visitParensExpr(ifccParser::ParensExprContext *ctx) override;
    virtual antlrcpp::Any visitConstExpr(ifccParser::ConstExprContext *ctx) override;
    virtual antlrcpp::Any visitAssignExpr(ifccParser::AssignExprContext *ctx) override;
    virtual antlrcpp::Any visitMultDivModExpr(ifccParser::MultDivModExprContext *ctx) override;
    virtual antlrcpp::Any visitVarExpr(ifccParser::VarExprContext *ctx) override;
    virtual antlrcpp::Any visitPreIncDecVarExpr(ifccParser::PreIncDecVarExprContext *ctx) override;
    virtual antlrcpp::Any visitPostIncDecVarExpr(ifccParser::PostIncDecVarExprContext *ctx) override;
    virtual antlrcpp::Any visitUnitaryExpr(ifccParser::UnitaryExprContext *ctx) override;
    virtual antlrcpp::Any visitAddSubExpr(ifccParser::AddSubExprContext *ctx) override;
    virtual antlrcpp::Any visitCompareExpr(ifccParser::CompareExprContext *ctx) override;
    virtual antlrcpp::Any visitEqualExpr(ifccParser::EqualExprContext *ctx) override;
    virtual antlrcpp::Any visitLogicBitANDExpr(ifccParser::LogicBitANDExprContext *ctx) override;
    virtual antlrcpp::Any visitLogicBitXORExpr(ifccParser::LogicBitXORExprContext *ctx) override;
    virtual antlrcpp::Any visitLogicBitORExpr(ifccParser::LogicBitORExprContext *ctx) override;
    virtual antlrcpp::Any visitLogicANDExpr(ifccParser::LogicANDExprContext *ctx) override;
    virtual antlrcpp::Any visitLogicORExpr(ifccParser::LogicORExprContext *ctx) override;
    virtual antlrcpp::Any visitCallExpr(ifccParser::CallExprContext *ctx) override;
    virtual antlrcpp::Any visitBreak_stmt(ifccParser::Break_stmtContext *ctx) override;
    virtual antlrcpp::Any visitContinue_stmt(ifccParser::Continue_stmtContext *ctx) override;
    virtual antlrcpp::Any visitWhile_stmt(ifccParser::While_stmtContext *ctx) override;
    virtual antlrcpp::Any visitSwitch_stmt(ifccParser::Switch_stmtContext *ctx) override;

    void checkUnusedVariables();
};