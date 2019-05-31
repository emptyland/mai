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

} // namespace nyaa
    
} // namespace mai
