#include "nyaa/builtin.h"
#include "nyaa/thread.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/object-factory.h"
#include "nyaa/function.h"
#include "nyaa/runtime.h"
#include "asm/x64/asm-x64.h"

namespace mai {

namespace nyaa {
    
using namespace x64;
    
#define __ masm->

static constexpr Register kScratch = Runtime::kScratch;
static constexpr Register kThread = Runtime::kThread;
static constexpr Register kCore = Runtime::kCore;
static constexpr Register kBP = Runtime::kBP;
    
void CallRuntime(Assembler *masm, NyaaCore *N, Runtime::ExternalLink sym, bool may_interrupt = true);
    
//
// protptype: void entry(NyThread *thd, NyCode *code, NyaaCore *N, CodeContextBundle *bundle, Address suspend_point)
static void BuildEntryTrampoline(Assembler *masm, NyaaCore *N) {
    __ pushq(rbp);
    __ movq(rbp, rsp);
    //==============================================================================================
    // NOTICE: The fucking clang++ optimizer will proecte: r12~r15 and rbx registers.
    // =============================================================================================
    __ pushq(r15);
    __ pushq(r14);
    __ pushq(r13);
    __ pushq(r12);
    __ pushq(rbx);
    __ subq(rsp, kPointerSize); // for rsp alignment.
    // =============================================================================================

    //__ Breakpoint();
    __ movq(kThread, kRegArgv[0]);
    __ movq(kCore, kRegArgv[2]);
    __ movq(kScratch, Operand(kThread, NyThread::kOffsetFrame));
    __ movq(kScratch, Operand(kScratch, CallFrame::kOffsetStackBP));
    __ movq(kBP, Operand(kThread, NyThread::kOffsetStack));
    __ movq(rax, kPointerSize);
    __ mulq(kScratch);
    __ addq(kBP, rax);
    
    __ movq(kScratch, Operand(kThread, NyThread::kOffsetSavePoint));
    __ movq(Operand(kScratch, CodeContextBundle::kOffsetNaStBP), rsp);
    
    //__ Breakpoint();
    __ cmpq(kRegArgv[4], 0);
    Label should_resume;
    __ j(NotEqual, &should_resume, false);

    //----------------------------------------------------------------------------------------------
    // Directy Run:
    //----------------------------------------------------------------------------------------------
    __ movq(Operand(kThread, NyThread::kOffsetState), NyThread::kRunning);
    __ lea(rax, Operand(kRegArgv[1], NyCode::kOffsetInstructions));
    __ call(rax);

    Label exit;
    __ jmp(&exit, true);
    
    //----------------------------------------------------------------------------------------------
    // Resume:
    //----------------------------------------------------------------------------------------------
    __ Bind(&should_resume);
    
    __ movq(kScratch, kRegArgv[4]); // backup suspend_point
    __ movq(rbx, Operand(kThread, NyThread::kOffsetNaStBKSize));
    __ movq(rdi, rsp);
    __ subq(rdi, rbx); // rdi = rsp - nast_bk_size_
    __ movq(rsi, Operand(kThread, NyThread::kOffsetNaStBK));

    //__ Breakpoint();
    __ xorq(rcx, rcx); // rcx = 0;
    __ movq(rbx, Operand(kThread, NyThread::kOffsetNaStBKSize));
    __ shrq(rbx, kPointerShift);
    Label copy_done, copy_retry;
    __ Bind(&copy_retry);
    __ cmpq(rcx, rbx);
    __ j(GreaterEqual, &copy_done, false);
    __ movq(rax, Operand(rsi, rcx, times_ptr_size, 0));
    __ movq(Operand(rdi, rcx, times_ptr_size, 0), rax);
    __ incq(rcx);
    __ jmp(&copy_retry, false);
    __ Bind(&copy_done);
    
    Label adjust_done, adjust_retry;
    __ movq(rdx, rdi);
    __ addq(rdx, Operand(kThread, NyThread::kOffsetNaStBKSize)); // init: rdx = rdi + bk_size = rsp + bk_size
    __ movq(rcx, rdi); // init: rcx = rdi = rsp
    __ Bind(&adjust_retry);
    __ cmpq(rcx, rdx);
    __ j(GreaterEqual, &adjust_done, false);
    __ movq(rax, Operand(rcx, 0)); // rbx = rsp[rcx]
    __ addq(rax, rcx); // rax = rsp + rsp[rcx]
    __ movq(Operand(rcx, 0), rax); // rsp[rcx] = rax = rsp + rsp[rcx]
    __ movq(rcx, rax); // rcx = rsp + rsp[rcx]
    __ jmp(&adjust_retry, false);
    __ Bind(&adjust_done);

    __ movq(Operand(kThread, NyThread::kOffsetState), NyThread::kRunning);
    __ movq(rsp, rdi);
    __ movq(rbp, rsp);
    __ jmp(kScratch); // long jump to suspend_point
    
    // save suspend point for long jumping
    N->set_recover_point_pc(masm->pc());
    //__ Breakpoint();
    __ movq(kScratch, Operand(kThread, NyThread::kOffsetSavePoint));
    __ movq(rsp, Operand(kScratch, CodeContextBundle::kOffsetNaStBP));
    __ movq(rax, -1);

    __ Bind(&exit);
    __ addq(rsp, kPointerSize);
    __ popq(rbx);
    __ popq(r12);
    __ popq(r13);
    __ popq(r14);
    __ popq(r15);

    __ popq(rbp);
    __ ret(0);
}

//
// protptype: void entry(int32_t callee, int32_t nargs, int32_t wanted, int32_t trace_id)
static void BuildCallStub(Assembler *masm, NyaaCore *N) {
    static constexpr int32_t kSavedSize = 32;
    static const Operand kSavedBP(rbp, -kPointerSize);
    static const Operand kArgCallee(rbp, -kPointerSize - 8);
    static const Operand kArgNArgs(rbp, -kPointerSize - 12);
    static const Operand kArgWanted(rbp, -kPointerSize - 16);
    static const Operand kArgTraceId(rbp, -kPointerSize - 20);

    __ pushq(rbp);
    __ movq(rbp, rsp);
    __ subq(rsp, kSavedSize);

    // Save current bp
    __ movq(rax, Operand(kThread, NyThread::kOffsetFrame));
    __ movq(rax, Operand(rax, CallFrame::kOffsetStackBP));
    __ movq(kSavedBP, rax);
    __ movl(kArgCallee, kRegArgv[0]);
    __ movl(kArgNArgs, kRegArgv[1]);
    __ movl(kArgWanted, kRegArgv[2]);
    __ movl(kArgTraceId, kRegArgv[3]);
    
    __ movq(kRegArgv[0], kThread);
    __ movl(kRegArgv[1], kArgCallee);
    __ movl(kRegArgv[2], kArgNArgs);
    __ movl(kRegArgv[3], kArgWanted);
    __ movl(kRegArgv[4], kArgTraceId);
    CallRuntime(masm, N, Runtime::kThread_PrepareCall, true/*may_interrupt*/); // rax = nargs(new)
    __ movq(kScratch, rax); // scratch = nargs
    
    // Restore
    __ movq(kBP, Operand(kThread, NyThread::kOffsetStack));
    __ movq(rax, kSavedBP);
    __ shlq(rax, kPointerShift);
    __ addq(kBP, rax);

    //__ Breakpoint();
    __ movl(rbx, kArgCallee);
    __ movq(rax, Operand(kBP, rbx, times_8, 0)); // rax = callee

    Label fallback;
#if defined(NYAA_USE_POINTER_TYPE)
    __ movq(rax, Operand(rax, 0)); // rax = mtword_
    __ movq(rbx, static_cast<int64_t>(NyObject::kTypeMask));
    __ andq(rax, rbx);
    __ shrq(rax, NyObject::kTypeBitsOrder);
    __ cmpl(rax, kTypeClosure);
    __ j(NotEqual, &fallback, false);
#endif
    
    __ movl(rbx, kArgCallee);
    __ movq(rax, Operand(kBP, rbx, times_8, 0)); // rax = callee
    __ movq(rax, Operand(rax, NyClosure::kOffsetProto));
    __ movq(rax, Operand(rax, NyFunction::kOffsetCode));
    __ lea(rax, Operand(rax, NyCode::kOffsetInstructions));
    // TODO: use call stub
    __ call(rax); // call generated native function
    
    //----------------------------------------------------------------------------------------------
    // Post GC
    __ movq(kRegArgv[0], kCore);
    __ movq(kRegArgv[1], 0);
    __ movl(kRegArgv[2], 0);
    CallRuntime(masm, N, Runtime::kNyaaCore_GarbageCollectionSafepoint);
    //----------------------------------------------------------------------------------------------
    
    Label exit;
    __ jmp(&exit, true);
    
    __ Bind(&fallback);
    __ movq(kRegArgv[0], kThread);
    __ movq(kRegArgv[1], kBP);
    __ movl(rax, kArgCallee);
    __ shll(rax, kPointerShift);
    __ addq(kRegArgv[1], rax); // argv[1] = base
    __ movl(kRegArgv[2], kScratch); // argv[2] = nargs
    __ movl(kRegArgv[3], kArgWanted);
    CallRuntime(masm, N, Runtime::kThread_FinalizeCall, true/*may_interrupt*/);

    __ Bind(&exit);
    __ addq(rsp, kSavedSize);
    __ popq(rbp);
    __ ret(0);
}
    
static void BuildRecoverIfNeed(Assembler *masm, NyaaCore *N) {
    __ pushq(rbp);
    __ movq(rbp, rsp);

    __ movl(rbx, Operand(kThread, NyThread::kOffsetInterruptionPending));
    __ cmpl(rbx, CallFrame::kException);
    Label l_raise;
    __ UnlikelyJ(Equal, &l_raise, true); // ------------> raise
    __ cmpl(rbx, CallFrame::kYield);
    Label l_yield;
    __ UnlikelyJ(Equal, &l_yield, true); // ------------> yield
    Label exit;
    __ jmp(&exit, true); // -------------> exit
    __ Bind(&l_raise); // <------------ raise
    // Has exception, jump to suspend point
    __ movq(kScratch, Operand(kThread, NyThread::kOffsetSavePoint));
    __ movq(Operand(kScratch, CodeContextBundle::kOffsetNaStTP), rsp);
    __ movq(kRegArgv[0], kCore);
    __ movp(rbx, Runtime::kExternalLinks[Runtime::kNyaaCore_GetRecoverPoint]);
    
    __ pushq(kThread);
    __ pushq(kCore);
    __ call(rbx);
    __ popq(kCore);
    __ popq(kThread);

    __ jmp(rax); // =======> Longjump to recover point

    __ Bind(&l_yield); // <------------ yield
    // Has Yield, jump to suspend point

    __ movq(kScratch, Operand(kThread, NyThread::kOffsetSavePoint));
    __ movq(Operand(kScratch, CodeContextBundle::kOffsetNaStTP), rsp);
    __ movq(kRegArgv[0], kThread);
    __ movq(kRegArgv[1], rsp);
    __ movp(rbx, Runtime::kExternalLinks[Runtime::kThread_SaveNativeStack]);
    
    __ pushq(kThread);
    __ pushq(kCore);
    __ call(rbx);
    __ popq(kCore);
    __ popq(kThread);
    
    __ movq(kRegArgv[0], kCore);
    __ movp(rbx, Runtime::kExternalLinks[Runtime::kNyaaCore_GetRecoverPoint]);

    __ pushq(kThread);
    __ pushq(kCore);
    __ call(rbx);
    __ popq(kCore);
    __ popq(kThread);
    
    //__ subq(rsp, kSavedSize);
    __ jmp(rax); // =======> Longjump to recover point

    __ Bind(&exit); // <------------- exit
    N->set_suspend_point_pc(masm->pc());
    __ popq(rbp);
    __ ret(0);
}
    
void CallRuntime(Assembler *masm, NyaaCore *N, Runtime::ExternalLink sym, bool may_interrupt) {
    __ pushq(kScratch);
    __ pushq(kBP);
    __ pushq(kThread);
    __ pushq(kCore);
    
    __ movp(rax, Runtime::kExternalLinks[sym]);
    __ call(rax);
    
    __ popq(kCore);
    __ popq(kThread);
    __ popq(kBP);
    __ popq(kScratch);
    
    if (may_interrupt) {
        __ movq(kScratch, N->code_pool()->kRecoverIfNeed->entry_address());
        __ call(kScratch);
    }
}
    
static NyCode *BuildCode(const std::string &buf, NyaaCore *N) {
    NyCode *code = N->factory()->NewCode(NyCode::kStub,
                                         reinterpret_cast<const uint8_t *>(buf.data()), buf.size());
    return code;
}
    
Error BuiltinCodePool::Boot(NyaaCore *N) {
    DCHECK(kEntryTrampoline == nullptr);
    Assembler masm;

    BuildEntryTrampoline(&masm, N);
    if (kEntryTrampoline = BuildCode(masm.buf(), N); !kEntryTrampoline) {
        return MAI_CORRUPTION("Not enough memory.");
    }
    masm.Reset();
    BuildRecoverIfNeed(&masm, N);
    if (kRecoverIfNeed = BuildCode(masm.buf(), N); !kRecoverIfNeed) {
        return MAI_CORRUPTION("Not enough memory.");
    }
    masm.Reset();
    BuildCallStub(&masm, N);
    if (kCallStub = BuildCode(masm.buf(), N); !kCallStub) {
        return MAI_CORRUPTION("Not enough memory.");
    }
    masm.Reset();

    return Error::OK();
}

} // namespace nyaa
    
} // namespace mai
