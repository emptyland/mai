#include "nyaa/builtin.h"
#include "nyaa/thread.h"
#include "nyaa/function.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/object-factory.h"
#include "nyaa/runtime.h"
#include "asm/x64/asm-x64.h"

namespace mai {

namespace nyaa {
    
using namespace x64;
    
#define __ masm->

static constexpr Register kScratch = r10;
//static constexpr Register kBytecodeArraySlot = r11;
//static constexpr Register kConstantPoolSlot = r12;
//static constexpr Register kBP = r13;

//
// protptype: void entry(NyThread *thd, NyClosure *callee, RegisterContext *regs)
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
    
    // save thread stack:
    __ movq(Operand(kRegArgv[2], arch::kRCOffsetRegs + rsp.code() * kPointerSize), rsp);
    __ movq(Operand(kRegArgv[2], arch::kRCOffsetRegs + rbp.code() * kPointerSize), rbp);
    
    // set thread stack:
    __ movq(kScratch, Operand(kRegArgv[0], NyThread::kOffsetCore));
    __ incl(Operand(kScratch, NyThread::kOffsetNestedEntries)); // ++nested_entries_
    __ movq(rax, Operand(kScratch, State::kOffsetStackTop));
    __ movq(rsp, rax);
    __ movq(rbp, rsp);
    __ pushq(StackFrame::kEntryTrampolineFrame);
    __ pushq(rax); // push old stack top pointer
    __ pushq(kRegArgv[2]); // push RegisterContext pointer
    __ pushq(0); // aligment
    
    __ movq(rax, N->code_pool()->kInterpreterPump->entry_address());
    __ call(rax);
    
    __ addq(rsp, kPointerSize);
    __ popq(kScratch);
    __ addq(rsp, 2 * kPointerSize);
    
    // restore thread stack:
    __ movq(rsp, Operand(kScratch, arch::kRCOffsetRegs + rsp.code() * kPointerSize));
    __ movq(rbp, Operand(kScratch, arch::kRCOffsetRegs + rbp.code() * kPointerSize));
    
    // =============================================================================================
    __ addq(rsp, kPointerSize);
    __ popq(rbx);
    __ popq(r12);
    __ popq(r13);
    __ popq(r14);
    __ popq(r15);

    __ popq(rbp);
    __ ret(0);
}

// protptype: void entry(NyThread *thd, NyClosure *callee, RegisterContext *regs)
static void BuildInterpreterPump(Assembler *masm, NyaaCore *N) {
    __ pushq(rbp);
    __ movq(rbp, rsp);
    
    // +-------
    // | return addr
    // +-------
    // | saved frame ptr
    // +-------
    // | maker
    // +-------
    // | bytecode-array
    // +-------
    //
    __ pushq(StackFrame::kInterpreterPump);
    __ movq(kScratch, Operand(kRegArgv[1], NyClosure::kOffsetProto));
    __ pushq(Operand(kScratch, NyFunction::kOffsetBcbuf));
    //__ pushq(Operand(kScratch, NyFunction::k))
    
    
    __ addq(rsp, 2 * kPointerSize);

    __ popq(rbp);
    __ ret(0);
}

static NyCode *BuildCode(const std::string &buf, NyaaCore *N) {
    NyCode *code = N->factory()->NewCode(NyCode::kStub, nullptr,
                                         reinterpret_cast<const uint8_t *>(buf.data()), buf.size());
    return code;
}

Error BuiltinCodePool::Boot(NyaaCore *N) {
    DCHECK(kEntryTrampoline == nullptr);
    
    ::memset(kBytecodeHandlers, 0, sizeof(NyCode *) * kMaxBytecodeHandles);
    
    Assembler masm;
    
    BuildInterpreterPump(&masm, N);
    if (kInterpreterPump = BuildCode(masm.buf(), N); !kInterpreterPump) {
        return MAI_CORRUPTION("Not enough memory.");
    }
    masm.Reset();

    BuildEntryTrampoline(&masm, N);
    if (kEntryTrampoline = BuildCode(masm.buf(), N); !kEntryTrampoline) {
        return MAI_CORRUPTION("Not enough memory.");
    }
    masm.Reset();
//    BuildRecoverIfNeed(&masm, N);
//    if (kRecoverIfNeed = BuildCode(masm.buf(), N); !kRecoverIfNeed) {
//        return MAI_CORRUPTION("Not enough memory.");
//    }
//    masm.Reset();
//    BuildCallStub(&masm, N);
//    if (kCallStub = BuildCode(masm.buf(), N); !kCallStub) {
//        return MAI_CORRUPTION("Not enough memory.");
//    }
//    masm.Reset();

    return Error::OK();
}

} // namespace nyaa

} // namespace mai
