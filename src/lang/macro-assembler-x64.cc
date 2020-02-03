#include "lang/macro-assembler-x64.h"
#include "lang/stack-frame.h"
#include "lang/coroutine.h"
#include "lang/isolate-inl.h"
#include "lang/bytecode.h"

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
void Generate_FunctionTemplateTestDummy(MacroAssembler *masm) {
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
    //__ Breakpoint();

    // Enter mai env:
    __ movq(rsp, Operand(CO, Coroutine::kOffsetSP));
    __ movq(rbp, Operand(CO, Coroutine::kOffsetBP));
    __ incl(Operand(CO, Coroutine::kOffsetReentrant)); // ++co->reentrant_;
    __ movl(Operand(CO, Coroutine::kOffsetYield), 0); // co->yield_ = 0;
    
    // Test co->entry_;
    __ movq(SCRATCH, Operand(CO, Coroutine::kOffsetEntry));
    __ movl(rax, Operand(SCRATCH, Any::kOffsetTags));
    __ testl(rax, Closure::kCxxFunction);
    Label fail;
    __ j(Zero, &fail, true/*is far*/);

    __ movq(SCRATCH, Operand(CO, Coroutine::kOffsetEntry));
    __ movq(SCRATCH, Operand(SCRATCH, Closure::kOffsetCode));
    __ leaq(rax, Operand(SCRATCH, Code::kOffsetEntry));
    __ call(rax);

    Label exit;
    __ jmp(&exit, false/*is_far*/);
    __ Bind(&fail);
    __ Breakpoint();

    // Exit mai env:
    __ Bind(&exit);
    __ movq(rsp, Operand(CO, Coroutine::kOffsetSysSP));
    __ movq(rbp, Operand(CO, Coroutine::kOffsetSysBP));
    
    // =============================================================================================
    // Fuck C++!
    __ RecoverCxxCallerRegisters();
}

// Switch to system stack and call
void Generate_SwitchSystemStackCall(MacroAssembler *masm) {
    StackFrameScope frame_scope(masm, StubStackFrame::kSize);

    //__ Breakpoint();
    __ movq(Operand(rbp, StubStackFrame::kOffsetMaker), StubStackFrame::kMaker);
    
    __ movq(Operand(CO, Coroutine::kOffsetBP), rbp);
    __ movq(Operand(CO, Coroutine::kOffsetSP), rsp);
    __ movq(rbp, Operand(CO, Coroutine::kOffsetSysBP)); // recover system bp
    __ movq(rsp, Operand(CO, Coroutine::kOffsetSysSP)); // recover system sp

    __ pushq(SCRATCH);
    __ pushq(CO);
    __ pushq(BC);
    __ pushq(BC_ARRAY);
    
    __ call(r11); // Call real function

    __ popq(BC_ARRAY); // Switch back to mai stack
    __ popq(BC);
    __ popq(CO);
    __ popq(SCRATCH);

    __ movq(rbp, Operand(CO, Coroutine::kOffsetBP)); // recover mai sp
    __ movq(rsp, Operand(CO, Coroutine::kOffsetSP)); // recover mai bp
}

class PartialBytecodeEmitter : public AbstractBytecodeEmitter {
public:
    PartialBytecodeEmitter(Isolate *isolate): isolate_(DCHECK_NOTNULL(isolate)) {}
    ~PartialBytecodeEmitter() override {}
#define DEFINE_METHOD(name, ...) \
    void Emit##name(MacroAssembler *masm) override { \
        __ Reset(); \
        __ int3(); \
    }
    DECLARE_ALL_BYTECODE(DEFINE_METHOD)
#undef DEFINE_METHOD
protected:
    Isolate *isolate_;
}; // class PartialBytecodeEmitter


class BytecodeEmitter : public PartialBytecodeEmitter {
public:
    BytecodeEmitter(Isolate *isolate): PartialBytecodeEmitter(DCHECK_NOTNULL(isolate)) {}
    ~BytecodeEmitter() override {}
    
    void EmitLdar64(MacroAssembler *masm) override {
        __ Reset();
        // TODO:
    }
    
    void StartBCMacro(MacroAssembler *masm) {
        // Jump to current pc's handler
        // SCRATCH = callee->mai_fn->bytecodes
        __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetBytecodeArray));
        // SCRATCH = &bytecodes->instructions
        __ addq(SCRATCH, BytecodeArray::kOffsetEntry);
        __ movl(rbx, Operand(rbp, BytecodeStackFrame::kOffsetPC));
        __ leaq(BC, Operand(SCRATCH, rbx, times_4, 0)); // [SCRATCH + rbx * 4]
        __ movl(rbx, Operand(BC, 0)); // get fist bytecode
        __ andl(rbx, 0xff000000);
        __ shrl(rbx, 24);
        // [BC_ARRAY + rbx * 8] jump to first bytecode handler
        __ jmp(Operand(BC_ARRAY, rbx, times_8, 0));
    }

}; // class BytecodeEmitter


AbstractBytecodeEmitter *NewBytecodeEmitter(Isolate *isolate) {
    TODO();
    return nullptr;
}

} // namespace lang

} // namespace mai
