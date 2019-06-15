#include "nyaa/code-gen-base.h"

namespace mai {

namespace nyaa {

FunctionScope::FunctionScope(CodeGeneratorVisitor *owns)
    : owns_(DCHECK_NOTNULL(owns))
    , kpool_builder_(owns->core_->factory()) {
    prev_ = owns_->fun_scope_;
    owns_->fun_scope_ = this;
    
    if (prev_) {
        level_ = prev_->level_ + 1;
    }
}

FunctionScope::~FunctionScope() {
    DCHECK_EQ(this, owns_->fun_scope_);
    owns_->fun_scope_ = prev_;
}

IVal FunctionScope::GetVariable(const ast::String *name) {
    BlockScope *blk = current_;
    while (blk && blk->owns() == this) {
        IVal val = blk->GetVariable(name);
        if (val.kind != IVal::kVoid) {
            return val;
        }
        blk = blk->prev();
    }
    auto iter = upvals_.find(name);
    if (iter != upvals_.end()) {
        return iter->second;
    }
    return IVal::Void();
}

IVal FunctionScope::GetOrNewUpvalNested(const ast::String *name) {
    IVal val = GetVariable(name);
    if (val.kind != IVal::kVoid) {
        return val;
    }
    if (prev_) {
        val = prev_->GetOrNewUpvalNested(name);
        if (val.kind == IVal::kLocal) {
            val = NewUpval(name, true, val.index);
        } else if (val.kind == IVal::kUpval) {
            val = NewUpval(name, false, val.index);
        }
        return val;
    }
    return IVal::Void();
}

BlockScope::BlockScope(FunctionScope *owns)
    : owns_(DCHECK_NOTNULL(owns))
    , active_vars_(0) {

    prev_ = owns_->owns_->blk_scope_;
    owns_->owns_->blk_scope_ = this;
    if (!owns_->top_) {
        owns_->top_ = this;
    }
    owns_->current_ = this;
}

BlockScope::~BlockScope() {
    DCHECK_EQ(this, owns_->owns_->blk_scope_);
    owns_->owns_->blk_scope_ = prev_;
    if (owns_->top_ == this) {
        owns_->top_ = nullptr;
    }
    owns_->current_ = prev_;
    
    owns_->active_vars_ -= active_vars_;
    owns_->free_reg_ = owns_->active_vars_;
    
    //    printf("*****************\n");
    //    for (const auto &var : vars_) {
    //        printf("var: %s = %d\n", var.first->data(), var.second.index);
    //    }
}
    
IVal BlockScope::PutVariable(const ast::String *name, const IVal *val) {
    IVal rv = IVal::Void();
    if (ast::IsPlaceholder(name)) {
        return rv; // ignore!
    }
    if (val) {
        rv = *val;
    } else {
        rv = owns_->NewLocal();
    }
    DCHECK(rv.kind != IVal::kLocal || rv.index < owns_->max_stack());
    vars_.insert({name, rv});
    //DCHECK_EQ(rv.index, active_vars_);
    active_vars_++;
    owns_->active_vars_++;
    return rv;
}
    
/*virtual*/ IVal CodeGeneratorVisitor::VisitBlock(ast::Block *node, ast::VisitorContext */*ctx*/) {
    BlockScope scope(fun_scope_);
    CodeGeneratorContext ctx;
    IVal ret;
    if (node->stmts()) {
        for (auto stmt : *node->stmts()) {
            ret = stmt->Accept(this, &ctx);
            if (ret.kind == IVal::kLocal) {
                fun_scope_->FreeVar(ret);
            }
        }
    }
    return IVal::Void();
}
    
/*virtual*/ IVal CodeGeneratorVisitor::VisitVarDeclaration(ast::VarDeclaration *node,
                                                           ast::VisitorContext *x) {
    std::vector<IVal> vars;
    
    if (node->inits()) {
        CodeGeneratorContext rix;
        if (node->names()->size() > 1 && node->GetNWanted() < 0) {
            rix.set_n_result(static_cast<int>(node->names()->size()));
            IVal rval = node->inits()->at(0)->Accept(this, &rix);
            fun_scope_->Reserve(rix.n_result() - 1);
            
            DCHECK_EQ(IVal::kLocal, rval.kind);
            for (size_t i = 0; i < node->names()->size(); ++i) {
                blk_scope_->PutVariable(node->names()->at(i), &rval);
                rval.index++;
            }
        } else {
            rix.set_n_result(1);
            
            for (size_t i = 0; i < node->names()->size(); ++i) {
                if (i < node->inits()->size()) {
                    ast::Expression *init = node->inits()->at(i);
                    IVal rval = init->Accept(this, &rix);
                    blk_scope_->PutVariable(node->names()->at(i), &rval);
                } else {
                    IVal lval = blk_scope_->PutVariable(node->names()->at(i), nullptr);
                    LoadNil(lval, 1, node->line());
                }
            }
        }
    } else {
        for (size_t i = 0; i < node->names()->size(); ++i) {
            const ast::String *name = node->names()->at(i);
            vars.push_back(blk_scope_->PutVariable(name, nullptr));
        }
        LoadNil(vars[0], static_cast<int32_t>(vars.size()), node->line());
    }
    return IVal::Void();
}
    
/*virtual*/ IVal CodeGeneratorVisitor::VisitVariable(ast::Variable *node, ast::VisitorContext *x) {
    CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
    
    IVal val = fun_scope_->GetOrNewUpvalNested(node->name());
    if (ctx->lval()) {
        switch (val.kind) {
            case IVal::kVoid:
                val = IVal::Global(fun_scope_->kpool()->GetOrNewStr(node->name()));
                StoreGlobal(ctx->rval(), val, node->line());
                break;
            case IVal::kUpval:
                StoreUp(ctx->rval(), val, node->line());
                break;
            default:
                DCHECK_EQ(IVal::kLocal, val.kind);
                Move(val, ctx->rval(), node->line());
                break;
        }
        return IVal::Void();
    } else {
        switch (val.kind) {
            case IVal::kVoid:
                val = IVal::Global(fun_scope_->kpool()->GetOrNewStr(node->name()));
                break;
            case IVal::kLocal:
            case IVal::kUpval:
                break;
            default:
                break;
        }
        if (ctx->localize()) {
            return Localize(val, node->line());
        } else {
            return val;
        }
    }
}
    
/*virtual*/ IVal CodeGeneratorVisitor::VisitAssignment(ast::Assignment *node,
                                                       ast::VisitorContext *x) {
    CodeGeneratorContext rix;
    if (node->rvals()->size() == 1 && node->GetNWanted() < 0) {
        rix.set_n_result(static_cast<int>(node->lvals()->size()));
        IVal val = node->rvals()->at(0)->Accept(this, &rix);
        for (size_t i = 0; i < node->lvals()->size(); ++i) {
            
            CodeGeneratorContext lix;
            lix.set_rval(val);
            lix.set_lval(true);
            node->lvals()->at(i)->Accept(this, &lix);
            val.index++;
        }
    } else {
        rix.set_n_result(1);
        size_t len = std::min(node->lvals()->size(), node->rvals()->size());
        for (size_t i = 0; i < len; ++i) {
            ast::Expression *expr = node->rvals()->at(i);
            IVal val = expr->Accept(this, &rix);
            
            CodeGeneratorContext lix;
            lix.set_rval(val);
            lix.set_lval(true);
            node->lvals()->at(i)->Accept(this, &lix);
            fun_scope_->FreeVar(val);
        }
    }
    return IVal::Void();
}
    
/*virtual*/ IVal CodeGeneratorVisitor::VisitIndex(ast::Index *node, ast::VisitorContext *x) {
    CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
    IVal val = ctx->lval() ? IVal::Void() : fun_scope_->NewLocal();
    
    CodeGeneratorContext ix;
    ix.set_n_result(1);
    IVal self = node->self()->Accept(this, &ix);
    ix.set_keep_const(true);
    IVal index = node->index()->Accept(this, &ix);
    
    if (ctx->lval()) {
        SetField(self, index, ctx->rval(), node->line());
        fun_scope_->FreeVar(index);
        fun_scope_->FreeVar(self);
        return IVal::Void();
    } else {
        GetField(val, self, index, node->line());
        fun_scope_->FreeVar(index);
        fun_scope_->FreeVar(self);
        return val;
    }
}

/*virtual*/ IVal CodeGeneratorVisitor::VisitDotField(ast::DotField *node, ast::VisitorContext *x) {
    CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
    IVal val = ctx->lval() ? IVal::Void() : fun_scope_->NewLocal();
    
    CodeGeneratorContext ix;
    ix.set_n_result(1);
    IVal self = node->self()->Accept(this, &ix);
    IVal index = IVal::Const(fun_scope_->kpool()->GetOrNewStr(node->index()));
    
    if (ctx->lval()) {
        SetField(self, index, ctx->rval(), node->line());
        fun_scope_->FreeVar(index);
        //fun_scope_->FreeVar(self);
        return IVal::Void();
    } else {
        GetField(val, self, index, node->line());
        fun_scope_->FreeVar(index);
        fun_scope_->FreeVar(self);
        return val;
    }
}
    
/*virtual*/ IVal CodeGeneratorVisitor::VisitCall(ast::Call *node, ast::VisitorContext *x) {
    int32_t n_args = node->GetNArgs();
    CodeGeneratorContext ix;
    ix.set_n_result(1);
    
    IVal callee = node->callee()->Accept(this, &ix);
    if (blk_scope_->Protected(callee)) {
        IVal tmp = fun_scope_->NewLocal();
        Move(tmp, callee, node->callee()->line());
        callee = tmp;
    }
    
    ix.set_n_result(n_args < 0 ? -1 : 1);
    int reg = callee.index;
    //std::vector<IVal> args;
    for (size_t i = 0; node->args() && i < node->args()->size(); ++i) {
        ast::Expression *expr = node->args()->at(i);
        AdjustStackPosition(++reg, expr->Accept(this, &ix), expr->line());
    }
    
    CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
    Call(callee, n_args, ctx->n_result(), node->line());
    
    fun_scope_->set_free_reg(callee.index + 1);
    return callee;
}

/*virtual*/ IVal CodeGeneratorVisitor::VisitReturn(ast::Return *node, ast::VisitorContext *x) {
    if (!node->rets()) {
        Ret(IVal::Local(0), 0, node->line());
        return IVal::Void();
    }
    
    int32_t n_rets = node->GetNRets();
    CodeGeneratorContext ix;
    ix.set_n_result(n_rets < 0 ? -1 : 1);
    
    std::vector<IVal> rets;
    IVal first = node->rets()->at(0)->Accept(this, &ix);
    rets.push_back(first);
    int32_t reg = first.index;
    
    for (size_t i = 1; i < node->rets()->size(); ++i) {
        ast::Expression *expr = node->rets()->at(i);
        rets.push_back(AdjustStackPosition(++reg, expr->Accept(this, &ix), expr->line()));
    }
    Ret(first, n_rets, node->line());
    
    for (int64_t i = rets.size() - 1; i >= 0; --i) {
        fun_scope_->FreeVar(rets[i]);
    }
    return IVal::Void();
}
    
/*virtual*/ IVal CodeGeneratorVisitor::VisitNilLiteral(ast::NilLiteral *node, ast::VisitorContext *) {
    IVal tmp = fun_scope_->NewLocal();
    LoadNil(tmp, 1, node->line());
    return tmp;
}

/*virtual*/ IVal CodeGeneratorVisitor::VisitStringLiteral(ast::StringLiteral *node,
                                                          ast::VisitorContext *x) {
    CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
    IVal val = IVal::Const(fun_scope_->kpool()->GetOrNewStr(node->value()));
    if (ctx->localize() && !ctx->keep_const()) {
        return Localize(val, node->line());
    }
    return val;
}

/*virtual*/ IVal CodeGeneratorVisitor::VisitApproxLiteral(ast::ApproxLiteral *node,
                                                          ast::VisitorContext *x) {
    CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
    IVal val = IVal::Const(fun_scope_->kpool()->GetOrNewF64(node->value()));
    if (ctx->localize() && !ctx->keep_const()) {
        return Localize(val, node->line());
    }
    return val;
}
    
/*virtual*/ IVal CodeGeneratorVisitor::VisitSmiLiteral(ast::SmiLiteral *node,
                                                       ast::VisitorContext *x) {
    CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
    IVal val = IVal::Const(fun_scope_->kpool()->GetOrNewSmi(node->value()));
    if (ctx->localize() && !ctx->keep_const()) {
        return Localize(val, node->line());
    }
    return val;
}

/*virtual*/ IVal CodeGeneratorVisitor::VisitIntLiteral(ast::IntLiteral *node,
                                                       ast::VisitorContext *x) {
    CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
    IVal val = IVal::Const(fun_scope_->kpool()->GetOrNewInt(node->value()));
    if (ctx->localize() && !ctx->keep_const()) {
        return Localize(val, node->line());
    }
    return val;
}

/*virtual*/ IVal CodeGeneratorVisitor::VisitMapInitializer(ast::MapInitializer *node,
                                                           ast::VisitorContext *x) {
    int index = 0;
    if (!node->value()) {
        IVal map = fun_scope_->NewLocal();
        NewMap(map, 0/*n*/, 0/*linear*/, node->line());
        return map;
    }
    bool linear = true;
    for (auto entry : *node->value()) {
        if (entry->key()) {
            linear = false;
            break;
        }
    }
    
    std::vector<IVal> kvs;
    CodeGeneratorContext ix;
    ix.set_n_result(1);
    ix.set_localize(true);
    ix.set_keep_const(false);
    
    if (linear) {
        for (auto entry : *node->value()) {
            IVal value = entry->value()->Accept(this, &ix);
            if (blk_scope_->Protected(value)) {
                IVal tmp = fun_scope_->NewLocal();
                Move(tmp, value, entry->value()->line());
                value = tmp;
            }
            kvs.push_back(value);
        }
    } else {
        for (auto entry : *node->value()) {
            IVal key;
            if (entry->key()) {
                key = entry->key()->Accept(this, &ix);
            } else {
                key = IVal::Const(fun_scope_->kpool()->GetOrNewSmi(index++));
                key = Localize(key, entry->value()->line());
            }
            IVal value = entry->value()->Accept(this, &ix);
            if (blk_scope_->Protected(value)) {
                IVal tmp = fun_scope_->NewLocal();
                Move(tmp, value, entry->value()->line());
                value = tmp;
            }
            kvs.push_back(key);
            kvs.push_back(value);
        }
    }
    
    IVal map = kvs.front();
    NewMap(map, static_cast<int>(kvs.size()), linear, node->line());
    for (int64_t i = kvs.size() - 1; i > 0; --i) {
        fun_scope_->FreeVar(kvs[i]);
    }
    return map;
}
    
IVal CodeGeneratorVisitor::AdjustStackPosition(int requried, IVal val, int line) {
    DCHECK_EQ(IVal::kLocal, val.kind);
    if (requried != val.index) {
        IVal dst = fun_scope_->NewLocal();
        DCHECK_EQ(requried, dst.index);
        Move(dst, val, line);
        return dst;
    }
    return val;
}

} // namespace nyaa
    
} // namespace mai
