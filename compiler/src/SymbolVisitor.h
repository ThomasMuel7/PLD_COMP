#pragma once
#include "antlr4-runtime.h"
#include "../generated/ifccBaseVisitor.h"
#include "SymbolTable.h"
#include "ScopeTable.h"

class SymbolVisitor : public ifccBaseVisitor {
public:
    SymbolVisitor();

    SymbolTable table;
    ScopeTable scopeTable; 
    
    int currentOffset = 0;
    int uniqueVarId = 0;
    bool hasError = false;

    std::string resolveVariable(const std::string& originalName);

    virtual antlrcpp::Any visitBlock(ifccParser::BlockContext *ctx) override;
    virtual antlrcpp::Any visitDeclare_stmt(ifccParser::Declare_stmtContext *ctx) override;
    virtual antlrcpp::Any visitAssignExpr(ifccParser::AssignExprContext *ctx) override;
    virtual antlrcpp::Any visitMultDivModExpr(ifccParser::MultDivModExprContext *ctx) override;
    virtual antlrcpp::Any visitVarExpr(ifccParser::VarExprContext *ctx) override;
    virtual antlrcpp::Any visitCallExpr(ifccParser::CallExprContext *ctx) override;


    void checkUnusedVariables();
};