#include "IRVisitor.h"
#include "BasicBlock.h"
#include "IR.h"
#include "SymbolTable.h"

using namespace std;

static ReturnType parseReturnType(const string &typeText) {
    return (typeText == "void") ? ReturnType::Void : ReturnType::Int;
}

string IRVisitor::resolveVariable(const string &originalName) {
    for (int i = (int)scopeTable.size() - 1; i >= 0; i--) {
        auto it = scopeTable[i].find(originalName);
        if (it != scopeTable[i].end()) {
            return it->second;
        }
    }
    return "";
}

string IRVisitor::createTemp() {
    string tempName = "tmp" + to_string(tempCounter++);
    currentOffset -= 4;
    table[tempName] = {currentOffset, true, 0};
    return tempName;
}

string IRVisitor::gen_unique_id(antlr4::ParserRuleContext *ctx) {
    return to_string(ctx->start->getLine()) + "_" + to_string(ctx->start->getCharPositionInLine());
}

IRVisitor::IRVisitor(SymbolTable &t, const FunctionTable &ft, int startoffset)
    : cfg(nullptr), current_bb(nullptr), bb_epilogue(nullptr), table(t), functionTable(ft), currentOffset(startoffset) {}

antlrcpp::Any IRVisitor::visitProg(ifccParser::ProgContext *ctx) {
    for (auto fn : ctx->function_decl()) {
        visit(fn);
    }
    return 0;
}

antlrcpp::Any IRVisitor::visitFunction_decl(ifccParser::Function_declContext *ctx) {
    string functionName = ctx->VAR()->getText();
    ReturnType retType = parseReturnType(ctx->type()->getText());

    cfg = new CFG(functionName, retType == ReturnType::Void);
    cfgs.push_back(cfg);

    BasicBlock *bb_prologue = new BasicBlock(cfg, "prologue");
    bb_epilogue = new BasicBlock(cfg, "epilogue");
    BasicBlock *bb_body = new BasicBlock(cfg, "body");

    cfg->entry = bb_prologue;
    bb_prologue->add_exit(bb_body);

    current_bb = bb_body;
    scopeTable.push_back(map<string, string>());

    if (ctx->param_list() != nullptr) {
        for (auto p : ctx->param_list()->param()) {
            string originalName = p->VAR()->getText();
            string uniqueName = originalName + "_" + to_string(uniqueVarId++);
            scopeTable.back()[originalName] = uniqueName;
            cfg->paramVarNames.push_back(uniqueName);
        }
    }

    visit(ctx->block());

    if (current_bb->exit_true == nullptr) {
        string dest = createTemp();
        current_bb->add_IRInstr(IRInstr::ldconst, {dest, "0"});
        current_bb->add_IRInstr(IRInstr::ret, {dest});
        current_bb->add_exit(bb_epilogue);
    }

    scopeTable.pop_back();
    return 0;
}

antlrcpp::Any IRVisitor::visitBlock(ifccParser::BlockContext *ctx) {
    scopeTable.push_back(map<string, string>());
    for (auto stmt : ctx->stmt()) {
        visit(stmt);
        if (current_bb->exit_true != nullptr) {
            break;
        }
    }
    scopeTable.pop_back();
    return 0;
}

antlrcpp::Any IRVisitor::visitDeclare_stmt(ifccParser::Declare_stmtContext *ctx) {
    for (auto elmtCtx : ctx->declare_elmt()) {
        visit(elmtCtx);
    }
    return 0;
}

antlrcpp::Any IRVisitor::visitDeclare_elmt(ifccParser::Declare_elmtContext *ctx) {
    if (ctx->VAR() != nullptr) {
        string originalName = ctx->VAR()->getText();
        string uniqueName = originalName + "_" + to_string(uniqueVarId++);
        scopeTable.back()[originalName] = uniqueName;
        return 0;
    }

    if (ctx->assign_stmt() != nullptr) {
        string originalName = ctx->assign_stmt()->VAR()->getText();
        string uniqueName = originalName + "_" + to_string(uniqueVarId++);
        scopeTable.back()[originalName] = uniqueName;
        return visit(ctx->assign_stmt());
    }

    return 0;
}

antlrcpp::Any IRVisitor::visitAssign_stmt(ifccParser::Assign_stmtContext *ctx) {
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
        IRInstr::Operation irOp = IRInstr::add;
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
    IRInstr::Operation irOp = IRInstr::mul;
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
    return resolveVariable(ctx->VAR()->getText());
}

antlrcpp::Any IRVisitor::visitPreIncDecVarExpr(ifccParser::PreIncDecVarExprContext *ctx) {
    string uniqueName = resolveVariable(ctx->VAR()->getText());
    string one = createTemp();
    current_bb->add_IRInstr(IRInstr::ldconst, {one, "1"});
    if (ctx->OP->getText() == "++") {
        current_bb->add_IRInstr(IRInstr::add, {uniqueName, uniqueName, one});
    } else {
        current_bb->add_IRInstr(IRInstr::sub, {uniqueName, uniqueName, one});
    }
    return uniqueName;
}

antlrcpp::Any IRVisitor::visitPostIncDecVarExpr(ifccParser::PostIncDecVarExprContext *ctx) {
    string uniqueName = resolveVariable(ctx->VAR()->getText());
    string oldVal = createTemp();
    current_bb->add_IRInstr(IRInstr::copy, {oldVal, uniqueName});

    string one = createTemp();
    current_bb->add_IRInstr(IRInstr::ldconst, {one, "1"});
    if (ctx->OP->getText() == "++") {
        current_bb->add_IRInstr(IRInstr::add, {uniqueName, uniqueName, one});
    } else {
        current_bb->add_IRInstr(IRInstr::sub, {uniqueName, uniqueName, one});
    }
    return oldVal;
}

antlrcpp::Any IRVisitor::visitParensExpr(ifccParser::ParensExprContext *ctx) {
    return std::any_cast<string>(visit(ctx->expr()));
}

antlrcpp::Any IRVisitor::visitUnitaryExpr(ifccParser::UnitaryExprContext *ctx) {
    string expr = std::any_cast<string>(visit(ctx->expr()));
    string dest = createTemp();
    IRInstr::Operation irOp = (ctx->OP->getText() == "-") ? IRInstr::neg : IRInstr::not_;
    current_bb->add_IRInstr(irOp, {dest, expr});
    return dest;
}

antlrcpp::Any IRVisitor::visitCompareExpr(ifccParser::CompareExprContext *ctx) {
    string left = std::any_cast<string>(visit(ctx->expr(0)));
    string right = std::any_cast<string>(visit(ctx->expr(1)));
    string dest = createTemp();
    string op = ctx->OP->getText();
    IRInstr::Operation irOp = IRInstr::cmp_lt;
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
    IRInstr::Operation irOp = (ctx->OP->getText() == "==") ? IRInstr::cmp_eq : IRInstr::cmp_ne;
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
    BasicBlock *bb_rhs = new BasicBlock(cfg, "land_rhs" + uid);
    BasicBlock *bb_false = new BasicBlock(cfg, "land_false" + uid);
    BasicBlock *bb_end = new BasicBlock(cfg, "land_end" + uid);

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
    BasicBlock *bb_true = new BasicBlock(cfg, "lor_true" + uid);
    BasicBlock *bb_rhs = new BasicBlock(cfg, "lor_rhs" + uid);
    BasicBlock *bb_end = new BasicBlock(cfg, "lor_end" + uid);

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
    string expr;
    if (ctx->expr() != nullptr) {
        expr = std::any_cast<string>(visit(ctx->expr()));
    } else {
        expr = createTemp();
        current_bb->add_IRInstr(IRInstr::ldconst, {expr, "0"});
    }
    current_bb->add_IRInstr(IRInstr::ret, {expr});
    current_bb->add_exit(bb_epilogue);
    return expr;
}

antlrcpp::Any IRVisitor::visitIf_stmt(ifccParser::If_stmtContext *ctx) {
    BasicBlock *bb_cond = new BasicBlock(cfg, "if_cond" + gen_unique_id(ctx));
    BasicBlock *bb_then = new BasicBlock(cfg, "if_then" + gen_unique_id(ctx));
    BasicBlock *bb_end = new BasicBlock(cfg, "if_end" + gen_unique_id(ctx));
    BasicBlock *bb_else = nullptr;

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
    BasicBlock *bb_cond = new BasicBlock(cfg, "while_cond" + gen_unique_id(ctx));
    BasicBlock *bb_body = new BasicBlock(cfg, "while_body" + gen_unique_id(ctx));
    BasicBlock *bb_end = new BasicBlock(cfg, "while_end" + gen_unique_id(ctx));

    current_bb->add_exit(bb_cond);
    current_bb = bb_cond;
    current_bb->test_var_name = std::any_cast<string>(visit(ctx->expr()));
    current_bb->add_exit(bb_body, bb_end);

    breakTargets.push_back(bb_end);
    continueTargets.push_back(bb_cond);

    current_bb = bb_body;
    visit(ctx->stmt());
    if (current_bb->exit_true == nullptr) {
        current_bb->add_exit(bb_cond);
    }

    continueTargets.pop_back();
    breakTargets.pop_back();

    current_bb = bb_end;
    return 0;
}

antlrcpp::Any IRVisitor::visitBreak_stmt(ifccParser::Break_stmtContext *ctx) {
    (void)ctx;
    if (!breakTargets.empty()) {
        current_bb->add_exit(breakTargets.back());
    }
    return 0;
}

antlrcpp::Any IRVisitor::visitContinue_stmt(ifccParser::Continue_stmtContext *ctx) {
    (void)ctx;
    if (!continueTargets.empty()) {
        current_bb->add_exit(continueTargets.back());
    }
    return 0;
}

antlrcpp::Any IRVisitor::visitSwitch_stmt(ifccParser::Switch_stmtContext *ctx) {
    string switchVal = std::any_cast<string>(visit(ctx->expr()));
    BasicBlock *bb_end = new BasicBlock(cfg, "switch_end" + gen_unique_id(ctx));

    vector<pair<int, BasicBlock *>> caseBlocks;
    BasicBlock *defaultBlock = nullptr;
    for (auto part : ctx->switch_part()) {
        if (part->case_label() != nullptr) {
            int cval = 0;
            auto c = part->case_label();
            if (c->INT() != nullptr) {
                cval = stoi(c->INT()->getText());
            } else if (c->CHAR() != nullptr) {
                cval = (int)c->CHAR()->getText()[1];
            }
            caseBlocks.push_back({cval, new BasicBlock(cfg, "switch_case" + gen_unique_id(part))});
        } else if (part->default_label() != nullptr) {
            if (defaultBlock == nullptr) {
                defaultBlock = new BasicBlock(cfg, "switch_default" + gen_unique_id(part));
            }
        }
    }

    BasicBlock *firstDispatch = new BasicBlock(cfg, "switch_dispatch" + gen_unique_id(ctx));
    current_bb->add_exit(firstDispatch);

    BasicBlock *dispatch = firstDispatch;
    for (size_t i = 0; i < caseBlocks.size(); i++) {
        string cst = createTemp();
        dispatch->add_IRInstr(IRInstr::ldconst, {cst, to_string(caseBlocks[i].first)});
        string cond = createTemp();
        dispatch->add_IRInstr(IRInstr::cmp_eq, {cond, switchVal, cst});
        dispatch->test_var_name = cond;

        BasicBlock *nextDispatch = nullptr;
        if (i + 1 < caseBlocks.size()) {
            nextDispatch = new BasicBlock(cfg, "switch_dispatch" + gen_unique_id(ctx) + "_" + to_string(i + 1));
        } else {
            nextDispatch = (defaultBlock != nullptr) ? defaultBlock : bb_end;
        }
        dispatch->add_exit(caseBlocks[i].second, nextDispatch);
        dispatch = nextDispatch;
    }
    if (caseBlocks.empty()) {
        firstDispatch->add_exit((defaultBlock != nullptr) ? defaultBlock : bb_end);
    }

    breakTargets.push_back(bb_end);

    BasicBlock *currentCase = nullptr;
    size_t caseIdx = 0;
    bool hasActiveLabel = false;
    for (auto part : ctx->switch_part()) {
        if (part->case_label() != nullptr) {
            BasicBlock *labelBlock = caseBlocks[caseIdx++].second;
            if (hasActiveLabel && current_bb->exit_true == nullptr && current_bb != labelBlock) {
                current_bb->add_exit(labelBlock);
            }
            current_bb = labelBlock;
            currentCase = labelBlock;
            hasActiveLabel = true;
            continue;
        }
        if (part->default_label() != nullptr) {
            if (defaultBlock == nullptr) {
                defaultBlock = new BasicBlock(cfg, "switch_default_fallback" + gen_unique_id(part));
            }
            if (hasActiveLabel && current_bb->exit_true == nullptr && current_bb != defaultBlock) {
                current_bb->add_exit(defaultBlock);
            }
            current_bb = defaultBlock;
            currentCase = defaultBlock;
            hasActiveLabel = true;
            continue;
        }

        if (!hasActiveLabel) {
            continue;
        }
        if (currentCase != nullptr && current_bb->exit_true != nullptr) {
            continue;
        }
        visit(part->stmt());
    }

    if (hasActiveLabel && current_bb->exit_true == nullptr) {
        current_bb->add_exit(bb_end);
    }

    breakTargets.pop_back();
    current_bb = bb_end;
    return 0;
}
