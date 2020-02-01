#ifndef MAI_LANG_MACRO_ASSEMBLER_X64_H_
#define MAI_LANG_MACRO_ASSEMBLER_X64_H_

#include "lang/mm.h"
#include "asm/x64/asm-x64.h"

namespace mai {

namespace lang {

using namespace mai::x64;

//SCRATCH = r12
//BC = r13
//BC_ARRAY = r14
//CO = r15
//ACC = rax
//XACC = xmm0

static constexpr Register SCRATCH = r12;
static constexpr Register BC = r13;
static constexpr Register BC_ARRAY = r14;
static constexpr Register CO = r15;
static constexpr Register ACC = rax;
static constexpr XMMRegister FACC = xmm0;

class MacroAssembler : public Assembler {
public:
    static constexpr size_t kAligmentPatchSize = 64; // Cache line size
    
    MacroAssembler() {}
    
    void AligmentPatch() {
        size_t wanted = RoundUp(buf().size(), kAligmentPatchSize);
        while (buf().size() < wanted) {
            int3();
        }
    }

    DISALLOW_IMPLICIT_CONSTRUCTORS(MacroAssembler);
}; // class MacroAssemblerX64

class StackFrameScope final {
public:
    StackFrameScope(MacroAssembler *masm, int32_t reserve_size = 0)
        : masm_(masm)
        , reserve_size_(reserve_size) {
        masm_->pushq(rbp);
        masm_->movq(rbp, rsp);
        if (reserve_size_ > 0) {
            masm_->subq(rsp, RoundUp(reserve_size_, kStackAligmentSize));
        }
    }
    
    ~StackFrameScope() {
        if (reserve_size_ > 0) {
            masm_->addq(rsp, reserve_size_);
        }
        masm_->popq(rbp);
        masm_->ret(0);
        masm_->AligmentPatch();
    }
    
    void Reserve(int32_t size) {
        if (reserve_size_ > 0) {
            int32_t request_size = RoundUp(size, kStackAligmentSize);
            masm_->subq(rsp, request_size);
            reserve_size_ += request_size;
        }
    }
    
private:
    int32_t reserve_size_ = 0;
    MacroAssembler *masm_;
}; // class StackFrameScope

} // namespace lang

} // namespace mai


#endif // MAI_LANG_MACRO_ASSEMBLER_X64_H_
