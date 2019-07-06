#include "nyaa/code-gen-utils.h"
#include "nyaa/function.h"
#include "nyaa/thread.h"
#include "nyaa/runtime.h"
#include "asm/x64/asm-x64.h"
#include "asm/utils.h"
#include "mai-lang/nyaa.h"

namespace mai {
    
namespace nyaa {

namespace {
    
class FunctionScopeBundle final : public FunctionScope {
public:
    FunctionScopeBundle(CodeGeneratorVisitor *owns)
        : FunctionScope(owns)
        , compact_line_info_(owns->core()->stub()->compact_source_line_info()) {}
    
    bool compact_line_info_ = false;
    x64::Assembler masm_;
    std::vector<int> line_info_;
}; // class FunctionScopeBundle
    
class FileLineScope final {
public:
    FileLineScope(FunctionScope *fs, int line)
        : fs_(static_cast<FunctionScopeBundle*>(fs))
        , start_pc_(fs_->masm_.pc())
        , line_(line) {
    }
    
    ~FileLineScope() {
        if (fs_->compact_line_info_) {
            fs_->line_info_.push_back(start_pc_);
            fs_->line_info_.push_back(fs_->masm_.pc() + 1);
            fs_->line_info_.push_back(line_);
        } else {
            int pc = start_pc_;
            while (pc++ < fs_->masm_.pc()) {
                fs_->line_info_.push_back(line_);
            }
        }
    }
    
private:
    FunctionScopeBundle *fs_;
    int start_pc_;
    int line_;
}; // class FileLineScope
    
} // namespace
    
using namespace x64;

#define __ masm()->
    
class AOTGeneratorVisitor : public CodeGeneratorVisitor {
public:
    static constexpr Register kScratch = Runtime::kScratch;
    static constexpr Register kThread = Runtime::kThread;
    static constexpr Register kCore = Runtime::kCore;
    static constexpr Register kBP = Runtime::kBP;
    
    using Context = CodeGeneratorContext;
    
    AOTGeneratorVisitor(NyaaCore *core, base::Arena *arena, Handle<NyString> file_name)
        : CodeGeneratorVisitor(core, arena, file_name) {}
    
    virtual ~AOTGeneratorVisitor() override {}

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

            InitializeFun(node->line());
            node->value()->Accept(this, &bix);
            Ret(IVal::Local(0), 0, node->end_line());
            
            Handle<NyInt32Array> info = core_->factory()->NewInt32Array(fun_scope.line_info_.size());
            info = info->Add(fun_scope.line_info_.data(), fun_scope.line_info_.size(), core_);
    
            auto buf = fun_scope.masm_.buf();
            Handle<NyCode> code =
                core_->factory()->NewCode(NyCode::kFunction, *info,
                                          reinterpret_cast<const uint8_t *>(buf.data()), buf.size());
            Handle<NyArray> kpool = fun_scope.kpool()->Build(core_);
            Handle<NyArray> fpool = fun_scope.BuildProtos(core_);
            
            proto = core_->factory()->NewFunction(nullptr/*name*/,
                                                  !node->params() ? 0 : node->params()->size()/*nparams*/,
                                                  node->vargs()/*vargs*/,
                                                  fun_scope.upval_desc_size() /*n_upvals*/,
                                                  fun_scope.max_stack(),
                                                  file_name_.is_empty() ? nullptr : *file_name_,
                                                  *code, *fpool, *kpool);
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
    
    virtual IVal VisitIfStatement(ast::IfStatement *node, ast::VisitorContext *x) override {
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        IVal cond = node->cond()->Accept(this, &ix);

        Label else_lable;
        {
            FileLineScope fls(fun_scope_, node->line());
            //__ Breakpoint();
            __ movq(kRegArgv[0], Local(cond.index));
            CallRuntime(Runtime::kObject_IsFalse);
            if (core_->stub()->use_jit()) {
                __ movq(kRegArgv[0], kCore);
                __ movl(kRegArgv[1], node->trace_id());
                __ movl(kRegArgv[2], rax);
                CallRuntime(Runtime::kNyaaCore_TraceBranchGuard);
            }
            __ cmpl(rax, 0);
            __ j(NotEqual, &else_lable, true); // is false?
        }
        fun_scope_->FreeVar(cond);

        node->then_clause()->Accept(this, &ix);
        
        if (!node->else_clause()) {
            __ Bind(&else_lable);
        } else {
            FileLineScope fls(fun_scope_, node->else_clause()->line());
            Label exit_lable;
            __ jmp(&exit_lable, true);
            __ Bind(&else_lable);
            node->else_clause()->Accept(this, &ix);
            __ Bind(&exit_lable);
        }
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
        
        Label in_label;
        {
            FileLineScope fls(fun_scope_, node->line());
            __ jmp(&in_label, true);
        }

        for (auto name : *DCHECK_NOTNULL(node->names())) {
            blk_scope_->PutVariable(name, nullptr);
        }
        
        Label body_label, out_label;
        scope.set_loop_in(&in_label);
        scope.set_loop_out(&out_label);
        __ Bind(&body_label);
        node->body()->Accept(this, x);
        scope.set_loop_in(nullptr);
        scope.set_loop_out(nullptr);
        
        // test
        __ Bind(&in_label);
        IVal callee = fun_scope_->NewLocal();
        Move(callee, generator, node->end_line());
        
        int nrets = static_cast<int>(node->names()->size());
        fun_scope_->Reserve(nrets);
        Call(callee, 0, nrets, node->end_line(), node->trace_id());
        {
            FileLineScope fls(fun_scope_, node->line());
            __ cmpq(Local(callee.index), 0);
            __ j(Equal, &out_label, true);
        }
        
        IVal base = callee;
        for (auto name : *DCHECK_NOTNULL(node->names())) {
            Move(blk_scope_->GetVariable(name), base, node->end_line());
            base.index++;
        }
        {
            FileLineScope fls(fun_scope_, node->line());
            __ jmp(&body_label, true);
        }
        __ Bind(&out_label);
        return IVal::Void();
    }

    // for (var in expr to expr) { body }
    // for (var in expr until expr) { body }
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
        IVal iter;
        if (fun_scope_->Protected(init)) {
            iter = blk_scope_->PutVariable(ast::String::New(arena_, "(iter)"), nullptr);
            
        } else {
            iter = blk_scope_->PutVariable(ast::String::New(arena_, "(iter)"), &init);
        }
        Move(var, iter, node->init()->line());
        IVal limit = node->limit()->Accept(this, &ix);
        IVal step;
        if (node->step()) {
            step = node->step()->Accept(this, &ix);
        } else {
            step = fun_scope_->NewLocal();
            LoadImm(step, 1, node->line());
        }
        
        Label body_label, in_label, out_label;
        // test
        __ Bind(&body_label);
        IVal ret = fun_scope_->NewLocal();
        if (node->is_until()) {
            BinaryExpression(ret, iter, limit, Operator::kGE, Runtime::kObject_GE, node->line(),
                             node->trace_id1());
        } else {
            BinaryExpression(ret, iter, limit, Operator::kGT, Runtime::kObject_GT, node->line(),
                             node->trace_id1());
        }
        __ movq(kRegArgv[0], Local(ret.index));
        CallRuntime(Runtime::kObject_IsFalse);
        __ cmpl(rax, 0);
        __ j(Equal, &out_label, true/*far*/);
        fun_scope_->FreeVar(ret);
        
        scope.set_loop_in(&in_label);
        scope.set_loop_out(&out_label);
        node->body()->Accept(this, x);
        scope.set_loop_in(nullptr);
        scope.set_loop_out(nullptr);
        
        __ Bind(&in_label);
        BinaryExpression(iter, iter, step, Operator::kAdd, Runtime::kObject_Add, node->line(),
                         node->trace_id2());
        Move(var, iter, node->line());
        __ jmp(&body_label, true/*far*/);
        __ Bind(&out_label);
        return IVal::Void();
    }
    
    virtual IVal VisitWhileLoop(ast::WhileLoop *node, ast::VisitorContext *x) override {
        CodeGeneratorContext ix;
        ix.set_n_result(1);

        Label in_label;
        __ Bind(&in_label);

        IVal cond = node->cond()->Accept(this, &ix);
        Label out_label;
        {
            FileLineScope fls(fun_scope_, node->line());
            __ movq(kRegArgv[0], Local(cond.index));
            __ cmpq(kRegArgv[0], 0);
            __ j(Equal, &out_label, true); // cond == nil ?
            CallRuntime(Runtime::kObject_IsFalse);
            __ cmpl(rax, 0);
            __ j(NotEqual, &out_label, true); // cond is true?
        }
        fun_scope_->FreeVar(cond);
        
        blk_scope_->set_loop_in(&in_label);
        blk_scope_->set_loop_out(&out_label);
        
        node->body()->Accept(this, x);
        
        blk_scope_->set_loop_in(nullptr);
        blk_scope_->set_loop_out(nullptr);

        {
            FileLineScope fls(fun_scope_, node->line());
            __ jmp(&in_label, true);
            __ Bind(&out_label);
        }
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

        FileLineScope fls(fun_scope_, node->line());
        __ jmp(static_cast<Label *>(blk->loop_in()), true);
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

        FileLineScope fls(fun_scope_, node->line());
        __ jmp(static_cast<Label *>(blk->loop_out()), true);
        return IVal::Void();
    }
    
    virtual IVal VisitMultiple(ast::Multiple *node, ast::VisitorContext *x) override {
        std::vector<IVal> operands;
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        ix.set_keep_const(true);
        
        IVal ret = fun_scope_->NewLocal();
        if (node->op() == Operator::kNot) {
            if (ast::Multiple *inner = node->operand()->ToMultiple()) {
                if (ProcessNotOperator(ret, inner, node->line())) {
                    return ret;
                }
            }
        }
        
        for (int i = 0; i < node->n_operands(); ++i) {
            operands.push_back(node->operand(i)->Accept(this, &ix));
        }

        #define DEF_BIN(op) BinaryExpression(ret, operands[0], operands[1], Operator::k##op, \
            Runtime::kObject_##op, node->line(), node->trace_id())
        #define DEF_BIT_BIN(op) BitBinaryExpression(ret, operands[0], operands[1], Operator::k##op, \
            Runtime::kObject_##op, node->line())
        switch (node->op()) {
            case Operator::kAdd:
                DEF_BIN(Add);
                break;
            case Operator::kSub:
                DEF_BIN(Sub);
                break;
            case Operator::kMul:
                DEF_BIN(Mul);
                break;
            case Operator::kDiv:
                DEF_BIN(Div);
                break;
            case Operator::kMod:
                DEF_BIN(Mod);
                break;
            case Operator::kEQ:
                DEF_BIN(EQ);
                break;
            case Operator::kNE:
                DEF_BIN(NE);
                break;
            case Operator::kLT:
                DEF_BIN(LT);
                break;
            case Operator::kLE:
                DEF_BIN(LE);
                break;
            case Operator::kGT:
                DEF_BIN(GT);
                break;
            case Operator::kGE:
                DEF_BIN(GE);
                break;
            case Operator::kShl:
                DEF_BIT_BIN(Shl);
                break;
            case Operator::kShr:
                DEF_BIT_BIN(Shr);
                break;
            case Operator::kBitAnd:
                DEF_BIT_BIN(BitAnd);
                break;
            case Operator::kBitOr:
                DEF_BIT_BIN(BitOr);
                break;
            case Operator::kBitXor:
                DEF_BIT_BIN(BitXor);
                break;
            case Operator::kUnm:
                UnmExpression(ret, operands[0], node->line(), node->trace_id());
                break;
            case Operator::kNot:
                NotExpression(ret, operands[0], node->line());
                break;
            case Operator::kBitInv:
                BitInvExpression(ret, operands[0], node->line());
                break;
            default:
                DLOG(FATAL) << "TODO:";
                break;
        }
        #undef DEF_BIT_BIN
        #undef DEF_BIN
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
                FileLineScope fls(fun_scope_, node->line());
                __ movq(kRegArgv[0], Local(lhs.index));
                CallRuntime(Runtime::kObject_IsFalse);
                __ cmpl(rax, 0);
                __ cmovq(NotEqual, rax, Local(lhs.index));
                __ cmovq(Equal, rax, Local(rhs.index));
                __ movq(Local(ret.index), rax);
            } break;
            case Operator::kOr: {
                FileLineScope fls(fun_scope_, node->line());
                __ movq(kRegArgv[0], Local(lhs.index));
                CallRuntime(Runtime::kObject_IsFalse);
                __ cmpl(rax, 0);
                __ cmovq(Equal, rax, Local(lhs.index));
                __ cmovq(NotEqual, rax, Local(rhs.index));
                __ movq(Local(ret.index), rax);
            } break;
            default:
                DLOG(FATAL) << "Noreached:" << node->op();
                break;
        }
        fun_scope_->FreeVar(rhs);
        fun_scope_->FreeVar(lhs);
        return ret;
    }

    void InitializeFun(int line) {
        FileLineScope fls(fun_scope_, line);
        __ pushq(rbp);
        __ movq(rbp, rsp);

        // Setup BP
        __ movq(kBP, Operand(kThread, NyThread::kOffsetStack));
        __ movq(rax, Operand(kThread, NyThread::kOffsetFrame));
        __ movq(rax, Operand(rax, CallFrame::kOffsetStackBP));
        __ shlq(rax, kPointerShift);
        __ addq(kBP, rax);
    }

    void FinalizeRet() { Ret(IVal::Local(0), 0, 0); }
private:
    virtual void LoadNil(IVal val, int n, int line) override {
        DCHECK_EQ(IVal::kLocal, val.kind);
        FileLineScope fls(fun_scope_, line);
        if (n <= 4) {
            __ xorq(rax, rax); // rax = nil
            for (int i = 0; i < n; ++i) {
                __ movq(Operand(kBP, (val.index + i) * kPointerSize), rax);
            }
        } else {
            DCHECK_GT(n, 4);
            Label retry;
            __ movl(rcx, 0);
            __ xorq(rax, rax); // rax = nil
            __ Bind(&retry);
            __ movq(Operand(kBP, rcx, times_ptr_size, 0), rax);
            __ incl(rcx);
            __ cmpl(rcx, n);
            __ LikelyJ(Less, &retry, true);
        }
    }

    virtual void LoadImm(IVal val, int32_t imm, int line) override {
        DCHECK_EQ(IVal::kLocal, val.kind);
        FileLineScope fls(fun_scope_, line);
        Object *imm_val = NySmi::New(imm);
        __ movq(rax, reinterpret_cast<Address>(imm_val));
        __ movq(Local(val.index), rax);
    }

    void Invoke(int32_t callee, int32_t nargs, int32_t wanted, int32_t trace_id) {
        SaveRIP();
        __ movl(kRegArgv[0], callee);
        __ movl(kRegArgv[1], nargs);
        __ movl(kRegArgv[2], wanted);
        __ movl(kRegArgv[3], trace_id);
        __ movq(rax, core_->code_pool()->kCallStub->entry_address());
        __ call(rax);
    }
    
    void SaveRIP() {
        __ movq(rbx, Operand(kThread, NyThread::kOffsetFrame));
        __ movq(r9, Operand(rbx, CallFrame::kOffsetEntry));
        __ lea(r8, Operand(rip, 0)); // r8 = rip
        __ subq(r8, r9);
        __ movl(Operand(rbx, CallFrame::kOffsetPC), r8);
    }

    // base = ob:method
    // base[0] = ob[method]
    // base[1] = ob
    virtual void Self(IVal base, IVal ob, IVal method, int line, int trace_id) override {
        DCHECK_EQ(IVal::kLocal, base.kind);
        DCHECK_EQ(IVal::kLocal, ob.kind);
        DCHECK_EQ(IVal::kConst, method.kind);
        FileLineScope fls(fun_scope_, line);
        GetField(base.index, ob.index, method.Encode(), trace_id);
        __ movq(kScratch, Local(ob.index));
        __ movq(Local(base.index + 1), kScratch);
    }

    virtual void Call(IVal callee, int nargs, int wanted, int line, int trace_id) override {
        DCHECK_EQ(IVal::kLocal, callee.kind);
        FileLineScope fls(fun_scope_, line);
        Invoke(callee.index, nargs, wanted, trace_id);
    }

    virtual void Ret(IVal base, int nrets, int line) override {
        DCHECK_EQ(IVal::kLocal, base.kind);
        FileLineScope fls(fun_scope_, line);
        __ movq(kRegArgv[0], kThread);
        __ movl(kRegArgv[1], base.index);
        __ movl(kRegArgv[2], nrets);
        CallRuntime(Runtime::kThread_Ret);
        __ movq(kScratch, rax); // save return value
        
        // Setup BP
        __ movq(kBP, Operand(kThread, NyThread::kOffsetStack));
        __ movq(rax, Operand(kThread, NyThread::kOffsetFrame));
        __ cmpq(rax, 0);
        Label exit;
        __ j(Equal, &exit, false);
 
        __ movq(rax, Operand(rax, CallFrame::kOffsetStackBP));
        __ shlq(rax, kPointerShift);
        __ addq(kBP, rax);
        
        __ Bind(&exit);
        __ movq(rax, kScratch);
        //__ addq(rsp, kFuncSavedSize);
        __ popq(rbp);
        __ ret(0);
    }

    virtual void Move(IVal dst, IVal src, int line) override {
        DCHECK_EQ(IVal::kLocal, dst.kind);
        DCHECK_EQ(IVal::kLocal, src.kind);
        if (dst.index == src.index) {
            return;
        }
        FileLineScope fls(fun_scope_, line);
        __ movq(rax, Local(src.index));
        __ movq(Local(dst.index), rax);
    }

    virtual void StoreUp(IVal val, IVal up, int line) override {
        DCHECK_EQ(IVal::kLocal, val.kind);
        DCHECK_EQ(IVal::kUpval, up.kind);
        FileLineScope fls(fun_scope_, line);
        __ movq(rax, Local(val.index));
        __ movq(kRegArgv[0], kThread);
        __ movq(kRegArgv[1], rax);
        __ movl(kRegArgv[2], up.index);
        CallRuntime(Runtime::kThread_SetUpVal);
    }

    virtual void NewMap(IVal map, int n, int linear, int line) override {
        DCHECK_EQ(IVal::kLocal, map.kind);
        FileLineScope fls(fun_scope_, line);
        __ movq(kRegArgv[0], kThread);
        __ movl(kRegArgv[1], map.index);
        __ movl(kRegArgv[2], n);
        __ movl(kRegArgv[3], linear);
        __ rdrand(kRegArgv[4]);
        CallRuntime(Runtime::kThread_NewMap);
    }

    virtual void SetField(IVal self, IVal index, IVal value, int line, int trace_id) override {
        DCHECK_EQ(IVal::kLocal, self.kind);
        FileLineScope fls(fun_scope_, line);
        __ movq(kRegArgv[0], kCore);
        __ movq(kRegArgv[1], Local(self.index));
        __ movl(kRegArgv[2], Operator::kIndex);
        CallRuntime(Runtime::kNyaaCore_TryMetaFunction);
        __ cmpq(rax, 0);
        Label call_meta;
        __ j(NotEqual, &call_meta, true); // ------------> call_meta
        
        __ movq(kRegArgv[0], Local(self.index));
        LoadLocalOrConst(kRegArgv[1], index, kRegArgv[2], value, nullptr/*fallback*/);
        __ movq(kRegArgv[3], kCore);
        CallRuntime(Runtime::kObject_Put);

        Label exit;
        __ jmp(&exit, true); // -------------> exit
        
        //------------------------------------------------------------------------------------------
        // Call meta function
        //------------------------------------------------------------------------------------------
        __ Bind(&call_meta);
        IVal callee = fun_scope_->Reserve(4);
        
        __ movq(Local(callee.index), rax);
        __ movq(rax, Local(self.index));
        __ movq(Local(callee.index + 1), rax);
        LoadLocalOrConst(rax, index, rbx, value, nullptr/*fallback*/);
        __ movq(Local(callee.index + 2), rax);
        __ movq(Local(callee.index + 3), rbx);
        Invoke(callee.index, 3/*nargs*/, 0/*wanted*/, trace_id/*trace_id*/);
        
        fun_scope_->FreeVar(IVal::Local(callee.index + 3));
        fun_scope_->FreeVar(IVal::Local(callee.index + 2));
        fun_scope_->FreeVar(IVal::Local(callee.index + 1));
        fun_scope_->FreeVar(callee);
        __ Bind(&exit);
    }

    virtual void GetField(IVal value, IVal self, IVal index, int line, int trace_id) override {
        DCHECK_EQ(IVal::kLocal, self.kind);
        DCHECK_EQ(IVal::kLocal, value.kind);
        DCHECK(index.kind == IVal::kLocal  || index.kind  == IVal::kConst) << index.kind;
        FileLineScope fls(fun_scope_, line);
        GetField(value.index, self.index, index.Encode(), trace_id);
    }

    virtual IVal Localize(IVal val, int line) override {
        switch (val.kind) {
            case IVal::kLocal:
                break;
            case IVal::kUpval: {
                IVal ret = fun_scope_->NewLocal();
                FileLineScope fls(fun_scope_, line);
                __ movq(kRegArgv[0], kThread);
                __ movl(kRegArgv[1], val.index);
                CallRuntime(Runtime::kThread_GetUpVal);
                __ movq(Local(ret.index), rax);
                return ret;
            }
            case IVal::kFunction: {
                IVal ret = fun_scope_->NewLocal();
                FileLineScope fls(fun_scope_, line);
                __ movq(kRegArgv[0], kThread);
                __ movq(kRegArgv[1], val.index);
                CallRuntime(Runtime::kThread_Closure);
                __ movq(Local(ret.index), rax);
                return ret;
            }
            case IVal::kGlobal: {
                IVal ret = fun_scope_->NewLocal();
                FileLineScope fls(fun_scope_, line);
                __ movq(rax, Operand(kThread, NyThread::kOffsetFrame)); // rax = frame
                __ movq(kScratch, Operand(rax, CallFrame::kOffsetConstPool)); // scratch = const_pool

                // scratch = kpool[val.index] scratch is key
                __ movq(kScratch, ArrayAt(kScratch, val.index));
                
                __ movq(rax, Operand(kThread, NyThread::kOffsetFrame)); // rax = frame
                __ movq(rax, Operand(rax, CallFrame::kOffsetEnv)); // rax = env
                
                __ movq(kRegArgv[0], rax); // argv[0] = env
                __ movq(kRegArgv[1], kScratch); // argv[1] = key
                __ movq(kRegArgv[2], Operand(kThread, NyThread::kOffsetOwns)); // argv[2] = thread->owns_
                CallRuntime(Runtime::kMap_RawGet); // rax = rax->RawGet(r10, onws)
                //__ Breakpoint();
                __ movq(Local(ret.index), rax);
                return ret;
            }
            case IVal::kConst: {
                IVal ret = fun_scope_->NewLocal();
                FileLineScope fls(fun_scope_, line);
                __ movq(rax, Operand(kThread, NyThread::kOffsetFrame)); // rax = frame
                __ movq(kScratch, Operand(rax, CallFrame::kOffsetConstPool)); // scratch = const_pool
                __ movq(rax, ArrayAt(kScratch, val.index));
                //__ Breakpoint();
                __ movq(Local(ret.index), rax);
                return ret;
            }
            case IVal::kVoid:
            default:
                DLOG(FATAL) << "noreached.";
                break;
        }
        return val;
    }
    
    virtual void StoreGlobal(IVal val, IVal name, int line) override {
        FileLineScope fls(fun_scope_, line);
        __ movq(rax, Operand(kThread, NyThread::kOffsetFrame)); // rax = frame
        __ movq(kRegArgv[0], Operand(rax, CallFrame::kOffsetEnv)); // argv[0] = env
        __ movq(kScratch, Operand(rax, CallFrame::kOffsetConstPool)); // scratch = const_pool
        // argv[1] = key
        __ movq(kRegArgv[1], ArrayAt(kScratch, name.index));
        DCHECK_EQ(IVal::kLocal, val.kind);
        __ movq(kRegArgv[2], Local(val.index)); // argv[2] = value
        __ movq(kRegArgv[3], kCore); // argv[3] = core
        //__ Breakpoint();
        CallRuntime(Runtime::kMap_RawPut, true/*may_interrupt*/);
    }
    
    void GetField(int32_t ret, int32_t self, int32_t index, int32_t trace_id) {
        __ movq(kRegArgv[0], kCore);
        __ movq(kRegArgv[1], Local(self));
        __ movl(kRegArgv[2], Operator::kIndex);
        CallRuntime(Runtime::kNyaaCore_TryMetaFunction);
        __ cmpq(rax, 0); // rax = meta-function or nil
        Label call_meta;
        __ j(NotEqual, &call_meta, true); // ------------> call_meta
        
        __ movq(kRegArgv[0], Local(self));
        if (index < 0) { // index is const
            __ movq(kScratch, Operand(kThread, NyThread::kOffsetFrame)); // rax = frame
            __ movq(kScratch, Operand(kScratch, CallFrame::kOffsetConstPool)); // scratch = const_pool
            __ movq(kRegArgv[1], ArrayAt(kScratch, -index - 1));
        } else {
            __ movq(kRegArgv[1], Local(index));
        }
        __ movq(kRegArgv[2], kCore);
        CallRuntime(Runtime::kObject_Get);
        __ movq(Local(ret), rax);
        
        Label exit;
        __ jmp(&exit, true); // -------------> exit
        
        //------------------------------------------------------------------------------------------
        // Call meta function
        //------------------------------------------------------------------------------------------
        __ Bind(&call_meta); // <------------ call_meta
        IVal callee = fun_scope_->Reserve(3);
        
        __ movq(Local(callee.index), rax); // rax = meta-function
        __ movq(rax, Local(self));
        __ movq(Local(callee.index + 1), rax);
        if (index < 0) { // index is const
            __ movq(kScratch, Operand(kThread, NyThread::kOffsetFrame)); // rax = frame
            __ movq(kScratch, Operand(kScratch, CallFrame::kOffsetConstPool)); // scratch = const_pool
            __ movq(rax, ArrayAt(kScratch, -index - 1));
        } else {
            __ movq(rax, Local(index));
        }
        __ movq(Local(callee.index + 2), rax);
        Invoke(callee.index, 2/*nargs*/, 1/*wanted*/, trace_id/*trace_id*/);
        __ movq(rax, Local(callee.index));
        __ movq(Local(ret), rax);
        
        fun_scope_->FreeVar(IVal::Local(callee.index + 2));
        fun_scope_->FreeVar(IVal::Local(callee.index + 1));
        fun_scope_->FreeVar(callee);
        
        __ Bind(&exit); // <------------- exit
    }

    virtual void Closure(IVal closure, IVal func, int line) override {
        FileLineScope fls(fun_scope_, line);
        __ movq(kRegArgv[0], kThread);
        __ movl(kRegArgv[1], func.index);
        CallRuntime(Runtime::kThread_Closure);
        __ movq(Local(closure.index), rax);
    }
    
    virtual void New(IVal val, IVal clazz, int nargs, int line) override {
        FileLineScope fls(fun_scope_, line);
        static const int32_t kSavedStack = RoundUp(16, arch::kNativeStackAligment);
        static const Operand kStNArgs = Operand(rbp, -4);
        static const Operand kStInit = Operand(rbp, -12);
        
        __ subq(rsp, kSavedStack);
        
        //__ Breakpoint();
        __ movq(kRegArgv[0], kThread);
        __ movl(kRegArgv[1], val.index);
        __ movl(kRegArgv[2], clazz.index);
        __ movl(kStNArgs, nargs);
        __ lea(kRegArgv[3], kStNArgs);
        __ lea(kRegArgv[4], kStInit);
        CallRuntime(Runtime::kThread_PrepareNew);
        __ cmpq(kStInit, 0);
        Label call_init;
        __ j(NotEqual, &call_init, true); // ----------------> call_init
        __ movq(Local(val.index), rax);
        Label exit;
        __ jmp(&exit, true); // ----------------> exit

        __ Bind(&call_init); // <---------------- call_init
        __ movq(Local(val.index), rax);
        __ movq(rsi, kBP);
        __ movl(rax, clazz.index);
        __ shll(rax, kPointerShift);
        __ addl(rax, kPointerSize);
        __ addq(rsi, rax); // rsi = bp + (clazz.index + 1) * 8

        __ movl(rax, kStNArgs);
        __ shll(rax, kPointerShift);
        __ movq(rdi, rsi);
        __ addq(rdi, rax); // rdi = bp + (clazz.index + 1) * 8 + nargs * 8
        
        __ movq(rax, kStInit);
        __ movq(Operand(rdi, 0), rax); // rdi[0] = function init
        __ movq(rax, Local(val.index));
        __ movq(Operand(rdi, kPointerSize), rax); // rdi[1] = self

        __ addq(rdi, kPointerSize * 2); // rdi[2..] = args
        __ movl(rcx, kStNArgs);
        __ cmpl(rcx, 0);
        Label out, retry;
        __ Bind(&retry);
        __ j(LessEqual, &out, false);
        __ movq(rax, Operand(rsi, 0));
        __ movq(Operand(rdi, 0), rax);
        __ addq(rsi, kPointerSize);
        __ addq(rdi, kPointerSize);
        __ decl(rcx);
        __ jmp(&retry, false);
        
        __ Bind(&out);
        SaveRIP();
        // argv[0] = clazz.index + 1 + nargs
        __ movl(kRegArgv[0], clazz.index);
        __ incl(kRegArgv[0]);
        __ addl(kRegArgv[0], kStNArgs);
        // argv[1] = nargs + 1
        __ movl(kRegArgv[1], kStNArgs);
        __ incl(kRegArgv[1]);
        // argv[2] = 0
        __ movl(kRegArgv[2], 0); // no return
        __ movq(rax, core_->code_pool()->kCallStub->entry_address());
        __ call(rax);

        __ Bind(&exit); // <---------------- exit
        __ addq(rsp, kSavedStack);
    }

    virtual void Vargs(IVal vargs, int wanted, int line) override {
        FileLineScope fls(fun_scope_, line);
        
        __ movq(kScratch, Operand(kThread, NyThread::kOffsetFrame));
        __ movq(rcx, Operand(kScratch, CallFrame::kOffsetStackBP));
        __ subq(rcx, Operand(kScratch, CallFrame::kOffsetStackBE)); // rcx = nvargs

        __ movq(rdi, kBP);
        __ addq(rdi, vargs.index * kPointerSize);
        if (wanted < 0) {
            __ pushq(rdi);
            __ pushq(rcx);
            
            __ movq(kRegArgv[0], kThread);
            __ movq(kRegArgv[1], Operand(kScratch, CallFrame::kOffsetStackBP));
            __ shlq(rcx, kPointerShift);
            __ addq(kRegArgv[1], rcx);
            CallRuntime(Runtime::kThread_CheckStack);

            __ popq(rcx);
            __ popq(rdi);
            
            __ movq(kScratch, Operand(kThread, NyThread::kOffsetFrame));
            __ movq(rsi, Operand(kThread, NyThread::kOffsetStack));
            __ movq(rax, Operand(kScratch, CallFrame::kOffsetStackBE));
            __ shlq(rax, kPointerShift);
            __ addq(rsi, rax);
            
            __ xorq(rbx, rbx);
            Label retry, done;
            __ Bind(&retry);
            __ cmpq(rbx, rcx);
            __ LikelyJ(GreaterEqual, &done, false);
            __ movq(rax, Operand(rsi, rbx, times_ptr_size, 0));
            __ movq(Operand(rdi, rbx, times_ptr_size, 0), rax);
            __ incq(rbx);
            __ jmp(&retry, false);
            __ Bind(&done);
            
        } else {
            __ movq(kScratch, Operand(kThread, NyThread::kOffsetFrame));
            __ movq(rsi, Operand(kThread, NyThread::kOffsetStack));
            __ movq(rax, Operand(kScratch, CallFrame::kOffsetStackBE));
            __ shlq(rax, kPointerShift);
            __ addq(rsi, rax);
            
            __ xorq(rbx, rbx);
            Label retry, done;
            __ Bind(&retry);
            __ cmpq(rbx, wanted); // rbx >= wanted?
            __ LikelyJ(GreaterEqual, &done, false);
            
            __ cmpq(rbx, rcx); // rbx >= nvargs?
            Label fill_nil;
            __ UnlikelyJ(GreaterEqual, &fill_nil, false);
            __ movq(rax, Operand(rsi, rbx, times_ptr_size, 0));
            __ movq(Operand(rdi, rbx, times_ptr_size, 0), rax);
            
            Label pre_retry;
            __ jmp(&pre_retry, false);

            __ Bind(&fill_nil);
            __ xorq(rax, rax);
            __ movq(Operand(rdi, rbx, times_ptr_size, 0), rax);
            
            __ Bind(&pre_retry);
            __ incq(rbx);
            __ jmp(&retry, false);
            __ Bind(&done);
        }
        
        __ shlq(rcx, kPointerShift);
        __ addq(rdi, rcx);
        __ movq(Operand(kThread, NyThread::kOffsetStackTP), rdi);
    }
    
    virtual void Concat(IVal val, IVal base, int n, int line) override {
        FileLineScope fls(fun_scope_, line);
        if (n > 4) {
            __ movq(kRegArgv[0], kThread);
            __ movl(kRegArgv[1], val.index);
            __ movl(kRegArgv[2], base.index);
            __ movl(kRegArgv[3], n);
            CallRuntime(Runtime::kThread_Concat);
            return;
        }
        
        __ movq(kRegArgv[0], Local(base.index));
        __ movq(kRegArgv[1], kCore);
        CallRuntime(Runtime::kObject_ToString);
        __ movq(rbx, rax);

        __ movq(kRegArgv[0], kCore);
        __ movl(kRegArgv[1], Operand(rbx, NyString::kOffsetSize));
        __ addq(kRegArgv[1], 64);
        CallRuntime(Runtime::kNyaaCore_NewUninitializedString);

        __ movq(kRegArgv[0], rax);
        __ movq(kRegArgv[1], rbx);
        __ movq(kRegArgv[2], kCore);
        CallRuntime(Runtime::kString_Add);
        __ movq(Local(val.index), rax);

        for (int i = 1; i < n; ++i) {
            __ movq(kRegArgv[0], Local(base.index + i));
            __ movq(kRegArgv[1], kCore);
            CallRuntime(Runtime::kObject_ToString);
            __ movq(rbx, rax);
            
            __ movq(kRegArgv[0], Local(val.index));
            __ movq(kRegArgv[1], rbx);
            __ movq(kRegArgv[2], kCore);
            CallRuntime(Runtime::kString_Add);
            __ movq(Local(val.index), rax);
        }

        __ movq(kRegArgv[0], Local(val.index));
        __ movq(kRegArgv[1], kCore);
        CallRuntime(Runtime::kString_Done);
        __ movq(Local(val.index), rax);
    }

    bool ProcessNotOperator(IVal ret, ast::Multiple *inner, int line) {
        CodeGeneratorContext ix;
        ix.set_n_result(1);
        ix.set_keep_const(true);
        IVal lhs, rhs;
        switch (inner->op()) {
        #define DEF_BIN(op) BinaryExpression(ret, lhs, rhs, Operator::k##op, \
            Runtime::kObject_##op, line, inner->trace_id())
            case Operator::kEQ:
                lhs = inner->lhs()->Accept(this, &ix);
                rhs = inner->rhs()->Accept(this, &ix);
                DEF_BIN(NE);
                break;
            case Operator::kNE:
                lhs = inner->lhs()->Accept(this, &ix);
                rhs = inner->rhs()->Accept(this, &ix);
                DEF_BIN(EQ);
                break;
            case Operator::kLE:
                lhs = inner->lhs()->Accept(this, &ix);
                rhs = inner->rhs()->Accept(this, &ix);
                DEF_BIN(GT);
                break;
            case Operator::kLT:
                lhs = inner->lhs()->Accept(this, &ix);
                rhs = inner->rhs()->Accept(this, &ix);
                DEF_BIN(GE);
                break;
            case Operator::kGT:
                lhs = inner->lhs()->Accept(this, &ix);
                rhs = inner->rhs()->Accept(this, &ix);
                DEF_BIN(LE);
                break;
            case Operator::kGE:
                lhs = inner->lhs()->Accept(this, &ix);
                rhs = inner->rhs()->Accept(this, &ix);
                DEF_BIN(LT);
                break;
            default:
                return false;
        #undef DEF_BING
        }
        fun_scope_->FreeVar(rhs);
        fun_scope_->FreeVar(lhs);
        return true;
    }
    
    void BitInvExpression(IVal ret, IVal lhs, int line) {
        FileLineScope fls(fun_scope_, line);
        
        Label fallback;
        LoadLocalOrConst(rax, lhs, &fallback);
        
        //------------------------------------------------------------------------------------------
        // Smi Processing
        //------------------------------------------------------------------------------------------
        __ shrq(rax, 2);
        __ not_(rax);
        __ shlq(rax, 2);
        __ orq(rax, 0x1); // (rax << 2) | 0x1
        Label exit;
        __ jmp(&exit, true/*far*/);
        
        //------------------------------------------------------------------------------------------
        // Fallback Calling
        //------------------------------------------------------------------------------------------
        __ Bind(&fallback);
        __ movq(kRegArgv[0], rax);
        __ movq(kRegArgv[1], kCore);
        CallRuntime(Runtime::kObject_BitInv, false/*may*/);
        
        __ Bind(&exit);
        __ movq(Local(ret.index), rax);
    }
    
    void NotExpression(IVal ret, IVal lhs, int line) {
        FileLineScope fls(fun_scope_, line);
        
        Label fallback;
        LoadLocalOrConst(rax, lhs, &fallback);
        
        //------------------------------------------------------------------------------------------
        // Smi Processing
        //------------------------------------------------------------------------------------------
        __ shrq(rax, 2);
        __ movq(rbx, reinterpret_cast<Address>(NySmi::New(0)));
        __ movq(rcx, reinterpret_cast<Address>(NySmi::New(1)));
        __ cmpq(rax, 0);
        __ cmovq(Equal, rax, rcx);
        __ cmovq(NotEqual, rax, rbx);
        Label exit;
        __ jmp(&exit, true/*far*/);

        //------------------------------------------------------------------------------------------
        // Fallback Calling
        //------------------------------------------------------------------------------------------
        __ Bind(&fallback);
        __ movq(kRegArgv[0], rax);
        __ movq(kRegArgv[1], kCore);
        CallRuntime(Runtime::kObject_Not, false/*may*/);

        __ Bind(&exit);
        __ movq(Local(ret.index), rax);
    }
    
    void UnmExpression(IVal ret, IVal lhs, int line, int trace_id) {
        FileLineScope fls(fun_scope_, line);
        
        Label call_meta;
        __ movq(kRegArgv[0], kCore);
        __ movq(kRegArgv[1], Local(lhs.index));
        __ movl(kRegArgv[2], Operator::kUnm);
        CallRuntime(Runtime::kNyaaCore_TryMetaFunction);
        __ cmpq(rax, 0);
        __ j(NotEqual, &call_meta, true);

        Label fallback;
        LoadLocalOrConst(rbx, lhs, &fallback);
        
        //------------------------------------------------------------------------------------------
        // Smi Processing
        //------------------------------------------------------------------------------------------
        __ shrq(rbx, 2);
        __ xorq(rax, rax);
        __ subq(rax, rbx); // 0 - rbx
        __ shlq(rax, 2);
        __ orq(rax, 0x1); // (rax << 2) | 0x1
        Label exit;
        __ jmp(&exit, true/*far*/);

        //------------------------------------------------------------------------------------------
        // Fallback Calling
        //------------------------------------------------------------------------------------------
        __ Bind(&fallback);
        __ movq(kRegArgv[0], rbx);
        __ movq(kRegArgv[1], kCore);
        CallRuntime(Runtime::kObject_Unm, true/*may*/);
        __ jmp(&exit, true/*far*/);

        //------------------------------------------------------------------------------------------
        // Meta Function Calling
        //------------------------------------------------------------------------------------------
        __ Bind(&call_meta);
        IVal callee = fun_scope_->Reserve(2);
        
        __ movq(Local(callee.index), rax);
        LoadLocalOrConst(rax, lhs, nullptr/*fallback*/);
        __ movq(Local(callee.index + 1), rax);
        Invoke(callee.index, 1/*nargs*/, 1/*wanted*/, trace_id/*trace_id*/);
        __ movq(rax, Local(callee.index));

        fun_scope_->FreeVar(IVal::Local(callee.index + 1));
        fun_scope_->FreeVar(callee);
        __ Bind(&exit);
        __ movq(Local(ret.index), rax);
    }
    
    void BinaryExpression(IVal ret, IVal lhs, IVal rhs, Operator::ID op, Runtime::ExternalLink rt,
                          int line, int trace_id) {
        FileLineScope fls(fun_scope_, line);
        __ movq(kRegArgv[0], kCore);
        __ movq(kRegArgv[1], Local(lhs.index));
        __ movl(kRegArgv[2], op);
        CallRuntime(Runtime::kNyaaCore_TryMetaFunction);
        __ cmpq(rax, 0);
        Label call_meta;
        __ j(NotEqual, &call_meta, true);
        
        LoadLocalOrConst(kRegArgv[0], lhs, kRegArgv[1], rhs, nullptr/*fallback*/);
        __ movq(kRegArgv[2], kCore);
        CallRuntime(rt, true/*may*/);
        __ movq(Local(ret.index), rax);
        Label exit;
        __ jmp(&exit, true);
        
        //------------------------------------------------------------------------------------------
        // Call meta function
        //------------------------------------------------------------------------------------------
        __ Bind(&call_meta);
        IVal callee = fun_scope_->Reserve(3);

        __ movq(Local(callee.index), rax);
        LoadLocalOrConst(rax, lhs, rbx, rhs, nullptr/*fallback*/);
        __ movq(Local(callee.index + 1), rax);
        __ movq(Local(callee.index + 2), rbx);
        Invoke(callee.index, 2/*nargs*/, 1/*wanted*/, trace_id/*trace_id*/);
        switch (op) {
            case Operator::kNE:
            case Operator::kGT:
            case Operator::kGE:
                __ movq(rax, Local(callee.index));
                __ shrq(rax, 2);
                __ movq(rbx, reinterpret_cast<Address>(NySmi::New(0)));
                __ movq(rcx, reinterpret_cast<Address>(NySmi::New(1)));
                __ cmpq(rax, 0);
                __ cmovq(Equal, rax, rcx);
                __ cmovq(NotEqual, rax, rbx);
                __ movq(Local(ret.index), rax);
                break;
            default:
                __ movq(rax, Local(callee.index));
                __ movq(Local(ret.index), rax);
                break;
        }

        fun_scope_->FreeVar(IVal::Local(callee.index + 2));
        fun_scope_->FreeVar(IVal::Local(callee.index + 1));
        fun_scope_->FreeVar(callee);
        __ Bind(&exit);
    }
    
    void BitBinaryExpression(IVal ret, IVal lhs, IVal rhs, Operator::ID op, Runtime::ExternalLink rt,
                             int line) {
        FileLineScope fls(fun_scope_, line);
        Label fallback;
        LoadLocalOrConst(kRegArgv[0], lhs, kRegArgv[1], rhs, &fallback);
        // lhs and rhs are smi:
        Label exit;
        switch (op) {
            case Operator::kBitAnd:
                __ movq(rax, kRegArgv[0]);
                __ andq(rax, kRegArgv[1]);
                __ jmp(&exit, true/*far*/);
                break;
            case Operator::kBitOr:
                __ movq(rax, kRegArgv[0]);
                __ orq(rax, kRegArgv[1]);
                __ jmp(&exit, true/*far*/);
                break;
            case Operator::kBitXor:
                __ movq(rax, kRegArgv[0]);
                __ shrq(rax, 2);
                __ movq(rbx, kRegArgv[1]);
                __ shrq(rbx, 2);
                __ xorq(rax, rbx);
                __ shlq(rax, 2);
                __ orq(rax, 0x1);
                __ jmp(&exit, true/*far*/);
                break;
            default:
                break;
        }

        __ Bind(&fallback);
        __ movq(kRegArgv[2], kCore);
        CallRuntime(rt, true/*may*/);
        __ Bind(&exit);
        __ movq(Local(ret.index), rax);
    }
    
    void CallRuntime(Runtime::ExternalLink sym, bool may_interrupt = false) {
        if (may_interrupt) {
            __ movq(rbx, Operand(kThread, NyThread::kOffsetFrame));
            __ movq(r9, Operand(rbx, CallFrame::kOffsetEntry));
            __ lea(r8, Operand(rip, 0)); // r8 = rip
            __ subq(r8, r9);
            __ movl(Operand(rbx, CallFrame::kOffsetPC), r8);
        }

        __ pushq(kScratch);
        __ pushq(kThread);
        __ pushq(kBP);
        __ pushq(kCore);

        __ movp(rax, Runtime::kExternalLinks[sym]);
        __ call(rax);

        __ popq(kCore);
        __ popq(kBP);
        __ popq(kThread);
        __ popq(kScratch);

        if (may_interrupt) {
            __ movq(kScratch, core_->code_pool()->kRecoverIfNeed->entry_address());
            __ call(kScratch);
        }
    }
    
    void LoadLocalOrConst(Register r1, IVal v1, Label *fallback, bool prepare = false) {
        if (prepare || v1.kind == IVal::kConst) {
            __ movq(kScratch, Operand(kThread, NyThread::kOffsetFrame)); // rax = frame
            __ movq(kScratch, Operand(kScratch, CallFrame::kOffsetConstPool)); // scratch = const_pool
        }
        if (v1.kind == IVal::kConst) {
            __ movq(r1, ArrayAt(kScratch, v1.index));
        } else {
            __ movq(r1, Local(v1.index));
        }
        if (fallback) {
            __ testq(r1, 0x1); // is not smi?
            __ UnlikelyJ(Zero, fallback, true/*far*/);
        }
    }
    
    void LoadLocalOrConst(Register r1, IVal v1, Register r2, IVal v2, Label *fallback) {
        LoadLocalOrConst(r1, v1, nullptr, v2.kind == IVal::kConst);
        DCHECK_NE(r1.code(), r2.code());
        if (v2.kind == IVal::kConst) {
            __ movq(r2, ArrayAt(kScratch, v2.index));
        } else {
            __ movq(r2, Local(v2.index));
        }
        if (fallback) {
            __ testq(r1, 0x1); // is not smi?
            __ UnlikelyJ(Zero, fallback, true/*far*/);
            __ testq(r2, 0x1); // is not smi?
            __ UnlikelyJ(Zero, fallback, true/*far*/);
        }
    }

    Operand Local(int index) { return Operand(kBP, (index << kPointerShift)); }
    
    Operand ArrayAt(Register arr, int index) {
        return Operand(arr, NyArray::kOffsetElems + (index << kPointerShift));
    }
    
    Assembler *masm() { return &static_cast<FunctionScopeBundle *>(fun_scope_)->masm_; }
}; // class AOTGeneratorVisitor
    
Handle<NyFunction> AOT_CodeGenerate(Handle<NyString> file_name, ast::Block *root,
                                         base::Arena *arena, NyaaCore *core) {
    HandleScope handle_scope(core->stub());
    
    AOTGeneratorVisitor visitor(core, arena, file_name);
    FunctionScopeBundle scope(&visitor);
    visitor.InitializeFun(1);
    root->Accept(&visitor, nullptr);
    visitor.FinalizeRet(); // last return

    Handle<NyInt32Array> info = core->factory()->NewInt32Array(scope.line_info_.size());
    info = info->Add(scope.line_info_.data(), scope.line_info_.size(), core);

    auto buf = scope.masm_.buf();
    Handle<NyCode> code = core->factory()->NewCode(NyCode::kFunction, *info,
                                                   reinterpret_cast<const uint8_t *>(buf.data()),
                                                   buf.size());
    Handle<NyArray> kpool = scope.kpool()->Build(core);
    Handle<NyArray> fpool = scope.BuildProtos(core);
    Handle<NyFunction> result =
        core->factory()->NewFunction(nullptr/*name*/, 0/*nparams*/, true/*vargs*/, 0/*n_upvals*/,
                                     scope.max_stack(), *file_name,/*source file name*/
                                     *code,/*exec object*/ *fpool/*proto_pool*/, *kpool);
    return handle_scope.CloseAndEscape(result);
}

} // namespace nyaa
    
} // namespace mai
