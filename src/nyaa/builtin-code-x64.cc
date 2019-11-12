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

//static constexpr Register kScratch = r10;
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

static NyCode *BuildCode(const std::string &buf, NyaaCore *N) {
    NyCode *code = N->factory()->NewCode(NyCode::kStub, nullptr,
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
