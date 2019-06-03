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
static constexpr Register kRuntime = Runtime::kRuntime;
static constexpr Register kCore = Runtime::kCore;
static constexpr Register kBP = Runtime::kBP;
    
//
// protptype: void entry(NyThread *thd, NyCode *code, NyaaCore *N)
static void BuildEntryTrampoline(Assembler *masm, NyaaCore *N) {
    __ pushq(rbp);
    __ movq(rbp, rsp);
    __ subq(rsp, 8);
    
    __ movq(kThread, kRegArgv[0]);
    __ movq(kCore, kRegArgv[2]);
    __ movq(kRuntime, reinterpret_cast<Address>(Runtime::kExternalLinks));
    __ movq(kScratch, Operand(kThread, NyThread::kOffsetFrame));
    __ movq(kScratch, Operand(kScratch, CallFrame::kOffsetStackBP));
    __ movq(kBP, Operand(kThread, NyThread::kOffsetStack));
    __ movq(rax, kPointerSize);
    __ mulq(kScratch);
    __ addq(kBP, rax);
    
    __ movq(kScratch, kRegArgv[1]);
    __ addq(kScratch, NyCode::kOffsetInstructions);
    __ call(kScratch);
    
    __ addq(rsp, 8);
    __ popq(rbp);
    __ ret(0);
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

    return Error::OK();
}

} // namespace nyaa
    
} // namespace mai
