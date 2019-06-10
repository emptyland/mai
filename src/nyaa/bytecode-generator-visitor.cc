#include "nyaa/code-gen-base.h"

namespace mai {

namespace nyaa {
    
namespace {

class FunctionScopeBundle final : public FunctionScope {
public:
    FunctionScopeBundle(CodeGeneratorVisitor *owns) : FunctionScope(owns) {}

    BytecodeArrayBuilder builder_;
}; // class FunctionScopeBundle
    
} // namespace

class BytecodeGeneratorVisitor : public CodeGeneratorVisitor {
public:
    using Context = CodeGeneratorContext;
    
    BytecodeGeneratorVisitor(NyaaCore *core, base::Arena *arena, Handle<NyString> file_name)
        : CodeGeneratorVisitor(core, arena, file_name) {}
    
    virtual ~BytecodeGeneratorVisitor() override {};
    
    virtual IVal
    VisitFunctionDefinition(ast::FunctionDefinition *node, ast::VisitorContext *x) override {
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
        builder()->Closure(closure, rval, node->line());
        if (node->self()) {
            CodeGeneratorContext lix;
            lix.set_n_result(1);
            IVal self = node->self()->Accept(this, &lix);
            IVal index = IVal::Const(fun_scope_->kpool()->GetOrNewStr(node->name()));
            builder()->SetField(self, index, closure);
            fun_scope_->FreeVar(self);
            fun_scope_->FreeVar(index);
        } else {
            if (fun_scope_->prev() == nullptr) {
                // as global variable
                IVal lval = IVal::Global(fun_scope_->kpool()->GetOrNewStr(node->name()));
                builder()->StoreGlobal(closure, lval, node->line());
            } else {
                // as local variable
                blk_scope_->PutVariable(node->name(), &closure);
            }
        }
        fun_scope_->FreeVar(closure);
        return IVal::Void();
    }

    virtual IVal VisitIfStatement(ast::IfStatement *node, ast::VisitorContext *x) override {
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        IVal cond = node->cond()->Accept(this, &ix);
        builder()->Test(cond, 0, 0, node->line());
        fun_scope_->FreeVar(cond);
        
        BytecodeLable else_lable;
        builder()->Jump(&else_lable, fun_scope_->kpool(), node->line());
        node->then_clause()->Accept(this, &ix);
        
        if (!node->else_clause()) {
            builder()->Bind(&else_lable, fun_scope_->kpool());
        } else {
            BytecodeLable out_lable;
            builder()->Jump(&out_lable, fun_scope_->kpool(), node->then_clause()->line());
            builder()->Bind(&else_lable, fun_scope_->kpool());
            node->else_clause()->Accept(this, &ix);
            builder()->Bind(&out_lable, fun_scope_->kpool());
        }
        return IVal::Void();
    }
    
    virtual IVal VisitWhileLoop(ast::WhileLoop *node, ast::VisitorContext *x) override {
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        BytecodeLable in_lable;
        builder()->Bind(&in_lable, fun_scope_->kpool());
        
        IVal cond = node->cond()->Accept(this, &ix);
        builder()->Test(cond, 0, 0, node->line());
        fun_scope_->FreeVar(cond);
        BytecodeLable out_lable;
        builder()->Jump(&out_lable, fun_scope_->kpool(), node->line());
        
        blk_scope_->set_loop_in(&in_lable);
        blk_scope_->set_loop_out(&out_lable);
        
        node->body()->Accept(this, x);
    
        blk_scope_->set_loop_in(nullptr);
        blk_scope_->set_loop_out(nullptr);
        
        builder()->Jump(&in_lable, fun_scope_->kpool(), node->end_line());
        builder()->Bind(&out_lable, fun_scope_->kpool());
        return IVal::Void();
    }
    
    // for v1, v2, ... = expr {
    //    stmts...
    // }
    virtual IVal VisitForIterateLoop(ast::ForIterateLoop *node, ast::VisitorContext *x) override {
        BlockScope scope(fun_scope_);
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        
        IVal generator = node->init()->Accept(this, &ix);
        blk_scope_->PutVariable(ast::String::New(arena_, "(generator)"), &generator);
        
        BytecodeLable in_label;
        builder()->Jump(&in_label, fun_scope_->kpool(), node->line());
        
        for (auto name : *DCHECK_NOTNULL(node->names())) {
            blk_scope_->PutVariable(name, nullptr);
        }
        
        BytecodeLable body_label, out_label;
        scope.set_loop_in(&in_label);
        scope.set_loop_out(&out_label);
        builder()->Bind(&body_label, fun_scope_->kpool());
        node->body()->Accept(this, x);
        scope.set_loop_in(nullptr);
        scope.set_loop_out(nullptr);
        
        // test
        builder()->Bind(&in_label, fun_scope_->kpool());
        //builder()->Bind(&test_lable, fun_scope_->kpool());
        IVal callee = fun_scope_->NewLocal();
        builder()->Move(callee, generator, node->end_line());
        
        int nrets = static_cast<int>(node->names()->size());
        fun_scope_->Reserve(nrets);
        builder()->Call(callee, 0, nrets, node->end_line());
        builder()->TestNil(callee, 1/*neg*/, 0, node->end_line());
        builder()->Jump(&out_label, fun_scope_->kpool(), node->end_line());
        
        IVal base = callee;
        for (auto name : *DCHECK_NOTNULL(node->names())) {
            builder()->Move(blk_scope_->GetVariable(name), base, node->end_line());
            base.index++;
        }
        builder()->Jump(&body_label, fun_scope_->kpool(), node->end_line());
        builder()->Bind(&out_label, fun_scope_->kpool());
        return IVal::Void();
    }
    
    virtual IVal VisitContinue(ast::Continue *node, ast::VisitorContext *x) override {
        auto blk = blk_scope_;
        while (blk && blk->owns() == fun_scope_) {
            if (blk->loop_in()) {
                break;
            }
            blk = blk->prev();
        }
        DCHECK_NOTNULL(blk);
        builder()->Jump(static_cast<BytecodeLable *>(blk->loop_in()), fun_scope_->kpool(),
                        node->line());
        return IVal::Void();
    }
    
    virtual IVal VisitBreak(ast::Break *node, ast::VisitorContext *x) override {
        auto blk = blk_scope_;
        while (blk && blk->owns() == fun_scope_) {
            if (blk->loop_out()) {
                break;
            }
            blk = blk->prev();
        }
        DCHECK_NOTNULL(blk);
        builder()->Jump(static_cast<BytecodeLable *>(blk->loop_out()), fun_scope_->kpool(),
                        node->line());
        return IVal::Void();
    }
    
    virtual IVal VisitObjectDefinition(ast::ObjectDefinition *node, ast::VisitorContext *x) override {
        IVal clazz = DefineClass(node, nullptr);
        builder()->New(clazz, clazz, 0, node->end_line());
        if (node->local()) {
            blk_scope_->PutVariable(node->name(), &clazz);
        } else {
            fun_scope_->FreeVar(clazz);
            IVal key = IVal::Global(fun_scope_->kpool()->GetOrNewStr(node->name()));
            builder()->StoreGlobal(clazz, key, node->end_line());
        }
        return IVal::Void();
    }
    
    virtual IVal VisitClassDefinition(ast::ClassDefinition *node, ast::VisitorContext *) override {
        IVal clazz = DefineClass(node, node->base());
        if (node->local()) {
            blk_scope_->PutVariable(node->name(), &clazz);
        } else {
            fun_scope_->FreeVar(clazz);
            IVal key = IVal::Global(fun_scope_->kpool()->GetOrNewStr(node->name()));
            builder()->StoreGlobal(clazz, key, node->end_line());
        }
        return IVal::Void();
    }
    
    virtual IVal VisitLambdaLiteral(ast::LambdaLiteral *node, ast::VisitorContext *x) override {
        //HandleScope handle_scope(core_->isolate());
        Handle<NyFunction> proto;
        {
            FunctionScopeBundle fun_scope(this);
            BlockScope blk_scope(&fun_scope);
            if (node->params()) {
                for (auto param : *node->params()) {
                    blk_scope.PutVariable(param, nullptr);
                }
            }
            CodeGeneratorContext bix;
            node->value()->Accept(this, &bix);
            builder()->Ret(IVal::Local(0), 0);
            
            Handle<NyByteArray> bcbuf;
            Handle<NyInt32Array> info;
            std::tie(bcbuf, info) = fun_scope.builder_.Build(core_);
            Handle<NyArray> kpool = fun_scope.kpool()->Build(core_);
            Handle<NyArray> fpool = fun_scope.BuildProtos(core_);
            
            proto = core_->factory()->NewFunction(nullptr/*name*/,
                                                  !node->params() ? 0 : node->params()->size()/*nparams*/,
                                                  node->vargs()/*vargs*/,
                                                  fun_scope.upval_desc_size() /*n_upvals*/,
                                                  fun_scope.max_stack(),
                                                  file_name_.is_empty() ? nullptr : *file_name_,
                                                  *info, *bcbuf, *fpool, *kpool);
            size_t i = 0;
            for (auto upval : fun_scope.upval_desc()) {
                NyString *name = core_->factory()->NewString(upval.name->data(), upval.name->size());
                proto->SetUpval(i++, name, upval.in_stack, upval.index, core_);
            }
        }
        IVal val = fun_scope_->NewProto(proto);
        CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
        if (ctx->localize() && !ctx->keep_const()) {
            return Localize(val, node->line());
        }
        return val;
    }
    
    virtual IVal VisitNew(ast::New *node, ast::VisitorContext *x) override {
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
        
        builder()->New(val, clazz, n_args, node->line());
        //fun_scope_->FreeVar(clazz);
        fun_scope_->set_free_reg(clazz.index);
        return val;
    }
    
    virtual IVal VisitSelfCall(ast::SelfCall *node, ast::VisitorContext *x) override {
        int32_t n_args = node->GetNArgs();
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        
        IVal base = fun_scope_->NewLocal();
        IVal self = fun_scope_->NewLocal();
        IVal callee = node->callee()->Accept(this, &ix);
        IVal method = IVal::Const(fun_scope_->kpool()->GetOrNewStr(node->method()));
        builder()->Self(base, callee, method, node->line());
        fun_scope_->FreeVar(callee);
        
        ix.set_n_result(n_args < 0 ? -1 : 1);
        int reg = base.index + 1;
        for (size_t i = 0; node->args() && i < node->args()->size(); ++i) {
            ast::Expression *expr = node->args()->at(i);
            AdjustStackPosition(++reg, expr->Accept(this, &ix), expr->line());
        }
        (void)self;
        //fun_scope_->FreeVar(self);
        
        CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
        builder()->Call(base, n_args + 1/*for self*/, ctx->n_result(), node->line());
        
        fun_scope_->set_free_reg(base.index + 1);
        return base;
    }
    
    virtual
    IVal VisitVariableArguments(ast::VariableArguments *node, ast::VisitorContext *x) override {
        CodeGeneratorContext *ctx = CodeGeneratorContext::Cast(x);
        IVal vargs = fun_scope_->NewLocal();
        builder()->Vargs(vargs, ctx->n_result(), node->line());
        return vargs;
    }
    
    virtual IVal VisitMultiple(ast::Multiple *node, ast::VisitorContext *x) override {
        std::vector<IVal> operands;
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        ix.set_keep_const(true);
        
        IVal ret = fun_scope_->NewLocal();
        for (int i = 0; i < node->n_operands(); ++i) {
            operands.push_back(node->operand(i)->Accept(this, &ix));
        }
        
        switch (node->op()) {
            case Operator::kAdd:
                builder()->Add(ret, operands[0], operands[1], node->line());
                break;
            case Operator::kSub:
                builder()->Sub(ret, operands[0], operands[1], node->line());
                break;
            case Operator::kMul:
                builder()->Mul(ret, operands[0], operands[1], node->line());
                break;
            case Operator::kDiv:
                builder()->Div(ret, operands[0], operands[1], node->line());
                break;
            case Operator::kMod:
                builder()->Mod(ret, operands[0], operands[1], node->line());
                break;
            case Operator::kEQ:
                builder()->Equal(ret, operands[0], operands[1], node->line());
                break;
            case Operator::kNE:
                builder()->NotEqual(ret, operands[0], operands[1], node->line());
                break;
            case Operator::kLT:
                builder()->LessThan(ret, operands[0], operands[1], node->line());
                break;
            case Operator::kLE:
                builder()->LessEqual(ret, operands[0], operands[1], node->line());
                break;
            case Operator::kGT:
                builder()->GreaterThan(ret, operands[0], operands[1], node->line());
                break;
            case Operator::kGE:
                builder()->GreaterEqual(ret, operands[0], operands[1], node->line());
                break;
            default:
                DLOG(FATAL) << "TODO:";
                break;
        }
        for (int64_t i = operands.size() - 1; i >= 0; --i) {
            fun_scope_->FreeVar(operands[i]);
        }
        return ret;
    }
    
    virtual IVal VisitLogicSwitch(ast::LogicSwitch *node, ast::VisitorContext *x) override {
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        
        IVal ret = fun_scope_->NewLocal();
        IVal lhs = node->lhs()->Accept(this, &ix);
        IVal rhs = node->rhs()->Accept(this, &ix);
        switch (node->op()) {
            case Operator::kAnd: {
                int br1 = builder()->GetCodeSize(ret.Encode(), rhs.Encode());
                br1 += builder()->GetCodeSize(br1) + 1;
                builder()->TestSet(ret, lhs, 1/*neg*/, node->line());
                builder()->Jump(br1, node->line());
                builder()->Move(ret, rhs, node->line());
            } break;
            case Operator::kOr: {
                int br1 = builder()->GetCodeSize(ret.Encode(), rhs.Encode());
                br1 += builder()->GetCodeSize(br1) + 1;
                builder()->TestSet(ret, lhs, 0/*neg*/, node->line());
                builder()->Jump(br1, node->line());
                builder()->Move(ret, rhs, node->line());
            } break;
            default:
                DLOG(FATAL) << "Noreached:" << node->op();
                break;
        }
        fun_scope_->FreeVar(rhs);
        fun_scope_->FreeVar(lhs);
        return ret;
    }
    
    virtual IVal VisitConcat(ast::Concat *node, ast::VisitorContext *x) override {
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
        builder()->Concat(base, base, static_cast<int32_t>(ops.size()), node->line());
        return base;
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(BytecodeGeneratorVisitor);
private:
    virtual void LoadNil(IVal val, int n, int line) override {
        builder()->LoadNil(val, n, line);
    }
    
    virtual void Call(IVal callee, int nargs, int wanted, int line) override {
        builder()->Call(callee, nargs, wanted, line);
    }
    
    virtual void Ret(IVal base, int nrets, int line) override {
        builder()->Ret(base, nrets, line);
    }
    
    virtual void Move(IVal dst, IVal src, int line) override {
        builder()->Move(dst, src, line);
    }
    
    virtual void StoreUp(IVal val, IVal up, int line) override {
        builder()->StoreUp(val, up, line);
    }

    virtual void StoreGlobal(IVal val, IVal name, int line) override {
        builder()->StoreGlobal(val, name, line);
    }

    virtual void NewMap(IVal map, int n, int linear, int line) override {
        builder()->NewMap(map, n, linear, line);
    }
    
    virtual void SetField(IVal self, IVal index, IVal value, int line) override {
        builder()->SetField(self, index, value, line);
    }

    virtual void GetField(IVal value, IVal self, IVal index, int line) override {
        builder()->GetField(value, self, index, line);
    }

    virtual IVal Localize(IVal val, int line) override {
        switch (val.kind) {
            case IVal::kLocal:
                break;
            case IVal::kUpval:
            case IVal::kFunction:
            case IVal::kGlobal:
            case IVal::kConst: {
                IVal ret = fun_scope_->NewLocal();
                builder()->Load(ret, val, line);
                return ret;
            } break;
                break;
            case IVal::kVoid:
            default:
                DLOG(FATAL) << "noreached.";
                break;
        }
        return val;
    }
    
    BytecodeArrayBuilder *builder() {
        return &static_cast<FunctionScopeBundle *>(fun_scope_)->builder_;
    }
    
    IVal DefineClass(ast::ObjectDefinition *node, ast::Expression *base) {
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
            if (blk_scope_->Protected(val)) {
                IVal tmp = fun_scope_->NewLocal();
                builder()->Move(tmp, val);
                val = tmp;
            }
            kvs.push_back(Localize(val, node->line()));
        }
        
        IVal clazz = kvs.front();
        builder()->NewMap(clazz, static_cast<int>(kvs.size()), -1/*linear*/, node->end_line());
        for (int64_t i = kvs.size() - 1; i > 0; --i) {
            fun_scope_->FreeVar(kvs[i]);
        }
        return clazz;
    }
    
    size_t DeclareClassProperies(ast::PropertyDeclaration *decl, size_t offset, FieldTable *fields,
                                 std::vector<IVal> *kvs) {
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
            builder()->LoadImm(val, static_cast<int32_t>(tag), decl->line());
            kvs->push_back(val);
        }
        return offset;
    }
    
    void DefineClassMethod(const ast::String *class_name, ast::FunctionDefinition *node,
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
        builder()->Closure(val, fn, node->line());
        kvs->push_back(val);
    }
    
    IVal AdjustStackPosition(int requried, IVal val, int line) {
        DCHECK_EQ(IVal::kLocal, val.kind);
        if (requried != val.index) {
            IVal dst = fun_scope_->NewLocal();
            DCHECK_EQ(requried, dst.index);
            builder()->Move(dst, val, line);
            return dst;
        }
        return val;
    }
}; // class CodeGeneratorVisitor

Handle<NyFunction> Bytecode_CodeGenerate(Handle<NyString> file_name, ast::Block *root,
                                         base::Arena *arena, NyaaCore *core) {
    HandleScope handle_scope(core->stub());
    
    BytecodeGeneratorVisitor visitor(core, arena, file_name);
    FunctionScopeBundle scope(&visitor);
    root->Accept(&visitor, nullptr);
    
    Handle<NyByteArray> bcbuf;
    Handle<NyInt32Array> info;
    scope.builder_.Ret(IVal::Local(0), 0); // last return
    std::tie(bcbuf, info) = scope.builder_.Build(core);
    Handle<NyArray> kpool = scope.kpool()->Build(core);
    Handle<NyArray> fpool = scope.BuildProtos(core);
    
    Handle<NyFunction> result =
        core->factory()->NewFunction(nullptr/*name*/, 0/*nparams*/, true/*vargs*/, 0/*n_upvals*/,
                                     scope.max_stack(), *file_name,/*source file name*/
                                     *info,/*source info */ *bcbuf,/*exec object*/
                                     *fpool/*proto_pool*/, *kpool);
    return handle_scope.CloseAndEscape(result);
}
    
} // namespace nyaa

} // namespace mai
