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

} // namespace nyaa
    
} // namespace mai
