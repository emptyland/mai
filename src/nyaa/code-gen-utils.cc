#include "nyaa/code-gen-utils.h"

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
    
/*virtual*/ IVal CodeGeneratorVisitor::VisitObjectDefinition(ast::ObjectDefinition *node,
                                                             ast::VisitorContext *x) {
    IVal clazz = DefineClass(node, nullptr);
    New(clazz, clazz, 0, node->end_line());
    if (node->local()) {
        blk_scope_->PutVariable(node->name(), &clazz);
    } else {
        fun_scope_->FreeVar(clazz);
        IVal key = IVal::Global(fun_scope_->kpool()->GetOrNewStr(node->name()));
        StoreGlobal(clazz, key, node->end_line());
    }
    return IVal::Void();
}

/*virtual*/ IVal CodeGeneratorVisitor::VisitClassDefinition(ast::ClassDefinition *node,
                                                            ast::VisitorContext *) {
    IVal clazz = DefineClass(node, node->base());
    if (node->local()) {
        blk_scope_->PutVariable(node->name(), &clazz);
    } else {
        fun_scope_->FreeVar(clazz);
        IVal key = IVal::Global(fun_scope_->kpool()->GetOrNewStr(node->name()));
        StoreGlobal(clazz, key, node->end_line());
    }
    return IVal::Void();
}
    
/*virtual*/ IVal CodeGeneratorVisitor::VisitFunctionDefinition(ast::FunctionDefinition *node,
                                                               ast::VisitorContext *x) {
    // TODO: for object or class scope.
    CodeGeneratorContext rix;
    rix.set_localize(false);
    rix.set_keep_const(true);
    IVal rval = node->literal()->Accept(this, &rix);
    DCHECK_EQ(IVal::kFunction, rval.kind);
    
    Handle<NyFunction> lambda = fun_scope_->proto(rval.index);
    lambda->SetName(core_->factory()->NewString(node->name()->data(),
                                                node->name()->size()), core_);
    IVal closure = fun_scope_->NewLocal();
    Closure(closure, rval, node->line());
    if (node->self()) {
        CodeGeneratorContext lix;
        lix.set_n_result(1);
        IVal self = node->self()->Accept(this, &lix);
        IVal index = IVal::Const(fun_scope_->kpool()->GetOrNewStr(node->name()));
        SetField(self, index, closure, node->line(), node->trace_id());
        fun_scope_->FreeVar(self);
        fun_scope_->FreeVar(index);
    } else {
        if (fun_scope_->prev() == nullptr) {
            // as global variable
            IVal lval = IVal::Global(fun_scope_->kpool()->GetOrNewStr(node->name()));
            StoreGlobal(closure, lval, node->line());
        } else {
            // as local variable
            blk_scope_->PutVariable(node->name(), &closure);
        }
    }
    fun_scope_->FreeVar(closure);
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
        IVal base = node->rvals()->at(0)->Accept(this, &rix);
        IVal val = base;
        for (size_t i = 0; i < node->lvals()->size(); ++i) {
            
            CodeGeneratorContext lix;
            lix.set_rval(val);
            lix.set_lval(true);
            node->lvals()->at(i)->Accept(this, &lix);
            val.index++;
        }
        if (!fun_scope_->Protected(base)) {
            fun_scope_->FreeVar(base);
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
        SetField(self, index, ctx->rval(), node->line(), node->trace_id());
        fun_scope_->FreeVar(index);
        fun_scope_->FreeVar(self);
        return IVal::Void();
    } else {
        GetField(val, self, index, node->line(), node->trace_id());
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
        SetField(self, index, ctx->rval(), node->line(), node->trace_id());
        //fun_scope_->FreeVar(index);
        fun_scope_->FreeVar(self);
        return IVal::Void();
    } else {
        GetField(val, self, index, node->line(), node->trace_id());
        fun_scope_->FreeVar(index);
        fun_scope_->FreeVar(self);
        return val;
    }
}
    
/*virtual*/ IVal CodeGeneratorVisitor::VisitSelfCall(ast::SelfCall *node, ast::VisitorContext *x) {
    int32_t n_args = node->GetNArgs();
    CodeGeneratorContext ix;
    ix.set_n_result(1);
    
    IVal base = fun_scope_->NewLocal();
    IVal self = fun_scope_->NewLocal();
    IVal callee = node->callee()->Accept(this, &ix);
    IVal method = IVal::Const(fun_scope_->kpool()->GetOrNewStr(node->method()));
    Self(base, callee, method, node->line(), node->trace_id());
    fun_scope_->FreeVar(callee);
    
    ix.set_n_result(n_args < 0 ? -1 : 1);
    int reg = base.index + 1;
    for (size_t i = 0; node->args() && i < node->args()->size(); ++i) {
        ast::Expression *expr = node->args()->at(i);
        AdjustStackPosition(++reg, expr->Accept(this, &ix), expr->line());
    }
    (void)self;
    
    CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
    int wanted = ctx->n_result() == 0 ? 1 : ctx->n_result();
    Call(base, n_args + 1/*for self*/, wanted, node->line(), node->trace_id());
    
    fun_scope_->set_free_reg(base.index + 1);
    return base;
}
    
/*virtual*/ IVal CodeGeneratorVisitor::VisitNew(ast::New *node, ast::VisitorContext *x) {
    int32_t n_args = node->GetNArgs();
    CodeGeneratorContext ix;
    
    ix.set_n_result(1);
    IVal val = fun_scope_->NewLocal();
    IVal clazz = node->callee()->Accept(this, &ix);
    
    ix.set_n_result(n_args < 0 ? -1 : 1);
    int reg = clazz.index;
    for (size_t i = 0; node->args() && i < node->args()->size(); ++i) {
        ast::Expression *expr = node->args()->at(i);
        AdjustStackPosition(++reg, expr->Accept(this, &ix), expr->line());
    }
    
    New(val, clazz, n_args, node->line());
    //fun_scope_->FreeVar(clazz);
    fun_scope_->set_free_reg(clazz.index);
    return val;
}

/*virtual*/ IVal CodeGeneratorVisitor::VisitCall(ast::Call *node, ast::VisitorContext *x) {
    int32_t n_args = node->GetNArgs();
    CodeGeneratorContext ix;
    ix.set_n_result(1);
    
    IVal callee = node->callee()->Accept(this, &ix);
    if (fun_scope_->Protected(callee)) {
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
    int wanted = ctx->n_result() == 0 ? 1 : ctx->n_result();
    Call(callee, n_args, wanted, node->line(), node->trace_id());
    
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
    
/*virtual*/ IVal CodeGeneratorVisitor::VisitVariableArguments(ast::VariableArguments *node,
                                                              ast::VisitorContext *x) {
    CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
    IVal vargs = fun_scope_->NewLocal();
    Vargs(vargs, ctx->n_result(), node->line());
    return vargs;
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
    if (!ctx->localize() || ctx->keep_const()) {
        return IVal::Const(fun_scope_->kpool()->GetOrNewSmi(node->value()));
    }

    IVal val;
    if (node->value() < std::numeric_limits<int32_t>::max() &&
        node->value() > std::numeric_limits<int32_t>::min()) {
        val = fun_scope_->NewLocal();
        LoadImm(val, static_cast<int32_t>(node->value()), node->line());
    } else {
        val = IVal::Const(fun_scope_->kpool()->GetOrNewSmi(node->value()));
        val = Localize(val, node->line());
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
    
/*virtual*/ IVal CodeGeneratorVisitor::VisitConcat(ast::Concat *node, ast::VisitorContext *x) {
    CodeGeneratorContext ix;
    ix.set_n_result(1);
    
    IVal base = IVal::Local(fun_scope_->free_reg());
    int32_t reg = base.index;
    std::vector<IVal> ops;
    for (auto op : *node->operands()) {
        IVal val = AdjustStackPosition(reg++, op->Accept(this, &ix), op->line());
        ops.push_back(val);
    }
    
    for (int64_t i = ops.size() -1; i >= 1; --i) {
        fun_scope_->FreeVar(ops[i]);
    }
    Concat(base, base, static_cast<int32_t>(ops.size()), node->line());
    return base;
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
            if (fun_scope_->Protected(value)) {
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
            if (fun_scope_->Protected(value)) {
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
    
IVal CodeGeneratorVisitor::DefineClass(ast::ObjectDefinition *node, ast::Expression *base) {
    std::vector<IVal> kvs;
    size_t index = 0;
    if (node->members()) {
        FieldTable fields;
        for (auto stmt : *node->members()) {
            if (ast::PropertyDeclaration *decl = stmt->ToPropertyDeclaration()) {
                index = DeclareClassProperies(decl, index, &fields, &kvs);
            } else if (ast::FunctionDefinition *func = stmt->ToFunctionDefinition()) {
                DefineClassMethod(node->name(), func, &kvs);
            } else {
                DLOG(FATAL) << "noreached!";
            }
        }
    }
    auto bkz = core_->bkz_pool();
    {
        IVal key = IVal::Const(fun_scope_->kpool()->GetOrNewStr(bkz->kInnerType));
        kvs.push_back(Localize(key, node->line()));
        IVal val = IVal::Const(fun_scope_->kpool()->GetOrNewStr(node->name()));
        kvs.push_back(Localize(val, node->line()));
    }
    if (base) {
        IVal key = IVal::Const(fun_scope_->kpool()->GetOrNewStr(bkz->kInnerBase));
        kvs.push_back(Localize(key, node->line()));
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        IVal val = base->Accept(this, &ix);
        if (fun_scope_->Protected(val)) {
            IVal tmp = fun_scope_->NewLocal();
            Move(tmp, val, base->line());
            val = tmp;
        }
        kvs.push_back(Localize(val, node->line()));
    }
    
    IVal clazz = kvs.front();
    NewMap(clazz, static_cast<int>(kvs.size()), -1/*linear*/, node->end_line());
    for (int64_t i = kvs.size() - 1; i > 0; --i) {
        fun_scope_->FreeVar(kvs[i]);
    }
    return clazz;
}

size_t CodeGeneratorVisitor::DeclareClassProperies(ast::PropertyDeclaration *decl, size_t offset,
                                                   FieldTable *fields, std::vector<IVal> *kvs) {
    for (auto nm : *decl->names()) {
        int64_t tag = decl->readonly() ? 0x1 : 0x3;
        auto iter = fields->find(nm);
        if (iter == fields->end()) {
            tag |= (offset++ << 2);
        } else {
            tag |= (iter->second << 2);
        }
        
        IVal key = IVal::Const(fun_scope_->kpool()->GetOrNewStr(nm));
        kvs->push_back(Localize(key, decl->line()));
        IVal val = fun_scope_->NewLocal();
        DCHECK_LT(tag, INT32_MAX);
        LoadImm(val, static_cast<int32_t>(tag), decl->line());
        kvs->push_back(val);
    }
    return offset;
}

void CodeGeneratorVisitor::DefineClassMethod(const ast::String *class_name,
                                             ast::FunctionDefinition *node,
                                             std::vector<IVal> *kvs) {
    CodeGeneratorContext rix;
    rix.set_localize(false);
    rix.set_keep_const(true);
    IVal fn = node->literal()->Accept(this, &rix);
    DCHECK_EQ(IVal::kFunction, fn.kind);
    
    Handle<NyFunction> lambda = fun_scope_->proto(fn.index);
    Handle<NyString> name = core_->factory()->Sprintf("%s::%s", class_name->data(),
                                                      node->name()->data());
    lambda->SetName(*name, core_);
    
    IVal key = IVal::Const(fun_scope_->kpool()->GetOrNewStr(node->name()));
    kvs->push_back(Localize(key, node->line()));
    IVal val = fun_scope_->NewLocal();
    Closure(val, fn, node->line());
    kvs->push_back(val);
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
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// AST Serialization
////////////////////////////////////////////////////////////////////////////////////////////////////
class ASTSerializationVisitor : public ast::Visitor {
public:
    ASTSerializationVisitor(std::string *buf): buf_(DCHECK_NOTNULL(buf)) {        
    }

    virtual ~ASTSerializationVisitor() override {}

    //----------------------------------------------------------------------------------------------
    // Implements from ast::Visitor
    //----------------------------------------------------------------------------------------------
    virtual IVal VisitLambdaLiteral(ast::LambdaLiteral *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->end_line());
        WriteNames(node->params());
        WriteByte(node->vargs());
        WriteNode(node->value());
        return IVal::Void();
    }
    virtual IVal VisitIfStatement(ast::IfStatement *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->trace_id());
        WriteNode(node->cond());
        WriteNode(node->then_clause());
        WriteNodeOptional(node->else_clause());
        return IVal::Void();
    }
    virtual IVal VisitForIterateLoop(ast::ForIterateLoop *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->end_line());
        WriteInt(node->trace_id());
        WriteNames(node->names());
        WriteNode(node->init());
        WriteNode(node->body());
        return IVal::Void();
    }
    virtual IVal VisitForStepLoop(ast::ForStepLoop *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->end_line());
        WriteInt(node->trace_id_base()); // only need base of trace_id
        WriteString(node->name());
        WriteNode(node->init());
        WriteByte(node->is_until());
        WriteNode(node->limit());
        WriteNodeOptional(node->step());
        WriteNode(node->body());
        return IVal::Void();
    }
    virtual IVal VisitWhileLoop(ast::WhileLoop *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteNode(node->cond());
        WriteNode(node->body());
        return IVal::Void();
    }
    virtual IVal VisitContinue(ast::Continue *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        return IVal::Void();
    }
    virtual IVal VisitBreak(ast::Break *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        return IVal::Void();
    }
    virtual IVal VisitMultiple(ast::Multiple *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->trace_id());
        WriteOperator(node->op());
        WriteInt(node->n_operands());
        for (int i = 0; i < node->n_operands(); ++i) {
            WriteNode(node->operand(i));
        }
        return IVal::Void();
    }
    virtual IVal VisitLogicSwitch(ast::LogicSwitch *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteOperator(node->op());
        WriteNode(node->lhs());
        WriteNode(node->rhs());
        return IVal::Void();
    }
    virtual IVal VisitObjectDefinition(ast::ObjectDefinition *node,
                                       ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->end_line());
        WriteString(node->name());
        WriteByte(node->local());
        if (node->members()) {
            WriteI64(node->members()->size());
            for (auto stmt : *node->members()) {
                WriteNode(stmt);
            }
        } else {
            WriteByte(0);
        }
        return IVal::Void();
    }
    virtual IVal VisitClassDefinition(ast::ClassDefinition *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->end_line());
        WriteString(node->name());
        WriteByte(node->local());
        if (node->members()) {
            WriteI64(node->members()->size());
            for (auto stmt : *node->members()) {
                WriteNode(stmt);
            }
        } else {
            WriteByte(0);
        }
        return IVal::Void();
    }
    virtual IVal VisitFunctionDefinition(ast::FunctionDefinition *node,
                                         ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->trace_id());
        WriteNodeOptional(node->self());
        WriteString(node->name());
        WriteNode(node->literal());
        return IVal::Void();
    }
    virtual IVal VisitBlock(ast::Block *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->end_line());
        if (node->stmts()) {
            base::Slice::WriteVarint64(buf_, node->stmts()->size());
            for (auto stmt : *node->stmts()) {
                WriteNode(stmt);
            }
        } else {
            WriteByte(0);
        }
        return IVal::Void();
    }
    virtual IVal VisitVarDeclaration(ast::VarDeclaration *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteNames(node->names());
        WriteExprs(node->inits());
        return IVal::Void();
    }
    virtual IVal VisitPropertyDeclaration(ast::PropertyDeclaration *node,
                                          ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteNames(node->names());
        WriteExprs(node->inits());
        WriteByte(node->readonly());
        return IVal::Void();
    }
    virtual IVal VisitVariable(ast::Variable *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteString(node->name());
        return IVal::Void();
    }
    virtual IVal VisitAssignment(ast::Assignment *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteLvals(node->lvals());
        WriteExprs(node->rvals());
        return IVal::Void();
    }
    virtual IVal VisitIndex(ast::Index *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->trace_id());
        WriteNode(node->self());
        WriteNode(node->index());
        return IVal::Void();
    }
    virtual IVal VisitDotField(ast::DotField *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->trace_id());
        WriteNode(node->self());
        WriteString(node->index());
        return IVal::Void();
    }
    virtual IVal VisitSelfCall(ast::SelfCall *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->trace_id());
        WriteNode(node->callee());
        WriteString(node->method());
        WriteExprs(node->args());
        return IVal::Void();
    }
    virtual IVal VisitNew(ast::New *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->trace_id());
        WriteNode(node->callee());
        WriteExprs(node->args());
        return IVal::Void();
    }
    virtual IVal VisitCall(ast::Call *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->trace_id());
        WriteNode(node->callee());
        WriteExprs(node->args());
        return IVal::Void();
    }
    virtual IVal VisitReturn(ast::Return *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteExprs(node->rets());
        return IVal::Void();
    }
    virtual IVal VisitNilLiteral(ast::NilLiteral *node, ast::VisitorContext *) override {
        WritePrefix(node);
        return IVal::Void();
    }
    virtual IVal VisitStringLiteral(ast::StringLiteral *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteString(node->value());
        return IVal::Void();
    }
    virtual IVal VisitApproxLiteral(ast::ApproxLiteral *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteF64(node->value());
        return IVal::Void();
    }
    virtual IVal VisitSmiLiteral(ast::SmiLiteral *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteI64(node->value());
        return IVal::Void();
    }
    virtual IVal VisitIntLiteral(ast::IntLiteral *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteString(node->value());
        return IVal::Void();
    }
    virtual IVal VisitMapInitializer(ast::MapInitializer *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->end_line());
        WriteExprs(node->value());
        return IVal::Void();
    }
    virtual IVal VisitConcat(ast::Concat *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteExprs(node->operands());
        return IVal::Void();
    }
    virtual IVal VisitVariableArguments(ast::VariableArguments *node,
                                        ast::VisitorContext *x) override {
        WritePrefix(node);
        return IVal::Void();
    }
    
private:
    void WriteNode(ast::AstNode *node) { node->Accept(this, nullptr); }
    
    void WriteNodeOptional(ast::AstNode *node) {
        if (node) {
            WriteByte(1);
            WriteNode(node);
        } else {
            WriteByte(0);
        }
    }
    
    void WritePrefix(ast::Statement *stmt) {
        WriteKind(stmt->kind());
        WriteInt(stmt->line());
    }
    
    void WriteNames(const base::ArenaVector<const ast::String *> *names) {
        if (!names) {
            WriteByte(0);
            return;
        }
        base::Slice::WriteVarint64(buf_, names->size());
        for (const ast::String *name : *names) {
            WriteString(name);
        }
    }
    
    template<class T>
    void WriteExprs(const base::ArenaVector<T *> *exprs) {
        if (!exprs) {
            WriteByte(0);
            return;
        }
        base::Slice::WriteVarint64(buf_, exprs->size());
        for (ast::Expression *expr : *exprs) {
            WriteNode(expr);
        }
    }
    
    void WriteLvals(const base::ArenaVector<ast::LValue *> *exprs) {
        if (!exprs) {
            WriteByte(0);
            return;
        }
        base::Slice::WriteVarint64(buf_, exprs->size());
        for (ast::Expression *expr : *exprs) {
            WriteNode(expr);
        }
    }
    
    void WriteString(const ast::String *s) {
        base::Slice::WriteString(buf_, std::string_view(s->data(), s->size()));
    }
    
    void WriteKind(ast::AstNode::Kind kind) { WriteByte(static_cast<uint8_t>(kind)); }
    
    void WriteOperator(Operator::ID op) { WriteByte(static_cast<uint8_t>(op)); }
    
    void WriteInt(int n) { base::Slice::WriteVarint32(buf_, n); }
    
    void WriteF64(double n) { Write(&n, sizeof(n)); }
    
    void WriteI64(int64_t n) { base::Slice::WriteVarint64(buf_, n); }
    
    void Write(const void *p, size_t n) {
        buf_->append(reinterpret_cast<const char *>(p), n);
    }
    
    void WriteByte(uint8_t b) { buf_->append(1, static_cast<char>(b)); }
    
    std::string *buf_;
}; // class SerializationVisitor
    
class ASTSerializationLoader final {
public:
    ASTSerializationLoader(std::string_view buf, ast::Factory *factory)
        : rd_(buf)
        , factory_(factory) {}
    
    ast::AstNode *LoadNode() {
        ast::AstNode::Kind kind;
        int line;
        std::tie(kind, line) = ReadPrefix();
        switch (kind) {
        #define DEFINE_CALL_READ(name) \
            case ast::AstNode::k##name: \
                return Read##name(line);
                DECL_AST_NODES(DEFINE_CALL_READ)
        #undef DEFINE_CALL_READ
            default:
                DLOG(FATAL) << "Noreached!" << kind << " line: " << line;
                break;
        }
        return nullptr;
    }
    
    ast::FunctionDefinition *ReadFunctionDefinition(int line) {
        return factory_->NewFunctionDefinition(ReadInt(), ReadExprOptional(), ReadString(),
                                               DCHECK_NOTNULL(LoadNode()->ToLambdaLiteral()),
                                               Location(line));
    }
    
    ast::LambdaLiteral *ReadLambdaLiteral(int line) {
        int end_line = ReadInt();
        return factory_->NewLambdaLiteral(ReadNames(), ReadBool(), ReadBlock(),
                                          Location(line, end_line));
    }

    ast::ObjectDefinition *ReadObjectDefinition(int line) {
        int end_line = ReadInt();
        return factory_->NewObjectDefinition(ReadString(), ReadBool(), ReadStmts(),
                                             Location(line, end_line));
    }
    
    ast::ClassDefinition *ReadClassDefinition(int line) {
        int end_line = ReadInt();
        return factory_->NewClassDefinition(ReadString(), ReadBool(), ReadStmts(),
                                            Location(line, end_line));
    }
    
    ast::PropertyDeclaration *ReadPropertyDeclaration(int line) {
        return factory_->NewPropertyDeclaration(ReadNames(), ReadExprs(), ReadBool());
    }
    
    ast::Block *ReadBlock(int line) {
        int end_line = ReadInt();
        uint64_t n = rd_.ReadVarint64();
        ast::Block::StmtList *stmts = n == 0
            ? nullptr : factory_->NewList<ast::Statement *>(nullptr);
        for (uint64_t i = 0; i < n; ++i) {
            stmts->push_back(ReadStmt());
        }
        return factory_->NewBlock(stmts, Location(line, end_line));
    }
    
    ast::IfStatement *ReadIfStatement(int line) {
        return factory_->NewIfStatement(ReadInt(), ReadExpr(), ReadBlock(), ReadStmtOptional(),
                                        Location(line));
    }
    
    ast::ForStepLoop *ReadForStepLoop(int line) {
        int end_line = ReadInt();
        int next_trace_id = ReadInt();
        return factory_->NewForStepLoop(&next_trace_id, ReadString(), ReadExpr(), ReadBool(),
                                        ReadExpr(), ReadExprOptional(), ReadBlock(),
                                        Location(line, end_line));
    }
    
    ast::ForIterateLoop *ReadForIterateLoop(int line) {
        int end_line = ReadInt();
        return factory_->NewForIterateLoop(ReadInt(), ReadNames(), ReadExpr(), ReadBlock(),
                                           Location(line, end_line));
    }
    
    ast::WhileLoop *ReadWhileLoop(int line) {
        int end_line = ReadInt();
        return factory_->NewWhileLoop(ReadExpr(), ReadBlock(), Location(line, end_line));
    }
    
    ast::Break *ReadBreak(int line) { return factory_->NewBreak(Location(line)); }
    
    ast::Continue *ReadContinue(int line) { return factory_->NewContinue(Location(line)); }
    
    ast::Assignment *ReadAssignment(int line) {
        return factory_->NewAssignment(ReadLvals(), ReadExprs(), Location(line));
    }
    
    ast::Multiple *ReadMultiple(int line) {
        int trace_id = ReadInt();
        Operator::ID op = ReadOperator();
        int n = ReadInt();
        switch (n) {
            case 1:
                return factory_->NewUnary(trace_id, op, ReadExpr(), Location(line));
            case 2:
                return factory_->NewBinary(trace_id, op, ReadExpr(), ReadExpr(), Location(line));
            default:
                DLOG(FATAL) << "Noreached!";
                break;
        }
        return nullptr;
    }
    
    ast::Concat *ReadConcat(int line) {
        return factory_->NewConcat(ReadExprs(), Location(line));
    }
    
    ast::LogicSwitch *ReadLogicSwitch(int line) {
        Operator::ID op = ReadOperator();
        switch (op) {
            case Operator::kOr:
                return factory_->NewOr(ReadExpr(), ReadExpr());
            case Operator::kAnd:
                return factory_->NewAnd(ReadExpr(), ReadExpr());
            default:
                DLOG(FATAL) << "Noreached!";
                break;
        }
        return nullptr;
    }
    
    ast::Index *ReadIndex(int line) {
        return factory_->NewIndex(ReadInt(), ReadExpr(), ReadExpr(), Location(line));
    }
    
    ast::DotField *ReadDotField(int line) {
        return factory_->NewDotField(ReadInt(), ReadExpr(), ReadString(), Location(line));
    }
    
    ast::Variable *ReadVariable(int line) {
        return factory_->NewVariable(ReadString(), Location(line));
    }
    
    ast::VarDeclaration *ReadVarDeclaration(int line) {
        return factory_->NewVarDeclaration(ReadNames(), ReadExprs(), Location(line));
    }
    
    ast::SelfCall *ReadSelfCall(int line) {
        return factory_->NewSelfCall(ReadInt(), ReadExpr(), ReadString(), ReadExprs(),
                                     Location(line));
    }
    
    ast::New *ReadNew(int line) {
        return factory_->NewNew(ReadInt(), ReadExpr(), ReadExprs(), Location(line));
    }
    
    ast::Call *ReadCall(int line) {
        return factory_->NewCall(ReadInt(), ReadExpr(), ReadExprs(), Location(line));
    }
    
    ast::Return *ReadReturn(int line) {
        return factory_->NewReturn(ReadExprs(), Location(line));
    }
    
    ast::NilLiteral *ReadNilLiteral(int line) {
        return factory_->NewNilLiteral(Location(line));
    }
    
    ast::StringLiteral *ReadStringLiteral(int line) {
        return factory_->NewStringLiteral(ReadString(), Location(line));
    }
    
    ast::ApproxLiteral *ReadApproxLiteral(int line) {
        return factory_->NewApproxLiteral(rd_.ReadFloat64(), Location(line));
    }
    
    ast::IntLiteral *ReadIntLiteral(int line) {
        return factory_->NewIntLiteral(ReadString(), Location(line));
    }
    
    ast::SmiLiteral *ReadSmiLiteral(int line) {
        return factory_->NewSmiLiteral(rd_.ReadVarint64(), Location(line));
    }
    
    ast::MapInitializer *ReadMapInitializer(int line) {
        int end_line = ReadInt();
        return factory_->NewMapInitializer(ReadEntries(), Location(line, end_line));
    }
    
    ast::VariableArguments *ReadVariableArguments(int line) {
        return factory_->NewVariableArguments(Location(line));
    }
private:
    ast::Block *ReadBlock() { return DCHECK_NOTNULL(LoadNode()->ToBlock()); }
    
    ast::Expression *ReadExpr() {
        ast::AstNode *node = LoadNode();
        DCHECK(node->is_expression());
        return static_cast<ast::Expression *>(node);
    }
    
    ast::Expression *ReadExprOptional() {
        ast::AstNode *node = ReadNodeOptional();
        DCHECK(!node || node->is_expression());
        return static_cast<ast::Expression *>(node);
    }
    
    ast::Statement *ReadStmt() {
        ast::AstNode *node = LoadNode();
        DCHECK(node->is_statement());
        return static_cast<ast::Statement *>(node);
    }
    
    ast::Statement *ReadStmtOptional() {
        ast::AstNode *node = ReadNodeOptional();
        DCHECK(node || node->is_statement());
        return static_cast<ast::Statement *>(node);
    }
    
    ast::AstNode *ReadNodeOptional() { return rd_.ReadByte() ? LoadNode() : nullptr; }

    base::ArenaVector<ast::Multiple *> *ReadEntries() { return ReadNodes<ast::Multiple>(); }
    
    base::ArenaVector<ast::LValue *> *ReadLvals() { return ReadNodes<ast::LValue>(); }
    
    base::ArenaVector<ast::Expression *> *ReadExprs() { return ReadNodes<ast::Expression>(); }
    
    base::ArenaVector<ast::Statement *> *ReadStmts() { return ReadNodes<ast::Statement>(); }
    
    template<class T>
    base::ArenaVector<T *> *ReadNodes() {
        uint64_t n = rd_.ReadVarint64();
        if (n == 0) {
            return nullptr;
        }
        base::ArenaVector<T *> *stmts = factory_->NewList<T *>(nullptr);
        for (uint64_t i = 0; i < n; ++i) {
            stmts->push_back(DCHECK_NOTNULL(T::Cast(LoadNode())));
        }
        return stmts;
    }
    
    base::ArenaVector<const ast::String *> *ReadNames() {
        uint64_t n = rd_.ReadVarint64();
        if (n == 0) {
            return nullptr;
        }
        base::ArenaVector<const ast::String *> *names =
            factory_->NewList<const ast::String *>(nullptr);
        for (uint64_t i = 0; i < n; ++i) {
            names->push_back(ReadString());
        }
        return names;
    }
    
    const ast::String *ReadString() {
        std::string_view s = rd_.ReadString();
        return factory_->NewRawString(s.data(), s.size());
    }
    
    bool ReadBool() { return rd_.ReadByte() != 0; }

    std::tuple<ast::AstNode::Kind, int> ReadPrefix() { return {ReadKind(), ReadInt()}; }
    
    ast::AstNode::Kind ReadKind() {
        return static_cast<ast::AstNode::Kind>(rd_.ReadByte());
    }
    
    Operator::ID ReadOperator() { return static_cast<Operator::ID>(rd_.ReadByte()); }

    int ReadInt() { return static_cast<int>(rd_.ReadVarint32()); }
    
    static ast::Location Location(int line) {
        ast::Location loc;
        loc.begin_line = line;
        return loc;
    }
    
    static ast::Location Location(int line, int end_line) {
        ast::Location loc;
        loc.begin_line = line;
        loc.end_line = end_line;
        return loc;
    }
    
    base::BufferReader rd_;
    ast::Factory *factory_;
}; // class ASTSerializationLoader

Error SerializeCompactedAST(ast::AstNode *node, std::string *buf) {
    ASTSerializationVisitor visitor(buf);
    DCHECK_NOTNULL(node)->Accept(&visitor, nullptr);
    return MAI_NOT_SUPPORTED("TODO:");
}
    
Error LoadCompactedAST(std::string_view buf, ast::Factory *factory, ast::AstNode **rv) {
    ASTSerializationLoader loader(buf, factory);
    *rv = loader.LoadNode();
    return Error::OK();
}

} // namespace nyaa
    
} // namespace mai
