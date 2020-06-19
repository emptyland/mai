#pragma once
#ifndef MAI_LANG_MACRO_ASSEMBLER_X64_H_
#define MAI_LANG_MACRO_ASSEMBLER_X64_H_

#include "lang/mm.h"
#include "asm/x64/asm-x64.h"

namespace mai {

namespace lang {

using namespace mai::x64;

class AbstractBytecodeEmitter;
class Isolate;

//SCRATCH = r12
//BC = r13
//BC_ARRAY = r14
//CO = r15
//ACC = rax
//XACC = xmm0

static constexpr Register PROFILER = r11;
static constexpr Register TRACER = r11;

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
    
    void LandingPatch(size_t n) {
        for (size_t i = 0; i < n; i++) {
            nop();
        }
    }

    void SaveCxxCallerRegisters() {
        pushq(r15);
        pushq(r14);
        pushq(r13);
        pushq(r12);
        pushq(rbx);
        pushq(rbx);
    }

    void RecoverCxxCallerRegisters() {
        popq(rbx);
        popq(rbx);
        popq(r12);
        popq(r13);
        popq(r14);
        popq(r15);
    }

    void SwitchSystemStackCall(Address cxx_func_entry, Address switch_sys_stack) {
        movq(r10, cxx_func_entry);
        movq(rax, switch_sys_stack);
        call(rax);
    }
    
    void InlineSwitchSystemStackCall(Address cxx_func_entry, bool enable_jit) {
        SaveBytecodeEnv(enable_jit);
        movq(rax, cxx_func_entry);
        call(rax); // Call real function
        RecoverBytecodeEnv(enable_jit);
    }
    
    void SaveBytecodeEnv(bool enable_jit);
    
    void RecoverBytecodeEnv(bool enable_jit);
    
    void SaveState0(Register scratch);
    
    void SaveState1(Register scratch);

    void Abort(const char *message);

    void StartBC();

    void JumpNextBC();
    
    void WriteBarrier(Register host, Register index, bool enable_jit) {
        WriteBarrier(host, Operand(host, index, times_1, 0), enable_jit);
    }

    void WriteBarrier(Register host, int32_t offset, bool enable_jit) {
        WriteBarrier(host, Operand(host, offset), enable_jit);
    }
    
    void WriteBarrier(Register host, Operand address, bool enable_jit);

    void Throw(Register scratch0, Register scratch1);
    
    void InstallCaughtHandler(bool enable_jit);
    
    void UninstallCaughtHandler();

    DISALLOW_IMPLICIT_CONSTRUCTORS(MacroAssembler);
}; // class MacroAssemblerX64

class StackFrameScope final {
public:
    StackFrameScope(MacroAssembler *masm, int32_t reserve_size = 0)
        : masm_(masm)
        , reserve_size_(RoundUp(reserve_size, kStackAligmentSize)) {
        masm_->pushq(rbp);
        masm_->movq(rbp, rsp);
        if (reserve_size_ > 0) {
            masm_->subq(rsp, reserve_size_);
        }
    }

    ~StackFrameScope() { Escape(); }
    
    void Escape() {
        if (reserve_size_ > 0) {
            masm_->addq(rsp, reserve_size_);
        }
        masm_->popq(rbp);
        masm_->ret(0);
        masm_->int3();
        masm_->AligmentPatch();
    }
    
private:
    int32_t reserve_size_ = 0;
    MacroAssembler *masm_;
}; // class StackFrameScope

void Generate_SanityTestStub(MacroAssembler *masm);
void Generate_FunctionTemplateTestDummy(MacroAssembler *masm);
void Generate_Trampoline(MacroAssembler *masm, Address switch_call, Address pump, bool enable_jit,
                         int *suspend_point_pc);
void Generate_InterpreterPump(MacroAssembler *masm, Address switch_call, bool enable_jit);
void Generate_SwitchSystemStackCall(MacroAssembler *masm, bool enable_jit);

void Patch_Tracing(MacroAssembler *masm);
void Patch_BackwardJump(MacroAssembler *masm);
void Patch_CheckStack(MacroAssembler *masm);

} // namespace lang

} // namespace mai


#endif // MAI_LANG_MACRO_ASSEMBLER_X64_H_
