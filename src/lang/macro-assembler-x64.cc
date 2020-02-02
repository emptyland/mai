#include "lang/macro-assembler-x64.h"
#include "lang/stack-frame.h"
#include "lang/coroutine.h"
#include "lang/isolate-inl.h"

namespace mai {

namespace lang {

#define __ masm->

// For code valid testing
void Generate_SanityTestStub(MacroAssembler *masm) {
    StackFrameScope frame_scope(masm, StubStackFrame::kSize);
    __ movq(rax, Argv_0);
    __ addq(rax, Argv_1);
}

// For function template testing
// Prototype: Dummy(Coroutine *co, uint8_t data[32]);
void Generate_FunctionTemplateTestDummy(MacroAssembler *masm, Address switch_system_stack) {
    static const int32_t kDataSize = 32;
    
    StackFrameScope frame_scope(masm);
    //==============================================================================================
    // NOTICE: The fucking clang++ optimizer will proecte: r12~r15 and rbx registers.
    // =============================================================================================
    __ SaveCxxCallerRegisters();
    // =============================================================================================

    // Save system stack and frame
    __ movq(CO, Argv_0);
    __ movq(Operand(CO, Coroutine::kOffsetSysBP), rbp);
    __ movq(Operand(CO, Coroutine::kOffsetSysSP), rsp);

    // Set bytecode handlers array
    // No need
    
    // Enter mai env:
    __ movq(rsp, Coroutine::kOffsetSP);
    __ movq(rbp, Coroutine::kOffsetBP);
    __ incl(Operand(CO, Coroutine::kOffsetReentrant)); // ++co->reentrant_;
    __ movl(Operand(CO, Coroutine::kOffsetYield), 0); // co->yield_ = 0;
    
    // Test co->entry_;
    __ movq(SCRATCH, Operand(CO, Coroutine::kOffsetEntry));
    __ movl(rax, Operand(SCRATCH, Any::kOffsetTags));
    __ testl(rax, Closure::kCxxFunction);
    Label fail;
    __ j(Zero, &fail, true/*is far*/);

    // Copy test data
    Label copy_retry, copy_done;
    __ subq(rsp, kDataSize);
    __ movq(rcx, kDataSize);
    __ Bind(&copy_retry);
    __ subq(rcx, kPointerSize);
    __ movq(rax, Operand(Argv_1, rcx, times_1, 0));
    __ movq(Operand(rsp, rcx, times_1, 0), rax);
    __ j(Zero, &copy_done, false/*is_far*/);
    __ jmp(&copy_retry, false/*is_far*/);
    __ Bind(&copy_done);
    
    __ movq(SCRATCH, Operand(CO, Coroutine::kOffsetEntry));
    __ movq(SCRATCH, Operand(SCRATCH, Closure::kOffsetProto));
    __ leaq(r11, Operand(SCRATCH, Code::kOffsetEntry));
    __ movq(SCRATCH, switch_system_stack);
    __ call(SCRATCH);

    __ addq(rsp, kDataSize);

    Label exit;
    __ jmp(&exit, false/*is_far*/);
    __ Bind(&fail);
    __ Breakpoint();

    // Exit mai env:
    __ Bind(&exit);
    __ movq(rsp, Operand(CO, Coroutine::kOffsetSP));
    __ movq(rbp, Operand(CO, Coroutine::kOffsetBP));
    
    // =============================================================================================
    // Fuck C++!
    __ RecoverCxxCallerRegisters();
}

// Switch to system stack and call
void Generate_SwitchSystemStackCall(MacroAssembler *masm) {
    StackFrameScope frame_scope(masm, StubStackFrame::kSize);

    __ movq(Operand(rbp, StubStackFrame::kOffsetMaker), StubStackFrame::kMaker);
    
    __ movq(rax, rbp);
    __ movq(rbx, rsp);
    __ movq(rbp, Operand(CO, Coroutine::kOffsetSysBP)); // recover system bp
    __ movq(rsp, Operand(CO, Coroutine::kOffsetSysSP)); // recover system sp
    __ pushq(rax); // save mai sp
    __ pushq(rbx); // save mai bp
    __ pushq(SCRATCH);
    __ pushq(CO);
    __ pushq(BC);
    __ pushq(BC_ARRAY);
    
    __ call(r11); // Call real function

    __ popq(BC_ARRAY); // Switch back to mai stack
    __ popq(BC);
    __ popq(CO);
    __ popq(SCRATCH);
    __ popq(rbx); // keep rax
    __ popq(rcx);

    __ movq(rsp, rcx); // recover mai sp
    __ movq(rbp, rbx); // recover mai bp
}

} // namespace lang

} // namespace mai
