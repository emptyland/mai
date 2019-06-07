#include "nyaa/code-gen-base.h"
#include "nyaa/function.h"
#include "nyaa/thread.h"
#include "nyaa/runtime.h"
#include "asm/x64/asm-x64.h"
#include "asm/utils.h"

namespace mai {
    
namespace nyaa {

namespace {
    
class FunctionScopeBundle final : public FunctionScope {
public:
    FunctionScopeBundle(CodeGeneratorVisitor *owns) : FunctionScope(owns) {}
    
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
        int pc = start_pc_;
        while (pc++ < fs_->masm_.pc()) {
            fs_->line_info_.push_back(line_);
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
    // The non-allocatable registers are:
    // rsp - stack pointer
    // rbp - frame pointer
    // r10 - thread
    // r12 - kpool
    // r13 - nyaa-bp
    //

    static constexpr Register kScratch = Runtime::kScratch;
    static constexpr Register kThread = Runtime::kThread;
    static constexpr Register kCore = Runtime::kCore;
    static constexpr Register kBP = Runtime::kBP;
    
    using Context = CodeGeneratorContext;
    
    AOTGeneratorVisitor(NyaaCore *core, base::Arena *arena, Handle<NyString> file_name)
        : CodeGeneratorVisitor(core, arena, file_name) {}
    
    virtual ~AOTGeneratorVisitor() override {}
    
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
        Closure(closure, rval, node->line());
        if (node->self()) {
            CodeGeneratorContext lix;
            lix.set_n_result(1);
            IVal self = node->self()->Accept(this, &lix);
            IVal index = IVal::Const(fun_scope_->kpool()->GetOrNewStr(node->name()));
            // TODO: builder()->SetField(self, index, closure);
            fun_scope_->FreeVar(self);
            fun_scope_->FreeVar(index);
            DLOG(FATAL) << "TODO:";
        } else {
            if (fun_scope_->prev() == nullptr) {
                // as global variable
                IVal name = IVal::Global(fun_scope_->kpool()->GetOrNewStr(node->name()));
                StoreGlobal(closure, name, node->line());
            } else {
                // as local variable
                blk_scope_->PutVariable(node->name(), &closure);
            }
        }
        fun_scope_->FreeVar(closure);
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

            InitializeFun();
            node->value()->Accept(this, &bix);
            Ret(IVal::Local(0), 0, node->end_line());
            
            Handle<NyInt32Array> info = core_->factory()->NewInt32Array(fun_scope.line_info_.size());
            info = info->Add(fun_scope.line_info_.data(), fun_scope.line_info_.size(), core_);
    
            auto buf = fun_scope.masm_.buf();
            Handle<NyCode> code =
                core_->factory()->NewCode(NyCode::kFunction,
                                          reinterpret_cast<const uint8_t *>(buf.data()), buf.size());
            Handle<NyArray> kpool = fun_scope.kpool()->Build(core_);
            Handle<NyArray> fpool = fun_scope.BuildProtos(core_);
            
            proto = core_->factory()->NewFunction(nullptr/*name*/,
                                                  !node->params() ? 0 : node->params()->size()/*nparams*/,
                                                  node->vargs()/*vargs*/,
                                                  fun_scope.upval_desc_size() /*n_upvals*/,
                                                  fun_scope.max_stack(),
                                                  file_name_.is_empty() ? nullptr : *file_name_,
                                                  *info, *code, *fpool, *kpool);
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
    
    void InitializeFun() {
        __ pushq(rbp);
        __ movq(rbp, rsp);
        __ subq(rsp, kPointerSize);
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
    
    virtual void Call(IVal callee, int nargs, int wanted, int line) override {
        DCHECK_EQ(IVal::kLocal, callee.kind);
        FileLineScope fls(fun_scope_, line);
        __ movq(kRegArgv[0], kThread);
        __ movl(kRegArgv[1], callee.index);
        __ movl(kRegArgv[2], nargs);
        __ movl(kRegArgv[3], wanted);
        CallRuntime(Runtime::kThread_Call, true/*may_interrupt*/);
    }
    
    virtual void Ret(IVal base, int nrets, int line) override {
        DCHECK_EQ(IVal::kLocal, base.kind);
        FileLineScope fls(fun_scope_, line);
        __ movq(kRegArgv[0], kThread);
        __ movl(kRegArgv[1], base.index);
        __ movl(kRegArgv[2], nrets);
        CallRuntime(Runtime::kThread_Ret);
        __ addq(rsp, kPointerSize);
        __ popq(rbp);
        __ ret(0);
    }
    
    virtual void Move(IVal dst, IVal src, int line) override {
        DCHECK_EQ(IVal::kLocal, dst.kind);
        DCHECK_EQ(IVal::kLocal, src.kind);
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
                __ movl(kRegArgv[1], val.index);
                CallRuntime(Runtime::kThread_GetProto);
                __ movq(Local(ret.index), rax);
                return ret;
            }
            case IVal::kGlobal: {
                IVal ret = fun_scope_->NewLocal();
                FileLineScope fls(fun_scope_, line);
                __ movq(rax, Operand(kThread, NyThread::kOffsetFrame)); // rax = frame
                __ movq(kScratch, Operand(rax, CallFrame::kOffsetConstPool)); // scratch = const_pool

                // scratch = kpool[val.index] scratch is key
                __ movq(kScratch, Operand(kScratch, NyArray::kOffsetElems + val.index * kPointerSize));
                
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
                __ movq(rax, Operand(kScratch, NyArray::kOffsetElems + val.index * kPointerSize));
                //__ Breakpoint();
                __ movq(Local(ret.index), rax);
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
    
    virtual void StoreGlobal(IVal val, IVal name, int line) override {
        FileLineScope fls(fun_scope_, line);
        __ movq(rax, Operand(kThread, NyThread::kOffsetFrame)); // rax = frame
        __ movq(kRegArgv[0], Operand(rax, CallFrame::kOffsetEnv)); // argv[0] = env
        __ movq(kScratch, Operand(rax, CallFrame::kOffsetConstPool)); // scratch = const_pool
        // argv[1] = key
        __ movq(kRegArgv[1], Operand(kScratch, NyArray::kOffsetElems + name.index * kPointerSize));
        DCHECK_EQ(IVal::kLocal, val.kind);
        __ movq(kRegArgv[2], Local(val.index)); // argv[2] = value
        CallRuntime(Runtime::kMap_RawPut, true/*may_interrupt*/);
    }

    void Closure(IVal closure, IVal func, int line) {
        FileLineScope fls(fun_scope_, line);
        __ movq(kRegArgv[0], kThread);
        __ movl(kRegArgv[1], func.index);
        CallRuntime(Runtime::kThread_Closure);
        __ movq(Local(closure.index), rax);
    }

    Assembler *masm() { return &static_cast<FunctionScopeBundle *>(fun_scope_)->masm_; }
    
    void CallRuntime(Runtime::ExternalLink sym, bool may_interrupt = false) {
        //if (may_interrupt) {
        //__ Breakpoint();
        __ movq(rbx, Operand(kThread, NyThread::kOffsetFrame));
        __ movq(r9, Operand(rbx, CallFrame::kOffsetEntry));
        __ lea(r8, Operand(rip, 0)); // r8 = rip
        __ subq(r8, r9);
        __ movl(Operand(rbx, CallFrame::kOffsetPC), r8);
        //}
        
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
            __ movl(rbx, Operand(kThread, NyThread::kOffsetInterruptionPending));
            __ cmpl(rbx, CallFrame::kException);
            Label l_raise;
            __ UnlikelyJ(Equal, &l_raise, true);
            __ cmpl(rbx, CallFrame::kYield);
            Label l_yield;
            __ UnlikelyJ(Equal, &l_yield, true);
            Label l_out;
            __ jmp(&l_out, true);
            __ Bind(&l_raise);
            // Has exception, jump to suspend point
            __ movq(kRegArgv[0], kCore);
            __ movp(rbx, Runtime::kExternalLinks[Runtime::kNyaaCore_GetSuspendPoint]);
            __ call(rbx);
            __ jmp(rax);
            __ Bind(&l_yield);
            // Has Yield, jump to suspend point
            __ int3(); // not in it
            __ Bind(&l_out);
        }
    }

    Operand Local(int index) {
        return Operand(kBP, index * kPointerSize);
    }
}; // class AOTGeneratorVisitor
    
Handle<NyFunction> AOT_CodeGenerate(Handle<NyString> file_name, ast::Block *root,
                                         base::Arena *arena, NyaaCore *core) {
    HandleScope handle_scope(core->stub());
    
    AOTGeneratorVisitor visitor(core, arena, file_name);
    FunctionScopeBundle scope(&visitor);
    visitor.InitializeFun();
    root->Accept(&visitor, nullptr);
    visitor.FinalizeRet(); // last return
    
    Handle<NyInt32Array> info = core->factory()->NewInt32Array(scope.line_info_.size());
    info = info->Add(scope.line_info_.data(), scope.line_info_.size(), core);

    auto buf = scope.masm_.buf();
    Handle<NyCode> code = core->factory()->NewCode(NyCode::kFunction,
                                                   reinterpret_cast<const uint8_t *>(buf.data()),
                                                   buf.size());
    Handle<NyArray> kpool = scope.kpool()->Build(core);
    Handle<NyArray> fpool = scope.BuildProtos(core);
    Handle<NyFunction> result =
        core->factory()->NewFunction(nullptr/*name*/, 0/*nparams*/, true/*vargs*/, 0/*n_upvals*/,
                                     scope.max_stack(), *file_name,/*source file name*/
                                     *info,/*source info */ *code,/*exec object*/
                                     *fpool/*proto_pool*/, *kpool);
    return handle_scope.CloseAndEscape(result);
}

} // namespace nyaa
    
} // namespace mai
