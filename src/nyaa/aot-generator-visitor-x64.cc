#include "nyaa/code-gen-base.h"
#include "nyaa/thread.h"
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
        while (pc < fs_->masm_.pc()) {
            fs_->line_info_.push_back(pc++);
        }
    }
    
private:
    FunctionScopeBundle *fs_;
    int start_pc_;
    int line_;
}; // class FileLineScope
    
} // namespace
    
using namespace x64;

class Runtime {
public:
    enum ExternalLink {
        kThread_Set,
        kThread_Get,
        kThread_GetUpVal,
        kThread_GetKPool,
        kThread_GetEnv,
        kThread_GetProtoPool,
        
        kMap_RawGet,
        kMaxLinks,
    };
    using ThreadTemplate = arch::ObjectTemplate<NyThread>;
    using MapTemplate = arch::ObjectTemplate<NyMap>;
    
    static Object *Thread_GetUpVal(NyThread *thd, int slot) {
        return thd->frame_->upval(slot);
    }
    
    static NyArray *Thread_GetKPool(NyThread *thd) {
        return thd->frame_->const_poll();
    }
    
    static NyMap *Thread_GetEnv(NyThread *thd) {
        return thd->frame_->env();
    }
    
    static NyArray *Thread_GetProtoPool(NyThread *thd) {
        return thd->frame_->proto()->proto_pool();
    }
    
    static const Address kExternalLinks[kMaxLinks];
}; // class Runtime

/*static*/ const Address Runtime::kExternalLinks[kMaxLinks] = {
    ThreadTemplate::MethodAddress(&NyThread::Set),
    ThreadTemplate::MethodAddress(&NyThread::Get),
    reinterpret_cast<Address>(&Thread_GetUpVal),
    reinterpret_cast<Address>(&Thread_GetKPool),
    reinterpret_cast<Address>(&Thread_GetEnv),
    reinterpret_cast<Address>(&Thread_GetProtoPool),
    
    MapTemplate::MethodAddress(&NyMap::RawGet),
};

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

    static constexpr Register kScratch = r10;
    static constexpr Register kThread = r11;
    static constexpr Register kExternalLinks = r12;
    static constexpr Register kCore = r13;
    static constexpr Register kBP = r14;
    
    using Context = CodeGeneratorContext;

    virtual IVal VisitNilLiteral(ast::NilLiteral *node, ast::VisitorContext *) override {
        IVal tmp = fun_scope_->NewLocal();
        LoadNil(tmp, 1, node->line());
        return tmp;
    }
    
    static Handle<NyFunction> Generate(Handle<NyString> file_name, ast::Block *root,
                                       base::Arena *arena, NyaaCore *core) {
        HandleScope handle_scope(core->stub());
        
        AOTGeneratorVisitor visitor(core, arena, file_name);
        FunctionScopeBundle scope(&visitor);
        root->Accept(&visitor, nullptr);

        scope.masm_.ret(0); // last return
        auto buf = scope.masm_.buf();
        Handle<NyCode> code = core->factory()->NewCode(NyCode::kFunction,
                                                       reinterpret_cast<const uint8_t *>(buf.data()),
                                                       buf.size());
        Handle<NyArray> kpool = scope.kpool()->Build(core);
        Handle<NyArray> fpool = scope.BuildProtos(core);
        Handle<NyFunction> result = core->factory()->NewFunction(nullptr/*name*/,
                                                                 0/*nparams*/,
                                                                 true/*vargs*/,
                                                                 0/*n_upvals*/,
                                                                 scope.max_stack(),
                                                                 *file_name,/*source file name*/
                                                                 nullptr,/*TODO: source info */
                                                                 *code,/*exec object*/
                                                                 *fpool/*proto_pool*/,
                                                                 *kpool);
        return handle_scope.CloseAndEscape(result);
    }
private:
    AOTGeneratorVisitor(NyaaCore *core, base::Arena *arena, Handle<NyString> file_name)
        : CodeGeneratorVisitor(core, arena, file_name) {}
    
    virtual ~AOTGeneratorVisitor() override {}
    
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
                __ movq(Operand(kBP, ret.index * kPointerSize), rax);
                return ret;
            }
            case IVal::kFunction: {
                IVal ret = fun_scope_->NewLocal();
                FileLineScope fls(fun_scope_, line);
                __ movq(kRegArgv[0], kThread);
                CallRuntime(Runtime::kThread_GetProtoPool);
                __ movq(kScratch, rax); // scratch = GetProtoPool(thread)
                __ movq(rax, Operand(kScratch, NyArray::kOffsetElems + val.index * kPointerSize));
                __ movq(Operand(kBP, ret.index * kPointerSize), rax);
                return ret;
            }
            case IVal::kGlobal: {
                IVal ret = fun_scope_->NewLocal();
                FileLineScope fls(fun_scope_, line);
                __ movq(kRegArgv[0], kThread);
                CallRuntime(Runtime::kThread_GetKPool); // rax = GetKPool(thread)
                __ movq(kScratch, rax);
                // rax = kpool[val.index]
                __ movq(rax, Operand(kScratch, NyArray::kOffsetElems + val.index * kPointerSize));
                // scratch(r10) = rax
                __ movq(kScratch, rax);
                __ movq(kRegArgv[0], kThread);
                CallRuntime(Runtime::kThread_GetEnv); // rax = GetEnv(thread)
                __ movq(kRegArgv[0], rax);
                __ movq(kRegArgv[1], kScratch);
                // argv[1] = thread->owns_
                __ movq(kRegArgv[2], Operand(kThread, NyThread::kOffsetOwns));
                CallRuntime(Runtime::kMap_RawGet); // rax = rax->RawGet(r10, onws)
                __ movq(Operand(kBP, ret.index * kPointerSize), rax);
                return ret;
            }
            case IVal::kConst: {
                IVal ret = fun_scope_->NewLocal();
                FileLineScope fls(fun_scope_, line);
                __ movq(kRegArgv[0], kThread);
                CallRuntime(Runtime::kThread_GetKPool); // rax = GetKPool(thread)
                __ movq(kScratch, rax);
                __ movq(rax, Operand(kScratch, NyArray::kOffsetElems + val.index * kPointerSize));
                __ movq(Operand(kBP, ret.index * kPointerSize), rax);
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

    Assembler *masm() { return &static_cast<FunctionScopeBundle *>(fun_scope_)->masm_; }
    
    Operand ExternalLink(Runtime::ExternalLink sym) {
        return Operand(kExternalLinks, sym * kPointerSize);
    }
    
    void CallRuntime(Runtime::ExternalLink sym) {
        __ pushq(kScratch);
        __ pushq(kThread);
        __ pushq(kExternalLinks);
        __ pushq(kCore);
        __ pushq(kBP);
        __ call(ExternalLink(sym));
        __ popq(kBP);
        __ popq(kCore);
        __ popq(kExternalLinks);
        __ popq(kThread);
        __ popq(kScratch);
    }
}; // class AOTGeneratorVisitor
    
Handle<NyFunction> AOT_CodeGenerate(Handle<NyString> file_name, ast::Block *root,
                                         base::Arena *arena, NyaaCore *core) {
    return AOTGeneratorVisitor::Generate(file_name, root, arena, core);
}

} // namespace nyaa
    
} // namespace mai
