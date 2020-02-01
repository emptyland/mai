#include "lang/macro-assembler-x64.h"
#include "lang/stack-frame.h"
#include "lang/coroutine.h"

namespace mai {

namespace lang {

#define __ masm->

// For code valid testing
void Generate_ValidTestStub(MacroAssembler *masm) {
    StackFrameScope frame_scope(masm, StubStackFrame::kSize);
    __ movq(rax, Argv_0);
    __ addq(rax, Argv_1);
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
