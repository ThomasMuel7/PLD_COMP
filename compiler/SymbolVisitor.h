<<<<<<< HEAD
#pragma once
#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "SymbolTable.h"

class SymbolVisitor : public ifccBaseVisitor {
public:
    SymbolTable table;
    int currentOffset = 0;
    bool hasError = false;

    virtual antlrcpp::Any visitDeclare_stmt(ifccParser::Declare_stmtContext *ctx) override;
    virtual antlrcpp::Any visitAssign_stmt(ifccParser::Assign_stmtContext *ctx) override;
    virtual antlrcpp::Any visitMultDivModExpr(ifccParser::MultDivModExprContext *ctx) override;
    virtual antlrcpp::Any visitVarExpr(ifccParser::VarExprContext *ctx) override;

    void checkUnusedVariables();
=======
#pragma once
#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "SymbolTable.h"

class SymbolVisitor : public ifccBaseVisitor
{
public:
    SymbolTable table;
    int currentOffset = 0;
    bool hasError = false;

    virtual antlrcpp::Any visitDeclare_stmt(ifccParser::Declare_stmtContext *ctx) override;
    virtual antlrcpp::Any visitAssign_stmt(ifccParser::Assign_stmtContext *ctx) override;
    virtual antlrcpp::Any visitMultDivModExpr(ifccParser::MultDivModExprContext *ctx) override;
    virtual antlrcpp::Any visitVarExpr(ifccParser::VarExprContext *ctx) override;

    void checkUnusedVariables();
>>>>>>> 6e2b3bbbfee102d95899e4843c55c1b244133002
};