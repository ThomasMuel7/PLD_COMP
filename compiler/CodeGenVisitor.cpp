#include "CodeGenVisitor.h"
#include <iostream>

using namespace std;

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx)
{
#ifdef __APPLE__
    cout << ".globl _main\n";
    cout << " _main: \n";
#else
    cout << ".globl main\n";
    cout << " main: \n";
#endif
    // Prologue
    cout << "    pushq %rbp\n";
    cout << "    movq %rsp, %rbp\n";

    for (auto stmt : ctx->stmt())
    {
        visit(stmt);
    }

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitDeclare_stmt(ifccParser::Declare_stmtContext *ctx)
{
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitAssign_stmt(ifccParser::Assign_stmtContext *ctx)
{
    visit(ctx->expr());

    string op = ctx->OP->getText();
    string varName = ctx->VAR()->getText();
    int offset = table[varName].index;

    if (op == "=")
        cout << "    movl %eax, " << offset << "(%rbp)\n";
    else if (op == "+=")
    {
        cout << "    movl " << offset << "(%rbp), %ecx\n";
        cout << "    addl %eax, %ecx\n";
        cout << "    movl %ecx, " << offset << "(%rbp)\n";
    }
    else if (op == "-=")
    {
        cout << "    movl " << offset << "(%rbp), %ecx\n";
        cout << "    subl %eax, %ecx\n";
        cout << "    movl %ecx, " << offset << "(%rbp)\n";
    }
    else if (op == "*=")
    {
        cout << "    movl " << offset << "(%rbp), %ecx\n";
        cout << "    imull %eax, %ecx\n";
        cout << "    movl %ecx, " << offset << "(%rbp)\n";
    }
    else if (op == "/=")
    {
        cout << "    movl %eax, %ecx\n";
        cout << "    movl " << offset << "(%rbp), %eax\n";
        cout << "    cltd\n";
        cout << "    idivl %ecx\n";
        cout << "    movl %eax, " << offset << "(%rbp)\n";
    }

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
    visit(ctx->expr());

    // Epilogue
    cout << "    popq %rbp\n";
    cout << "    ret\n";
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitMultDivModExpr(ifccParser::MultDivModExprContext *ctx)
{
    visit(ctx->expr(0));

    int tempOffset = getNewTempOffset();
    cout << "    movl %eax, " << tempOffset << "(%rbp)\n";

    visit(ctx->expr(1));

    cout << "    movl %eax, %ebx\n";
    cout << "    movl " << tempOffset << "(%rbp), %eax\n";

    freeTempOffset();

    string op = ctx->OP->getText();

    if (op == "*")
    {
        cout << "    imull %ebx, %eax\n";
    }
    else if (op == "/" || op == "%")
    {
        cout << "    cltd\n";
        cout << "    idivl %ebx\n";
        if (op == "%")
        {
            cout << "    movl %edx, %eax\n";
        }
    }
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitAddSubExpr(ifccParser::AddSubExprContext *ctx)
{
    visit(ctx->expr(0));

    int tempOffset = getNewTempOffset();
    cout << "    movl %eax, " << tempOffset << "(%rbp)\n";

    visit(ctx->expr(1));

    cout << "    movl " << tempOffset << "(%rbp), %ebx\n";
    freeTempOffset();

    string op = ctx->OP->getText();
    if (op == "+")
    {
        cout << "    addl %ebx, %eax\n";
    }
    else
    {
        cout << "    subl %eax, %ebx\n";
        cout << "    movl %ebx, %eax\n";
    }
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitParensExpr(ifccParser::ParensExprContext *ctx)
{
    return visit(ctx->expr());
}

antlrcpp::Any CodeGenVisitor::visitConstExpr(ifccParser::ConstExprContext *ctx)
{
    int val = 0;
    if (ctx->INT() != nullptr)
    {
        val = stoi(ctx->INT()->getText());
    }
    else
    {
        val = static_cast<int>(ctx->CHAR()->getText()[1]);
    }
    cout << "    movl $" << val << ", %eax\n";
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitVarExpr(ifccParser::VarExprContext *ctx)
{
    string varName = ctx->VAR()->getText();
    int offset = table[varName].index;
    cout << "    movl " << offset << "(%rbp), %eax\n";
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitUnitaryExpr(ifccParser::UnitaryExprContext *ctx)
{
    visit(ctx->expr());
    string op = ctx->OP->getText();
    if (op == "-")
    {
        cout << "    negl %eax\n";
    }
    else if (op == "!")
    {
        cout << "    xor     eax, eax\n";
        cout << "    test    edi, edi\n";
        cout << "    sete    al\n";
    }
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitCompareExpr(ifccParser::CompareExprContext *ctx)
{
    visit(ctx->expr(0));

    int tempOffset = getNewTempOffset();
    cout << "    movl %eax, " << tempOffset << "(%rbp)\n";

    visit(ctx->expr(1));

    cout << "    movl " << tempOffset << "(%rbp), %ebx\n";
    freeTempOffset();

    string op = ctx->OP->getText();
    cout << "    cmp %eax, %ebx\n";
    if (op == "<")
    {
        cout << "    setl %al\n";
        cout << "    movzb %al, %eax\n";
    }
    else if (op == "<=")
    {
        cout << "    setle %al\n";
        cout << "    movzb %al, %eax\n";
    }
    else if (op == ">")
    {
        cout << "    setg %al\n";
        cout << "    movzb %al, %eax\n";
    }
    else if (op == ">=")
    {
        cout << "    setge %al\n";
        cout << "    movzb %al, %eax\n";
    }

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitEqualExpr(ifccParser::EqualExprContext *ctx)
{
    visit(ctx->expr(0));

    int tempOffset = getNewTempOffset();
    cout << "    movl %eax, " << tempOffset << "(%rbp)\n";

    visit(ctx->expr(1));

    cout << "    movl " << tempOffset << "(%rbp), %ebx\n";
    freeTempOffset();

    string op = ctx->OP->getText();
    cout << "    cmp %eax, %ebx\n";
    if (op == "==")
    {
        cout << "    sete %al\n";
        cout << "    movzb %al, %eax\n";
    }
    else if (op == "!=")
    {
        cout << "    setne %al\n";
        cout << "    movzb %al, %eax\n";
    }

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitLogicBitANDExpr(ifccParser::LogicBitANDExprContext *ctx)
{
    visit(ctx->expr(0));

    int tempOffset = getNewTempOffset();
    cout << "    movl %eax, " << tempOffset << "(%rbp)\n";

    visit(ctx->expr(1));

    cout << "    movl " << tempOffset << "(%rbp), %ebx\n";
    freeTempOffset();

    cout << "    andl %ebx, %eax\n";

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitLogicBitXORExpr(ifccParser::LogicBitXORExprContext *ctx)
{
    visit(ctx->expr(0));

    int tempOffset = getNewTempOffset();
    cout << "    movl %eax, " << tempOffset << "(%rbp)\n";

    visit(ctx->expr(1));

    cout << "    movl " << tempOffset << "(%rbp), %ebx\n";
    freeTempOffset();

    cout << "    xorl %ebx, %eax\n";

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitLogicBitORExpr(ifccParser::LogicBitORExprContext *ctx)
{
    visit(ctx->expr(0));

    int tempOffset = getNewTempOffset();
    cout << "    movl %eax, " << tempOffset << "(%rbp)\n";

    visit(ctx->expr(1));

    cout << "    movl " << tempOffset << "(%rbp), %ebx\n";
    freeTempOffset();

    cout << "    orl %ebx, %eax\n";

    return 0;
}