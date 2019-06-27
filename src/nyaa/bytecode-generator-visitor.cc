#include "nyaa/code-gen-utils.h"

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

    virtual IVal VisitIfStatement(ast::IfStatement *node, ast::VisitorContext *x) override {
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        IVal cond = node->cond()->Accept(this, &ix);
        builder()->Test(cond, 0, 0, node->line());
        fun_scope_->FreeVar(cond);
        
        BytecodeLabel else_lable;
        builder()->Jump(&else_lable, fun_scope_->kpool(), node->line());
        node->then_clause()->Accept(this, &ix);
        
        if (!node->else_clause()) {
            builder()->Bind(&else_lable, fun_scope_->kpool());
        } else {
            BytecodeLabel out_lable;
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
        BytecodeLabel in_lable;
        builder()->Bind(&in_lable, fun_scope_->kpool());
        
        IVal cond = node->cond()->Accept(this, &ix);
        builder()->Test(cond, 0, 0, node->line());
        fun_scope_->FreeVar(cond);
        BytecodeLabel out_lable;
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
    
    // for v1, v2, ... in expr {
    //    stmts...
    // }
    virtual IVal VisitForIterateLoop(ast::ForIterateLoop *node, ast::VisitorContext *x) override {
        BlockScope scope(fun_scope_);
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        
        IVal generator = node->init()->Accept(this, &ix);
        blk_scope_->PutVariable(ast::String::New(arena_, "(generator)"), &generator);
        
        BytecodeLabel in_label;
        builder()->Jump(&in_label, fun_scope_->kpool(), node->line());
        
        for (auto name : *DCHECK_NOTNULL(node->names())) {
            blk_scope_->PutVariable(name, nullptr);
        }
        
        BytecodeLabel body_label, out_label;
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
        Move(callee, generator, node->end_line());
        
        int nrets = static_cast<int>(node->names()->size());
        fun_scope_->Reserve(nrets);
        Call(callee, 0, nrets, node->end_line());
        builder()->TestNil(callee, 1/*neg*/, 0, node->end_line());
        builder()->Jump(&out_label, fun_scope_->kpool(), node->end_line());
        
        IVal base = callee;
        for (auto name : *DCHECK_NOTNULL(node->names())) {
            Move(blk_scope_->GetVariable(name), base, node->end_line());
            base.index++;
        }
        builder()->Jump(&body_label, fun_scope_->kpool(), node->end_line());
        builder()->Bind(&out_label, fun_scope_->kpool());
        return IVal::Void();
    }

    // for var in expr to expr { body }
    // for var in expr until expr { body }
    virtual IVal VisitForStepLoop(ast::ForStepLoop *node, ast::VisitorContext *x) override {
        BlockScope scope(fun_scope_);
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        
        IVal var = fun_scope_->NewLocal();
        if (IsNotPlaceholder(node->name())) {
            blk_scope_->PutVariable(node->name(), &var);
        } else {
            blk_scope_->PutVariable(ast::String::New(arena_, "(shadow)"), &var);
        }
        IVal init = node->init()->Accept(this, &ix);
        Move(var, init, node->init()->line());
        IVal limit = node->limit()->Accept(this, &ix);
        IVal step;
        if (node->step()) {
            step = node->step()->Accept(this, &ix);
        } else {
            step = fun_scope_->NewLocal();
            LoadImm(step, 1, node->line());
        }
        
        BytecodeLabel body_label, in_label, out_label;
        // test
        builder()->Bind(&body_label, fun_scope_->kpool());
        IVal ret = fun_scope_->NewLocal();
        if (node->is_until()) {
            builder()->GreaterEqual(ret, var, limit, node->line());
            builder()->Test(ret, 1, 0, node->line());
        } else {
            builder()->GreaterThan(ret, var, limit, node->line());
            builder()->Test(ret, 1, 0, node->line());
        }
        builder()->Jump(&out_label, fun_scope_->kpool(), node->end_line());
        fun_scope_->FreeVar(ret);

        scope.set_loop_in(&in_label);
        scope.set_loop_out(&out_label);
        node->body()->Accept(this, x);
        scope.set_loop_in(nullptr);
        scope.set_loop_out(nullptr);

        builder()->Bind(&in_label, fun_scope_->kpool());
        builder()->Add(var, var, step);
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
        builder()->Jump(static_cast<BytecodeLabel *>(blk->loop_in()), fun_scope_->kpool(),
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
        builder()->Jump(static_cast<BytecodeLabel *>(blk->loop_out()), fun_scope_->kpool(),
                        node->line());
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
            //if (!blk_scope_->Protected(operands[i])) {
            fun_scope_->FreeVar(operands[i]);
            //}
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
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(BytecodeGeneratorVisitor);
private:
    virtual void LoadNil(IVal val, int n, int line) override {
        builder()->LoadNil(val, n, line);
    }

    virtual void LoadImm(IVal val, int32_t imm, int line) override {
        builder()->LoadImm(val, imm, line);
    }
    
    virtual void Self(IVal base, IVal callee, IVal method, int line) override {
        builder()->Self(base, callee, method, line);
    }
    
    virtual void Call(IVal callee, int nargs, int wanted, int line) override {
        DCHECK_NE(0, wanted) << "at leatest one wanted resunt value.";
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
    
    virtual void Closure(IVal closure, IVal func, int line) override {
        builder()->Closure(closure, func, line);
    }
    
    virtual void New(IVal val, IVal clazz, int nargs, int line) override {
        builder()->New(val, clazz, nargs, line);
    }

    virtual void Vargs(IVal vargs, int wanted, int line) override {
        builder()->Vargs(vargs, wanted, line);
    }

    virtual void Concat(IVal val, IVal base, int n, int line) override {
        builder()->Concat(val, base, n, line);
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
