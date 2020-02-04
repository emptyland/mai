#include "lang/macro-assembler-x64.h"
#include "lang/stack-frame.h"
#include "lang/coroutine.h"
#include "lang/isolate-inl.h"
#include "lang/bytecode.h"
#include "asm/utils.h"

namespace mai {

namespace lang {

void MacroAssembler::StartBC() {
    // Jump to current pc's handler
    // SCRATCH = callee->mai_fn->bytecodes
    movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetBytecodeArray));
    // SCRATCH = &bytecodes->instructions
    addq(SCRATCH, BytecodeArray::kOffsetEntry);
    movl(rbx, Operand(rbp, BytecodeStackFrame::kOffsetPC));
    leaq(BC, Operand(SCRATCH, rbx, times_4, 0)); // [SCRATCH + rbx * 4]
    movl(rbx, Operand(BC, 0)); // get fist bytecode
    andl(rbx, 0xff000000);
    shrl(rbx, 24);
    // [BC_ARRAY + rbx * 8] jump to first bytecode handler
    jmp(Operand(BC_ARRAY, rbx, times_8, 0));
}

void MacroAssembler::JumpNextBC() {
    // Move next BC and jump to handler
    incl(Operand(rbp, BytecodeStackFrame::kOffsetPC)); // pc++ go to next bc
    addq(BC, sizeof(BytecodeInstruction)); // BC++
    movq(rbx, Operand(BC, 0)); // rbx = BC[0]
    andl(rbx, BytecodeNode::kIDMask);
    shrl(rbx, 24); // (bc & 0xff000000) >> 24
    // Jump to next bytecode handler
    jmp(Operand(BC_ARRAY, rbx, times_8, 0)); // [BC_ARRAY + rbx * 8]
}

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

// Prototype: void Trampoline(Coroutine *co)
void Generate_Trampoline(MacroAssembler *masm, Address switch_call, Address pump) {
    StackFrameScope frame_scope(masm);
    //==============================================================================================
    // NOTICE: The fucking clang++ optimizer will proecte: r12~r15 and rbx registers.
    // =============================================================================================
    __ SaveCxxCallerRegisters();
    // =============================================================================================
    
    // Save system stack and frame
    __ movq(CO, Argv_0); // Install CO register
    __ movq(Operand(CO, Coroutine::kOffsetSysBP), rbp);
    __ movq(Operand(CO, Coroutine::kOffsetSysSP), rsp);
    // TODO:
//    movq Coroutine_sys_pc(CO), @suspend

    // Set bytecode handlers array
    __ movq(SCRATCH, reinterpret_cast<Address>(&__isolate));
    __ movq(SCRATCH, Operand(SCRATCH, 0));
    __ movq(BC_ARRAY, Operand(SCRATCH, Isolate::kOffsetBytecodeHandlerEntries));
    
    Label entry;
    // if (coroutine.reentrant > 0) setup root exception is not need
    __ cmpl(Operand(CO, Coroutine::kOffsetReentrant), 0);
    __ j(Greater, &entry, true/*is_far*/);


    // Set root exception handler
    __ leaq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetCaughtNode));
    __ movq(Operand(CO, Coroutine::kOffsetCaught), SCRATCH); // coroutine.caught = &caught
    __ movq(Operand(SCRATCH, kOffsetCaught_Next), static_cast<int32_t>(0));
    __ movq(Operand(SCRATCH, kOffsetCaught_BP), rbp); // caught.bp = system rbp
    __ movq(Operand(SCRATCH, kOffsetCaught_SP), rsp); // caught.sp = system rsp
    __ leaq(rax, Operand(rip, masm->pc() + 20)); // @uncaught_handler
    __ movq(Operand(SCRATCH, kOffsetCaught_PC), rax); // caught.pc = @exception_handler
    __ jmp(&entry, true/*is_far*/);
    __ LandingPatch(16); // Jump landing area
    
    // uncaught: -----------------------------------------------------------------------------------
    // Handler root exception
    __ movq(Argv_0, CO);
    __ movq(Argv_1, rax);
    __ SwitchSystemStackCall(arch::MethodAddress(&Coroutine::Uncaught), switch_call);
    Label done;
    __ jmp(&done, true/*is_far*/);
    

    // entry: --------------------------------------------------------------------------------------
    // Function entry:
    __ Bind(&entry);
    __ movq(rbp, Operand(CO, Coroutine::kOffsetBP)); // recover mai stack
    __ movq(rsp, Operand(CO, Coroutine::kOffsetSP)); // recover mai stack
    __ movl(Operand(CO, Coroutine::kOffsetYield), 0); // coroutine.yield = 0
    __ cmpl(Operand(CO, Coroutine::kOffsetReentrant), 0);
    Label resume; // if (coroutine->reentrant > 0)
    __ j(Greater, &resume, true/*is_far*/); // if (coroutine->reentrant > 0)
    // first calling
    __ incl(Operand(CO, Coroutine::kOffsetReentrant)); // coroutine.reentrant++
    __ movq(Argv_0, Operand(CO, Coroutine::kOffsetEntry));
    __ movq(rax, pump);
    __ call(rax);
    Label uninstall;
    __ jmp(&uninstall, true/*is_far*/);


    // suspend: ------------------------------------------------------------------------------------
    // TODO: save pc;
    __ movq(Argv_0, CO);
    __ movq(Argv_1, rax);
    // call co->Suspend(acc, xacc)
    __ SwitchSystemStackCall(arch::MethodAddress(&Coroutine::Suspend), switch_call);
    __ jmp(&done, true/*is_far*/);


    // resume: -------------------------------------------------------------------------------------
    // coroutine->reentrant > 0, means: should resume this coroutine
    __ Bind(&resume);
    __ incl(Operand(CO, Coroutine::kOffsetReentrant)); // coroutine.reentrant++
    // Setup bytecode env
    // SCRATCH = bytecode array
    __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetBytecodeArray));
    __ movq(rbx, Operand(rbp, BytecodeStackFrame::kOffsetPC));
    // SCRATCH = &bytecodes->instructions
    // [SCRATCH + rbx * 4 + BytecodeArray::kOffsetEntry]
    __ leaq(BC, Operand(SCRATCH, rbx, times_4, BytecodeArray::kOffsetEntry));
    __ movq(ACC, Operand(CO, Coroutine::kOffsetACC)); // recover mai ACC
    __ movsd(FACC, Operand(CO, Coroutine::kOffsetFACC)); // recover mai FACC
    __ JumpNextBC();


    // uninstall: ----------------------------------------------------------------------------------
    // Restore native stack and frame
    // Unset root exception handler
    __ Bind(&uninstall);
    __ leaq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetCaughtNode));
    __ movq(rax, Operand(SCRATCH, kOffsetCaught_Next));
    __ movq(Operand(CO, Coroutine::kOffsetCaught), rax); // coroutine.caught = caught.next


    // done: ---------------------------------------------------------------------------------------
    // Recover system stack
    __ Bind(&done);
    __ movq(rbp, Operand(CO, Coroutine::kOffsetSysBP)); // recover system bp
    __ movq(rsp, Operand(CO, Coroutine::kOffsetSysSP)); // recover system sp
    // =============================================================================================
    // Fuck C++!
    __ RecoverCxxCallerRegisters();
}

class PartialBytecodeEmitter : public AbstractBytecodeEmitter {
public:
    PartialBytecodeEmitter(MetadataSpace *space): space_(DCHECK_NOTNULL(space)) {}
    ~PartialBytecodeEmitter() override {}
#define DEFINE_METHOD(name, ...) \
    void Emit##name(MacroAssembler *masm) override { \
        __ int3(); \
    }
    DECLARE_ALL_BYTECODE(DEFINE_METHOD)
#undef DEFINE_METHOD
protected:
    MetadataSpace *space_;
}; // class PartialBytecodeEmitter


class BytecodeEmitter : public PartialBytecodeEmitter {
public:
    class InstrAScope {
    public:
        InstrAScope(MacroAssembler *m)
            : masm(m) {
            __ movl(rbx, Operand(BC, 0));
            __ andl(rbx, BytecodeNode::kAOfABMask);
            __ negq(rbx);
        }
        
        ~InstrAScope() {
            __ JumpNextBC();
        }
    private:
        MacroAssembler *masm;
    }; // class TypeAScope
    
    BytecodeEmitter(MetadataSpace *space): PartialBytecodeEmitter(DCHECK_NOTNULL(space)) {}
    ~BytecodeEmitter() override {}

    void EmitLdar32(MacroAssembler *masm) override {
        InstrAScope instr_scope(masm);
        __ movl(rax, Operand(rbp, rbx, times_4, 0));
    }
    
    void EmitLdar64(MacroAssembler *masm) override {
        InstrAScope instr_scope(masm);
        __ movq(rax, Operand(rbp, rbx, times_4, 0));
    }
    
    void EmitLdarPtr(MacroAssembler *masm) override {
        InstrAScope instr_scope(masm);
        __ movq(rax, Operand(rbp, rbx, times_4, 0));
    }
    
    void EmitLdaf32(MacroAssembler *masm) override {
        InstrAScope instr_scope(masm);
        __ movss(xmm0, Operand(rbp, rbx, times_4, 0));
    }
    
    void EmitLdaf64(MacroAssembler *masm) override {
        InstrAScope instr_scope(masm);
        __ movsd(xmm0, Operand(rbp, rbx, times_4, 0));
    }

    void EmitStar32(MacroAssembler *masm) override {
        InstrAScope instr_scope(masm);
        __ movl(Operand(rbp, rbx, times_4, 0), rax);
    }
    
    void EmitStar64(MacroAssembler *masm) override {
        InstrAScope instr_scope(masm);
        __ movq(Operand(rbp, rbx, times_4, 0), rax);
    }
    
    void EmitStarPtr(MacroAssembler *masm) override {
        InstrAScope instr_scope(masm);
        __ movq(Operand(rbp, rbx, times_4, 0), rax);
    }
    
    void EmitReturn(MacroAssembler *masm) override {
        // Keep rax, it's return value
        __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetCallee));
        __ movq(SCRATCH, Operand(SCRATCH, Closure::kOffsetProto));
        // Has exception handlers ?
        __ cmpl(Operand(SCRATCH, Function::kOffsetExceptionTableSize), 0);
        Label done;
        __ j(Equal, &done, true/*is far*/);

        // Uninstall caught handle
        __ movq(SCRATCH, Operand(CO, Coroutine::kOffsetCaught));
        __ movq(SCRATCH, Operand(SCRATCH, kOffsetCaught_Next));
        // coroutine.caught = coroutine.caught.next
        __ movq(Operand(CO, Coroutine::kOffsetCaught), SCRATCH);

        __ Bind(&done);
        __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetCallee));
        __ movq(SCRATCH, Operand(SCRATCH, Closure::kOffsetProto));
        __ addq(rsp, Operand(SCRATCH, Function::kOffsetStackSize)); // Recover stack
        __ popq(rbp);
        __ ret(0);
    }
}; // class BytecodeEmitter


/*static*/ AbstractBytecodeEmitter *AbstractBytecodeEmitter::New(MetadataSpace *space) {
    return new BytecodeEmitter(space);
}

} // namespace lang

} // namespace mai
