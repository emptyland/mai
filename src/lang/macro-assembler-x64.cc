#include "lang/macro-assembler-x64.h"
#include "lang/stack-frame.h"
#include "lang/coroutine.h"
#include "lang/runtime.h"
#include "lang/isolate-inl.h"
#include "lang/bytecode.h"
#include "asm/utils.h"

namespace mai {

namespace lang {

void MacroAssembler::StartBC() {
    //Breakpoint();
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
    jmp(Operand(BC_ARRAY, rbx, times_ptr_size, 0));
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

void MacroAssembler::Throw(Register scratch0, Register scratch1) {
    movq(scratch0, Operand(CO, Coroutine::kOffsetCaught));
    movq(scratch1, Operand(scratch0, kOffsetCaught_PC));
    jmp(scratch1);
    int3(); // Never goto there
}

void MacroAssembler::SaveBytecodeEnv() {
    movq(Operand(CO, Coroutine::kOffsetBP1), rbp);
    movq(Operand(CO, Coroutine::kOffsetSP1), rsp);
    movq(rbp, Operand(CO, Coroutine::kOffsetSysBP)); // recover system bp
    movq(rsp, Operand(CO, Coroutine::kOffsetSysSP)); // recover system sp
    movl(Operand(CO, Coroutine::kOffsetState), Coroutine::kFallIn);

    pushq(SCRATCH);
    pushq(CO);
    pushq(BC);
    pushq(BC_ARRAY);
}

void MacroAssembler::RecoverBytecodeEnv() {
    popq(BC_ARRAY); // Switch back to mai stack
    popq(BC);
    popq(CO);
    popq(SCRATCH);

    movl(Operand(CO, Coroutine::kOffsetState), Coroutine::kRunning);
    movq(rbp, Operand(CO, Coroutine::kOffsetBP1)); // recover mai sp
    movq(rsp, Operand(CO, Coroutine::kOffsetSP1)); // recover mai bp
}

void MacroAssembler::SaveState0(Register scratch) {
    movq(Operand(CO, Coroutine::kOffsetACC), ACC);
    movsd(Operand(CO, Coroutine::kOffsetFACC), FACC);
    movl(scratch, Operand(rbp, BytecodeStackFrame::kOffsetPC));
    movq(Operand(CO, Coroutine::kOffsetPC0), scratch);
    movq(Operand(CO, Coroutine::kOffsetBP0), rbp);
    movq(Operand(CO, Coroutine::kOffsetSP0), rsp);
}

void MacroAssembler::SaveState1(Register scratch) {
    movl(scratch, Operand(rbp, BytecodeStackFrame::kOffsetPC));
    movq(Operand(CO, Coroutine::kOffsetPC1), scratch);
    movq(Operand(CO, Coroutine::kOffsetBP1), rbp);
    movq(Operand(CO, Coroutine::kOffsetSP1), rsp);
}

void MacroAssembler::Abort(const char *message) {
    movq(Operand(CO, Coroutine::kOffsetACC), rax);
    movsd(Operand(CO, Coroutine::kOffsetFACC), xmm0);

    movq(Argv_0, bit_cast<Address>(message));
    movq(rax, arch::FuncAddress(&Runtime::DebugAbort));
    call(rax);
    int3(); // Never goto here
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

    // Enter mai env:
    __ movq(rsp, Operand(CO, Coroutine::kOffsetSP0));
    __ movq(rbp, Operand(CO, Coroutine::kOffsetBP0));
    __ incl(Operand(CO, Coroutine::kOffsetReentrant)); // ++co->reentrant_;
    __ movl(Operand(CO, Coroutine::kOffsetYield), 0); // co->yield_ = 0;
    __ movl(Operand(CO, Coroutine::kOffsetState), Coroutine::kRunning);
    
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

    // Exit mai env:
    __ Bind(&exit);
    __ movl(Operand(CO, Coroutine::kOffsetState), Coroutine::kDead);
    __ movq(rsp, Operand(CO, Coroutine::kOffsetSysSP));
    __ movq(rbp, Operand(CO, Coroutine::kOffsetSysBP));
    
    // =============================================================================================
    // Fuck C++!
    __ RecoverCxxCallerRegisters();
}

// Switch to system stack and call
void Generate_SwitchSystemStackCall(MacroAssembler *masm) {
    StackFrameScope frame_scope(masm, StubStackFrame::kSize);
    __ movq(Operand(rbp, StubStackFrame::kOffsetMaker), StubStackFrame::kMaker);

    __ SaveBytecodeEnv();
    __ call(r11); // Call real function
    __ RecoverBytecodeEnv();
}

// Prototype: void Trampoline(Coroutine *co)
void Generate_Trampoline(MacroAssembler *masm, Address switch_call, Address pump,
                         int *suspend_point_pc) {
    StackFrameScope frame_scope(masm, TrampolineStackFrame::kSize);
    //==============================================================================================
    // NOTICE: The fucking clang++ optimizer will proecte: r12~r15 and rbx registers.
    // =============================================================================================
    __ SaveCxxCallerRegisters();
    // =============================================================================================
    
    // Setup stack maker
    __ movl(Operand(rbp, StackFrame::kOffsetMaker), TrampolineStackFrame::kMaker);

    // Save system stack and frame
    __ movq(CO, Argv_0); // Install CO register
    __ movq(Operand(CO, Coroutine::kOffsetSysBP), rbp);
    __ movq(Operand(CO, Coroutine::kOffsetSysSP), rsp);

    // Set bytecode handlers array
    __ movq(SCRATCH, reinterpret_cast<Address>(&__isolate));
    __ movq(SCRATCH, Operand(SCRATCH, 0));
    __ movq(BC_ARRAY, Operand(SCRATCH, Isolate::kOffsetBytecodeHandlerEntries));
    __ movq(rax, Operand(SCRATCH, Isolate::kOffsetTrampolineSuspendPoint));
    __ movq(Operand(CO, Coroutine::kOffsetSysPC), rax); // Setup suspend point
    
#if defined(DEBUG) || defined(_DEBUG)
    __ cmpl(Operand(CO, Coroutine::kOffsetState), Coroutine::kRunnable);
    Label ok0;
    __ j(Equal, &ok0, false/*is_far*/);
    __ Abort("Incorrect coroutine state, required: kRunnable");
    __ Bind(&ok0);
#endif // defined(DEBUG) || defined(_DEBUG)

    Label entry;
    // if (coroutine.reentrant > 0) setup root exception is not need
    __ cmpl(Operand(CO, Coroutine::kOffsetReentrant), 0);
    __ j(Greater, &entry, true/*is_far*/);

    // Set root exception handler
    __ leaq(SCRATCH, Operand(rbp, TrampolineStackFrame::kOffsetCaughtPoint));
    __ movq(Operand(CO, Coroutine::kOffsetCaught), SCRATCH);
    __ movq(Operand(SCRATCH, kOffsetCaught_Next), static_cast<int32_t>(0));
    __ movq(Operand(SCRATCH, kOffsetCaught_BP), rbp); // caught.bp = system rbp
    __ movq(Operand(SCRATCH, kOffsetCaught_SP), rsp); // caught.sp = system rsp
    __ leaq(rax, Operand(rip, 20)); // @uncaught_handler
    __ movq(Operand(SCRATCH, kOffsetCaught_PC), rax); // caught.pc = @exception_handler
    __ jmp(&entry, true/*is_far*/);
    __ LandingPatch(20); // Jump landing area
    
    // uncaught: -----------------------------------------------------------------------------------
    // Handler root exception
    __ movq(Argv_0, CO);
    __ movq(Argv_1, rax);
    __ SwitchSystemStackCall(arch::MethodAddress(&Coroutine::Uncaught), switch_call);
    __ movl(Operand(CO, Coroutine::kOffsetState), Coroutine::kPanic);
    Label done;
    __ jmp(&done, true/*is_far*/);
    

    // entry: --------------------------------------------------------------------------------------
    // Function entry:
    __ Bind(&entry);
    __ movq(rbp, Operand(CO, Coroutine::kOffsetBP0)); // recover mai stack
    __ movq(rsp, Operand(CO, Coroutine::kOffsetSP0)); // recover mai stack
    __ movl(Operand(CO, Coroutine::kOffsetYield), 0); // coroutine.yield = 0
    __ cmpl(Operand(CO, Coroutine::kOffsetReentrant), 0);
    Label resume; // if (coroutine->reentrant > 0)
    __ j(Greater, &resume, true/*is_far*/); // if (coroutine->reentrant > 0)
    // first calling
    __ incl(Operand(CO, Coroutine::kOffsetReentrant)); // coroutine.reentrant++
    __ movl(Operand(CO, Coroutine::kOffsetState), Coroutine::kRunning);
    __ movq(Argv_0, Operand(CO, Coroutine::kOffsetEntry));
    __ movq(rax, pump);
    __ call(rax);
    __ movl(Operand(CO, Coroutine::kOffsetState), Coroutine::kDead);
    __ movq(Operand(CO, Coroutine::kOffsetCaught), static_cast<int32_t>(0));
    __ jmp(&done, true/*is_far*/);


    // suspend: ------------------------------------------------------------------------------------
    *suspend_point_pc = masm->pc(); // Save PC
    __ movq(Argv_0, CO);
    __ movq(Argv_1, rax);
    // Call co->Suspend(acc, xacc)
    __ SwitchSystemStackCall(arch::MethodAddress(&Coroutine::DidSuspend), switch_call);
    __ movl(Operand(CO, Coroutine::kOffsetState), Coroutine::kInterrupted);
    __ jmp(&done, true/*is_far*/);


    // resume: -------------------------------------------------------------------------------------
    // coroutine->reentrant > 0, means: should resume this coroutine
    __ Bind(&resume);
    __ incl(Operand(CO, Coroutine::kOffsetReentrant)); // coroutine.reentrant++
    // Setup bytecode env
    // SCRATCH = bytecode array

    __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetBytecodeArray));
    __ movl(rbx, Operand(rbp, BytecodeStackFrame::kOffsetPC));
    // SCRATCH = &bytecodes->instructions
    // [SCRATCH + rbx * 4 + BytecodeArray::kOffsetEntry]
    __ leaq(BC, Operand(SCRATCH, rbx, times_4, BytecodeArray::kOffsetEntry));
    __ movq(ACC, Operand(CO, Coroutine::kOffsetACC)); // recover mai ACC
    __ movsd(FACC, Operand(CO, Coroutine::kOffsetFACC)); // recover mai FACC
    __ movl(Operand(CO, Coroutine::kOffsetState), Coroutine::kRunning);
    __ JumpNextBC();

    // done: ---------------------------------------------------------------------------------------
    // Recover system stack
    __ Bind(&done);
    __ movq(rbp, Operand(CO, Coroutine::kOffsetSysBP)); // recover system bp
    __ movq(rsp, Operand(CO, Coroutine::kOffsetSysSP)); // recover system sp
    // =============================================================================================
    // Fuck C++!
    __ RecoverCxxCallerRegisters();
}

// Prototype: InterpreterPump(Closure *callee)
// make a interpreter env
void Generate_InterpreterPump(MacroAssembler *masm, Address switch_call) {
    __ pushq(rbp);
    __ movq(rbp, rsp);

    //__ Breakpoint();
    __ movq(SCRATCH, Operand(Argv_0, Closure::kOffsetProto)); // SCRATCH = callee->mai_fn
    // rsp -= mai_fn->stack_size and keep rbp
    __ movl(rbx, Operand(SCRATCH, Function::kOffsetStackSize));
    __ subq(rsp, rbx);
    __ movl(Operand(rbp, BytecodeStackFrame::kOffsetMaker), BytecodeStackFrame::kMaker);
    __ movl(Operand(rbp, BytecodeStackFrame::kOffsetPC), 0); // set pc = 0

    __ movq(Operand(rbp, BytecodeStackFrame::kOffsetCallee), Argv_0); // set callee
    __ movq(rbx, Operand(SCRATCH, Function::kOffsetBytecode)); // rbx = mai_fn->bytecodes
    __ movq(Operand(rbp, BytecodeStackFrame::kOffsetBytecodeArray), rbx); // set bytecode array
    __ movq(rbx, Operand(SCRATCH, Function::kOffsetConstPool)); // rbx = mai_fn->const_pool
    __ movq(Operand(rbp, BytecodeStackFrame::kOffsetConstPool), rbx); // set const_pool
    __ cmpl(Operand(SCRATCH, Function::kOffsetExceptionTableSize), 0);
    Label start;
    // if (mai_fn->has_execption_handle())
    __ j(Equal, &start, true/*is_far*/);

    // install_caught_handler: ---------------------------------------------------------------------
    __ leaq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetCaughtPoint));
    __ movq(rax, Operand(CO, Coroutine::kOffsetCaught)); // rax = next caught
    __ movq(Operand(CO, Coroutine::kOffsetCaught), SCRATCH); // coroutine.caught = &caught
    __ movq(Operand(SCRATCH, kOffsetCaught_Next), rax); // caught.next = coroutine.caught
    __ movq(Operand(SCRATCH, kOffsetCaught_BP), rbp); // caught.bp = system rbp
    __ movq(Operand(SCRATCH, kOffsetCaught_SP), rsp); // caught.sp = system rsp
    __ leaq(rax, Operand(rip, 20));
    __ movq(Operand(SCRATCH, kOffsetCaught_PC), rax); // caught.pc = @exception_dispatch
    __ jmp(&start, true/*is_far*/);
    __ LandingPatch(20);

    // exception_dispatch: -------------------------------------------------------------------------
    __ movq(SCRATCH, Operand(CO, Coroutine::kOffsetCaught));
    __ movq(rbp, Operand(SCRATCH, kOffsetCaught_BP));
    __ movq(rsp, Operand(SCRATCH, kOffsetCaught_SP)); // Recover BP and SP first
    __ movq(SCRATCH, rax); // SCRATCH will be protectd by SwitchSystemStackCall
    __ movq(Argv_0, Operand(rbp, BytecodeStackFrame::kOffsetCallee));
    __ movq(Argv_0, Operand(Argv_0, Closure::kOffsetProto));
    __ movq(Argv_1, SCRATCH); // argv[1] = exception
    __ movl(Argv_2, Operand(rbp, BytecodeStackFrame::kOffsetPC)); // argv[2] = pc
    // Switch system stack and call a c++ function
    __ InlineSwitchSystemStackCall(arch::MethodAddress(&Function::DispatchException));
    __ cmpl(rax, 0); // if (retval < 0)
    Label throw_again;
    __ j(Less, &throw_again, true/*is_far*/);
    // Do dispatch: rax is destination pc
    __ movl(Operand(rbp, BytecodeStackFrame::kOffsetPC), rax); // Update PC
    __ movq(rax, SCRATCH); // Recover saved exception
    __ StartBC();

    // throw_again: --------------------------------------------------------------------------------
    // Uncaught exception, should throw again
    __ Bind(&throw_again);
    __ movq(rbx, Operand(CO, Coroutine::kOffsetCaught));
    __ movq(rax, Operand(rbx, kOffsetCaught_Next));
    __ movq(Operand(CO, Coroutine::kOffsetCaught), rax); // coroutine.caught = coroutine.caught.next
    __ movq(rbx, Operand(rax, kOffsetCaught_PC));
    __ movq(rax, SCRATCH); // SCRATCH is saved to rax: current exception
    __ jmp(rbx); // throw again to prev handler

    // start: --------------------------------------------------------------------------------------
    // Goto first bytecode handler
    // The first bytecode can jump to second bytecode handler, and next and next next.
    __ Bind(&start);
    __ StartBC();
    // Never goto this
}

class PartialBytecodeEmitter : public AbstractBytecodeEmitter {
public:
    PartialBytecodeEmitter(MetadataSpace *space): space_(DCHECK_NOTNULL(space)) {}
    ~PartialBytecodeEmitter() override {}
#define DEFINE_METHOD(name, ...) \
    void Emit##name(MacroAssembler *masm) override { \
        __ Abort("[" #name "] Not implement"); \
    }
    DECLARE_ALL_BYTECODE(DEFINE_METHOD)
#undef DEFINE_METHOD
protected:
    MetadataSpace *space_;
}; // class PartialBytecodeEmitter


class BytecodeEmitter : public PartialBytecodeEmitter {
public:
    static constexpr int kStackDelta = static_cast<int>(kParameterSpaceOffset/kStackOffsetGranularity);
    
    class InstrBaseScope {
    public:
        InstrBaseScope(MacroAssembler *m): masm(m) {}
        ~InstrBaseScope() { __ JumpNextBC(); }
    protected:
        MacroAssembler *masm;
    }; // class InstrBaseScope
    
    class InstrImmAScope : public InstrBaseScope {
    public:
        InstrImmAScope(MacroAssembler *m): InstrBaseScope(m) { GetATo(rbx); }
        
        void GetATo(Register dst) {
            __ movl(dst, Operand(BC, 0));
            __ andl(dst, BytecodeNode::kAOfAMask);
        }
        
        void GetAToRBX() { GetATo(rbx); }
    }; // class InstrImmAScope
    
    class InstrStackAScope : public InstrImmAScope {
    public:
        InstrStackAScope(MacroAssembler *m): InstrImmAScope(m) { GetAToRBX(); }

        void GetAToRBX() {
            GetATo(rcx);
            __ movq(rbx, kStackDelta);
            __ subq(rbx, rcx);
        }
    }; // class InstrStackAScope
    
    class InstrImmABScope : public InstrBaseScope {
    public:
        InstrImmABScope(MacroAssembler *m)
            : InstrBaseScope(m) {
            GetATo(rbx);
        }
        
        void GetATo(Register dst) {
            __ movl(dst, Operand(BC, 0));
            __ andl(dst, BytecodeNode::kAOfABMask);
            __ shrl(dst, 12);
        }
        
        void GetBTo(Register dst) {
            __ movl(dst, Operand(BC, 0));
            __ andl(dst, BytecodeNode::kBOfABMask);
        }
    }; // class InstrImmABScope
    
    class InstrStackBAScope : public InstrImmABScope {
    public:
        InstrStackBAScope(MacroAssembler *m)
            : InstrImmABScope(m) {
            GetBToRBX();
        }
        
        void GetBToRBX() {
            GetBTo(rcx);
            __ movq(rbx, kStackDelta);
            __ subq(rbx, rcx);
        }
        
        void GetAToRBX() {
            GetATo(rcx);
            __ movq(rbx, kStackDelta);
            __ subq(rbx, rcx);
        }
    }; // class InstrBAScope
    
    class InstrStackABScope : public InstrImmABScope {
    public:
        InstrStackABScope(MacroAssembler *m)
            : InstrImmABScope(m) {
            GetAToRBX();
        }

        void GetBToRBX() {
            GetBTo(rcx);
            __ movq(rbx, kStackDelta);
            __ subq(rbx, rcx);
        }
        
        void GetAToRBX() {
            GetATo(rcx);
            __ movq(rbx, kStackDelta);
            __ subq(rbx, rcx);
        }
    }; // class InstrABScope
    
    class InstrStackImmABScope : public InstrImmABScope {
    public:
        InstrStackImmABScope(MacroAssembler *m)
            : InstrImmABScope(m) {
            GetATo(rcx);
            __ movq(rbx, kStackDelta);
            __ subq(rbx, rcx);
        }
        
        void GetBToRBX() { GetBTo(rbx); }
    }; // class InstrStackImmABScope
    
    class InstrStackImmBAScope : public InstrImmABScope {
    public:
        InstrStackImmBAScope(MacroAssembler *m)
            : InstrImmABScope(m) {
            GetBTo(rbx);
        }
        
        void GetAToRBX() {
            GetATo(rcx);
            __ movq(rbx, kStackDelta);
            __ subq(rbx, rcx);
        }
    }; // class InstrStackOffsetABScope
    
    class InstrFABaseScope : public InstrBaseScope {
    public:
        InstrFABaseScope(MacroAssembler *m): InstrBaseScope(m) {}
        
        void GetFTo(Register dst) {
            __ movl(dst, Operand(BC, 0));
            __ andl(dst, BytecodeNode::kFOfFAMask);
            __ shrl(dst, 16);
        }
        
        void GetATo(Register dst) {
            __ movl(dst, Operand(BC, 0));
            __ andl(dst, BytecodeNode::kAOfFAMask);
        }
    }; // class InstrFABaseScope
    
    class InstrImmFAScope : public InstrFABaseScope{
    public:
        InstrImmFAScope(MacroAssembler *m): InstrFABaseScope(m) {
            GetFTo(rbx);
        }
    }; // class InstrImmFAScope
    
    class InstrImmAFScope : public InstrFABaseScope{
    public:
        InstrImmAFScope(MacroAssembler *m): InstrFABaseScope(m) {
            GetATo(rbx);
        }
    }; // class InstrImmFAScope
    
    BytecodeEmitter(MetadataSpace *space): PartialBytecodeEmitter(DCHECK_NOTNULL(space)) {}
    ~BytecodeEmitter() override {}

    // Load to ACC ---------------------------------------------------------------------------------
    void EmitLdar32(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitLdar64(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movq(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitLdarPtr(MacroAssembler *masm) override { EmitLdar64(masm); }
    
    void EmitLdaf32(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movss(FACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitLdaf64(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movsd(FACC, Operand(rbp, rbx, times_2, 0));
    }

    // Load imm
    void EmitLdaTrue(MacroAssembler *masm) override {
        InstrBaseScope instr_scope(masm);
        __ movq(ACC, 1);
    }

    void EmitLdaFalse(MacroAssembler *masm) override { EmitLdaZero(masm); }

    void EmitLdaZero(MacroAssembler *masm) override {
        InstrBaseScope instr_scope(masm);
        __ xorq(ACC, ACC);
        __ xorpd(FACC, FACC);
    }

    void EmitLdaSmi32(MacroAssembler *masm) override {
        InstrImmAScope instr_scope(masm);
        __ movq(ACC, rbx);
    }

    #if defined(DEBUG) || defined(_DEBUG)
    #define CHECK_CAPTURED_VAR_INDEX() \
        __ cmpl(rbx, Operand(SCRATCH, Closure::kOffsetCapturedVarSize)); \
        Label ok1; \
        __ j(Less, &ok1, false); \
        __ Abort("CapturedVarIndex out of bound!"); \
        __ Bind(&ok1)
    #else
    #define CHECK_CAPTURED_VAR_INDEX() (void)0
    #endif // defined(DEBUG) || defined(_DEBUG)
    
    class InstrCapturedVarScope : public InstrImmAScope {
    public:
        InstrCapturedVarScope(MacroAssembler *m)
            : InstrImmAScope(m) {
            __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetCallee));
            CHECK_CAPTURED_VAR_INDEX();
            __ movq(SCRATCH, Operand(SCRATCH, rbx, times_ptr_size, Closure::kOffsetCapturedVar));
        }
    };

    void EmitLdaCaptured32(MacroAssembler *masm) override {
        InstrCapturedVarScope instr_scope(masm);
        __ lfence();
        __ movl(ACC, Operand(SCRATCH, CapturedValue::kOffsetValue));
    }
    
    void EmitLdaCaptured64(MacroAssembler *masm) override {
        InstrCapturedVarScope instr_scope(masm);
        __ lfence();
        __ movq(ACC, Operand(SCRATCH, CapturedValue::kOffsetValue));
    }
    
    void EmitLdaCapturedPtr(MacroAssembler *masm) override { EmitLdaCaptured64(masm); }
    
    void EmitLdaCapturedf32(MacroAssembler *masm) override {
        InstrCapturedVarScope instr_scope(masm);
        __ lfence();
        __ movss(FACC, Operand(SCRATCH, CapturedValue::kOffsetValue));
    }
    
    void EmitLdaCapturedf64(MacroAssembler *masm) override {
        InstrCapturedVarScope instr_scope(masm);
        __ lfence();
        __ movsd(FACC, Operand(SCRATCH, CapturedValue::kOffsetValue));
    }

    // Load constant
    void EmitLdaConst32(MacroAssembler *masm) override {
        InstrImmAScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetConstPool));
        __ movl(ACC, Operand(SCRATCH, rbx, times_4, 0));
    }

    void EmitLdaConst64(MacroAssembler *masm) override {
        InstrImmAScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetConstPool));
        __ movq(ACC, Operand(SCRATCH, rbx, times_4, 0));
    }

    void EmitLdaConstPtr(MacroAssembler *masm) override { EmitLdaConst64(masm); }

    void EmitLdaConstf32(MacroAssembler *masm) override {
        InstrImmAScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetConstPool));
        __ movss(FACC, Operand(SCRATCH, rbx, times_4, 0));
    }

    void EmitLdaConstf64(MacroAssembler *masm) override {
        InstrImmAScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetConstPool));
        __ movsd(FACC, Operand(SCRATCH, rbx, times_4, 0));
    }
    
    #if defined(DEBUG) || defined(_DEBUG)
    #define CHECK_GLOBAL_OFFSET() \
        __ movq(rcx, rbx); \
        __ shlq(rcx, kGlobalSpaceOffsetGranularityShift); \
        __ cmpl(rcx, Operand(CO, Coroutine::kOffsetGlobalLength)); \
        Label ok; \
        __ j(Less, &ok, false/*is_far*/); \
        __ Abort("Global offset out of bound"); \
        __ Bind(&ok)
    #else // defined(DEBUG) || defined(_DEBUG)
    #define CHECK_GLOBAL_OFFSET()
    #endif // !defined(DEBUG) && !defined(_DEBUG)
    
    // Load Global
    void EmitLdaGlobal32(MacroAssembler *masm) override {
        InstrImmAScope instr_scope(masm);
        CHECK_GLOBAL_OFFSET();
        __ movq(SCRATCH, Operand(CO, Coroutine::kOffsetGlobalGuard));
        __ movl(ACC, Operand(SCRATCH, rbx, times_4, 0));
    }
    
    void EmitLdaGlobal64(MacroAssembler *masm) override {
        InstrImmAScope instr_scope(masm);
        CHECK_GLOBAL_OFFSET();
        __ movq(SCRATCH, Operand(CO, Coroutine::kOffsetGlobalGuard));
        __ movq(ACC, Operand(SCRATCH, rbx, times_4, 0));
    }
    
    void EmitLdaGlobalPtr(MacroAssembler *masm) override { EmitLdaGlobal64(masm); }

    void EmitLdaGlobalf32(MacroAssembler *masm) override {
        InstrImmAScope instr_scope(masm);
        CHECK_GLOBAL_OFFSET();
        __ movq(SCRATCH, Operand(CO, Coroutine::kOffsetGlobalGuard));
        __ movss(FACC, Operand(SCRATCH, rbx, times_4, 0));
    }
    
    void EmitLdaGlobalf64(MacroAssembler *masm) override {
        InstrImmAScope instr_scope(masm);
        CHECK_GLOBAL_OFFSET();
        __ movq(SCRATCH, Operand(CO, Coroutine::kOffsetGlobalGuard));
        __ movsd(FACC, Operand(SCRATCH, rbx, times_4, 0));
    }

    // Load Property
    void EmitLdaProperty8(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ xorq(ACC, ACC);
        //__ lfence(); // XXX
        __ movb(ACC, Operand(SCRATCH, rbx, times_1, 0));
    }
    
    void EmitLdaProperty16(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ xorq(ACC, ACC);
        //__ lfence(); // XXX
        __ movw(ACC, Operand(SCRATCH, rbx, times_1, 0));
    }

    void EmitLdaProperty32(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        //__ lfence(); // XXX
        __ movl(ACC, Operand(SCRATCH, rbx, times_1, 0));
    }
    
    void EmitLdaProperty64(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        //__ lfence(); // XXX
        __ movq(ACC, Operand(SCRATCH, rbx, times_1, 0));
    }
    
    void EmitLdaPropertyPtr(MacroAssembler *masm) override { EmitLdaProperty64(masm); }
    
    void EmitLdaPropertyf32(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        //__ lfence(); // XXX
        __ movss(FACC, Operand(SCRATCH, rbx, times_1, 0));
    }

    void EmitLdaPropertyf64(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        //__ lfence(); // XXX
        __ movsd(FACC, Operand(SCRATCH, rbx, times_1, 0));
    }
    
    void EmitLdaVtableFunction(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        __ cmpq(SCRATCH, 0);
        
        __ movq(SCRATCH, Operand(SCRATCH, Any::kOffsetKlass));
        __ andq(SCRATCH, ~1); // SCRATCH = class
    #if defined(DEBUG) || defined(_DEBUG)
        __ cmpq(SCRATCH, 0);
        Label ok1;
        __ j(NotEqual, &ok1, false/*is_far*/);
        __ Abort("nil kclass field!");
        __ Bind(&ok1);
    #endif // defined(DEBUG) || defined(_DEBUG)

        __ movq(SCRATCH, Operand(SCRATCH, Class::kOffsetMethods));
        instr_scope.GetBToRBX();
        __ movq(rax, static_cast<int32_t>(sizeof(Method)));
        __ mulq(rbx); // rax = rax * rbx;
        __ leaq(SCRATCH, Operand(SCRATCH, rax, times_1, 0));
        __ movq(rax, Operand(SCRATCH, Method::kOffsetFunction));
    }

    // Store from ACC ------------------------------------------------------------------------------
    void EmitStar32(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movl(Operand(rbp, rbx, times_2, 0), ACC);
    }

    void EmitStar64(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movq(Operand(rbp, rbx, times_2, 0), ACC);
    }

    void EmitStarPtr(MacroAssembler *masm) override { EmitStar64(masm); }

    void EmitStaf32(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movss(Operand(rbp, rbx, times_2, 0), FACC);
    }

    void EmitStaf64(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movsd(Operand(rbp, rbx, times_2, 0), FACC);
    }
    
    void EmitStaCaptured32(MacroAssembler *masm) override {
        InstrCapturedVarScope instr_scope(masm);
        __ movl(Operand(SCRATCH, CapturedValue::kOffsetValue), ACC);
        __ sfence();
    }
    
    void EmitStaCaptured64(MacroAssembler *masm) override {
        InstrCapturedVarScope instr_scope(masm);
        __ movq(Operand(SCRATCH, CapturedValue::kOffsetValue), ACC);
        __ sfence();
    }
    
    void EmitStaCapturedPtr(MacroAssembler *masm) override {
        //__ Abort("TODO: Write barrier");
        InstrCapturedVarScope instr_scope(masm);
        __ movq(Operand(SCRATCH, CapturedValue::kOffsetValue), ACC);
        __ sfence();
        
        // Write Barrier
        // Check host is new-generation ?
        __ cmpq(SCRATCH, Operand(CO, Coroutine::kOffsetHeapGuard0));
        Label host_is_old;
        __ j(Less, &host_is_old, false/*is_far*/);
        __ cmpq(SCRATCH, Operand(CO, Coroutine::kOffsetHeapGuard1));
        __ j(GreaterEqual, &host_is_old, false/*is_far*/);
        Label store_free;
        __ jmp(&store_free, false/*is_far*/);

        __ Bind(&host_is_old); // Host ensure is old-generation
        __ movq(Argv_0, SCRATCH); // argv[0] = host
        __ leaq(Argv_1, Operand(SCRATCH, CapturedValue::kOffsetValue)); // argv[1] = address
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::WriteBarrierWithAddress));

        __ Bind(&store_free);
    }
    
    void EmitStaCapturedf32(MacroAssembler *masm) override {
        InstrCapturedVarScope instr_scope(masm);
        __ movss(Operand(SCRATCH, CapturedValue::kOffsetValue), FACC);
        __ sfence();
    }
    
    void EmitStaCapturedf64(MacroAssembler *masm) override {
        InstrCapturedVarScope instr_scope(masm);
        __ movsd(Operand(SCRATCH, CapturedValue::kOffsetValue), FACC);
        __ sfence();
    }
    
    // Store Global
    void EmitStaGlobal32(MacroAssembler *masm) override {
        InstrImmAScope instr_scope(masm);
        CHECK_GLOBAL_OFFSET();
        __ movq(SCRATCH, Operand(CO, Coroutine::kOffsetGlobalGuard));
        __ movl(Operand(SCRATCH, rbx, times_4, 0), ACC);
    }
    
    void EmitStaGlobal64(MacroAssembler *masm) override {
        InstrImmAScope instr_scope(masm);
        CHECK_GLOBAL_OFFSET();
        __ movq(SCRATCH, Operand(CO, Coroutine::kOffsetGlobalGuard));
        __ movq(Operand(SCRATCH, rbx, times_4, 0), ACC);
    }
    
    void EmitStaGlobalPtr(MacroAssembler *masm) override { EmitStaGlobal64(masm); }
    
    void EmitStaGlobalf32(MacroAssembler *masm) override {
        InstrImmAScope instr_scope(masm);
        CHECK_GLOBAL_OFFSET();
        __ movq(SCRATCH, Operand(CO, Coroutine::kOffsetGlobalGuard));
        __ movss(Operand(SCRATCH, rbx, times_4, 0), FACC);
    }
    
    void EmitStaGlobalf64(MacroAssembler *masm) override {
        InstrImmAScope instr_scope(masm);
        CHECK_GLOBAL_OFFSET();
        __ movq(SCRATCH, Operand(CO, Coroutine::kOffsetGlobalGuard));
        __ movsd(Operand(SCRATCH, rbx, times_4, 0), FACC);
    }
    
    // Store Property
    void EmitStaProperty8(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movb(Operand(SCRATCH, rbx, times_1, 0), ACC);
        //__ sfence(); // XXX
    }
    
    void EmitStaProperty16(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movw(Operand(SCRATCH, rbx, times_1, 0), ACC);
        //__ sfence(); // XXX
    }

    void EmitStaProperty32(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movl(Operand(SCRATCH, rbx, times_1, 0), ACC);
        //__ sfence(); // XXX
    }
    
    void EmitStaProperty64(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movq(Operand(SCRATCH, rbx, times_1, 0), ACC);
        //__ sfence(); // XXX
    }
    
    void EmitStaPropertyPtr(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movq(Operand(SCRATCH, rbx, times_1, 0), ACC);
        
        // Write Barrier
        // Check host is new-generation ?
        __ cmpq(SCRATCH, Operand(CO, Coroutine::kOffsetHeapGuard0));
        Label host_is_old;
        __ j(Less, &host_is_old, false/*is_far*/);
        __ cmpq(SCRATCH, Operand(CO, Coroutine::kOffsetHeapGuard1));
        __ j(GreaterEqual, &host_is_old, false/*is_far*/);
        Label store_free;
        __ jmp(&store_free, false/*is_far*/);

        __ Bind(&host_is_old); // Host ensure is old-generation
        __ movq(Argv_0, SCRATCH); // argv[0] = host
        __ movl(Argv_1, rbx); // argv[1] = offset
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::WriteBarrierWithOffset));

        __ Bind(&store_free);
    }
    
    void EmitStaPropertyf32(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movss(Operand(SCRATCH, rbx, times_1, 0), FACC);
        //__ sfence(); // XXX
    }

    void EmitStaPropertyf64(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movsd(Operand(SCRATCH, rbx, times_1, 0), FACC);
        //__ sfence(); // XXX
    }

    // Move from Stack to Stack --------------------------------------------------------------------
    void EmitMove32(MacroAssembler *masm) override {
        InstrStackBAScope instr_scope(masm);
        __ movl(rax, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetAToRBX();
        __ movl(Operand(rbp, rbx, times_2, 0), rax);
    }

    void EmitMove64(MacroAssembler *masm) override {
        InstrStackBAScope instr_scope(masm);
        __ movq(rax, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetAToRBX();
        __ movq(Operand(rbp, rbx, times_2, 0), rax);
    }

    void EmitMovePtr(MacroAssembler *masm) override { EmitMove64(masm); }
    
    // Binary Operation ----------------------------------------------------------------------------
    void EmitAdd32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ addl(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitAdd64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ addq(ACC, Operand(rbp, rbx, times_2, 0));
    }

    void EmitAddf32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movss(FACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ addss(FACC, Operand(rbp, rbx, times_2, 0));
    }

    void EmitAddf64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movsd(FACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ addsd(FACC, Operand(rbp, rbx, times_2, 0));
    }

    void EmitSub32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ subl(ACC, Operand(rbp, rbx, times_2, 0));
    }

    void EmitSub64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ subq(ACC, Operand(rbp, rbx, times_2, 0));
    }

    void EmitSubf32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movss(FACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ subss(FACC, Operand(rbp, rbx, times_2, 0));
    }

    void EmitSubf64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movsd(FACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ subsd(FACC, Operand(rbp, rbx, times_2, 0));
    }

    void EmitMul32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        // rax:rdx <- rax * operand
        __ mull(Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitMul64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        // rax:rdx <- rax * operand
        __ mulq(Operand(rbp, rbx, times_2, 0));
    }

    void EmitMulf32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movss(FACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ mulss(FACC, Operand(rbp, rbx, times_2, 0));
    }

    void EmitMulf64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movsd(FACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ mulsd(FACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitIMul32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ imull(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitIMul64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(rax, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ imulq(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitDiv32(MacroAssembler *masm) override {
        // TODO: Div by zero
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        __ xorl(rdx, rdx);
        instr_scope.GetBToRBX();
        // rax:rdx <- rax / operand
        __ divl(Operand(rbp, rbx, times_2, 0));
    }

    void EmitDiv64(MacroAssembler *masm) override {
        // TODO: Div by zero
        InstrStackABScope instr_scope(masm);
        __ movq(ACC, Operand(rbp, rbx, times_2, 0));
        __ xorq(rdx, rdx);
        instr_scope.GetBToRBX();
        // rax:rdx <- rax / operand
        __ divq(Operand(rbp, rbx, times_2, 0));
    }

    void EmitDivf32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movss(FACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ divss(FACC, Operand(rbp, rbx, times_2, 0));
    }

    void EmitDivf64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movsd(FACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ divsd(FACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitIDiv32(MacroAssembler *masm) override {
        // TODO: Div by zero
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ idivl(Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitIDiv64(MacroAssembler *masm) override {
        // TODO: Div by zero
        InstrStackABScope instr_scope(masm);
        __ movq(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ idivq(Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitMod32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        // rax:rdx <- rax * operand
        __ divl(Operand(rbp, rbx, times_2, 0));
        __ movl(ACC, rdx);
    }
    
    void EmitMod64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        // rax:rdx <- rax * operand
        __ divq(Operand(rbp, rbx, times_2, 0));
        __ movq(ACC, rdx);
    }
    
    void EmitBitwiseAnd32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ andl(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitBitwiseAnd64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ andq(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitBitwiseOr32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ orl(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitBitwiseOr64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ orq(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitBitwiseXor32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ xorl(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitBitwiseXor64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ xorq(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitBitwiseShl32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movl(rcx, Operand(rbp, rbx, times_2, 0));
        __ shll(ACC);
    }
    
    void EmitBitwiseShl64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movl(rcx, Operand(rbp, rbx, times_2, 0));
        __ shlq(ACC);
    }
    
    void EmitBitwiseShr32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movl(rcx, Operand(rbp, rbx, times_2, 0));
        __ sarl(ACC);
    }
    
    void EmitBitwiseShr64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movl(rcx, Operand(rbp, rbx, times_2, 0));
        __ sarq(ACC);
    }
    
    void EmitBitwiseLogicShr32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movl(rcx, Operand(rbp, rbx, times_2, 0));
        __ shrl(ACC);
    }
    
    void EmitBitwiseLogicShr64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movl(rcx, Operand(rbp, rbx, times_2, 0));
        __ shrq(ACC);
    }
    
    void EmitIncrement32(MacroAssembler *masm) override {
        InstrStackImmBAScope instr_scope(masm);
        __ movl(SCRATCH, rbx);
        instr_scope.GetAToRBX();
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        __ addl(Operand(rbp, rbx, times_2, 0), SCRATCH);
    }
    
    void EmitIncrement64(MacroAssembler *masm) override {
        InstrStackImmBAScope instr_scope(masm);
        __ movq(SCRATCH, rbx);
        instr_scope.GetAToRBX();
        __ movq(ACC, Operand(rbp, rbx, times_2, 0));
        __ addq(Operand(rbp, rbx, times_2, 0), SCRATCH);
    }

    // Comparation ---------------------------------------------------------------------------------
    void EmitTestEqual32(MacroAssembler *masm) override { EmitCompare32(masm, Equal); }
    
    void EmitTestNotEqual32(MacroAssembler *masm) override { EmitCompare32(masm, NotEqual); }
    
    void EmitTestLessThan32(MacroAssembler *masm) override { EmitCompare32(masm, Less); }
    
    void EmitTestLessThanOrEqual32(MacroAssembler *masm) override {
        EmitCompare32(masm, LessEqual);
    }
    
    void EmitTestGreaterThan32(MacroAssembler *masm) override { EmitCompare32(masm, Greater); }
    
    void EmitTestGreaterThanOrEqual32(MacroAssembler *masm) override {
        EmitCompare32(masm, GreaterEqual);
    }
    
    void EmitTestEqual64(MacroAssembler *masm) override { EmitCompare64(masm, Equal); }
    
    void EmitTestNotEqual64(MacroAssembler *masm) override { EmitCompare64(masm, NotEqual); }
    
    void EmitTestLessThan64(MacroAssembler *masm) override { EmitCompare64(masm, Less); }
    
    void EmitTestLessThanOrEqual64(MacroAssembler *masm) override {
        EmitCompare64(masm, LessEqual);
    }
    
    void EmitTestGreaterThan64(MacroAssembler *masm) override { EmitCompare64(masm, Greater); }
    
    void EmitTestGreaterThanOrEqual64(MacroAssembler *masm) override {
        EmitCompare64(masm, GreaterEqual);
    }
    
    void EmitTestEqualf32(MacroAssembler *masm) override { EmitComparef32(masm, Equal); }
    
    void EmitTestNotEqualf32(MacroAssembler *masm) override { EmitComparef32(masm, NotEqual); }
    
    void EmitTestLessThanf32(MacroAssembler *masm) override { EmitComparef32(masm, Less); }
    
    void EmitTestLessThanOrEqualf32(MacroAssembler *masm) override {
        EmitComparef32(masm, LessEqual);
    }
    
    void EmitTestGreaterThanf32(MacroAssembler *masm) override { EmitComparef32(masm, Greater); }
    
    void EmitTestGreaterThanOrEqualf32(MacroAssembler *masm) override {
        EmitComparef32(masm, GreaterEqual);
    }
    
    void EmitTestEqualf64(MacroAssembler *masm) override { EmitComparef64(masm, Equal); }
    
    void EmitTestNotEqualf64(MacroAssembler *masm) override { EmitComparef64(masm, NotEqual); }
    
    void EmitTestLessThanf64(MacroAssembler *masm) override { EmitComparef64(masm, Less); }
    
    void EmitTestLessThanOrEqualf64(MacroAssembler *masm) override {
        EmitComparef64(masm, LessEqual);
    }
    
    void EmitTestGreaterThanf64(MacroAssembler *masm) override { EmitComparef64(masm, Greater); }
    
    void EmitTestGreaterThanOrEqualf64(MacroAssembler *masm) override {
        EmitComparef64(masm, GreaterEqual);
    }

    // Flow Control --------------------------------------------------------------------------------
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
        __ movl(rbx, Operand(SCRATCH, Function::kOffsetStackSize));
        __ addq(rsp, rbx); // Recover stack
        __ popq(rbp);
        __ ret(0);
    }

    void EmitYield(MacroAssembler *masm) override {
        InstrBaseScope instr_scope(masm);

        __ movl(rbx, Operand(BC, 0));
        __ andl(rbx, BytecodeNode::kAOfAMask);
        __ cmpl(rbx, YIELD_FORCE);
        Label force;
        __ j(Equal, &force, true/*is_far*/);
        __ cmpl(rbx, YIELD_PROPOSE);
        Label propose;
        __ j(Equal, &propose, true/*is_far*/);
        __ cmpl(rbx, YIELD_RANDOM);
        Label random;
        __ j(Equal, &random, true/*is_far*/);
        // Bad bytecode
        __ Abort("Incorrect yield control code.");
        
        // Propose:
        __ Bind(&propose);
        __ cmpl(Operand(CO, Coroutine::kOffsetYield), 0);
        __ j(Greater, &force, true/*is_far*/);
        Label done;
        __ jmp(&done, true/*is_far*/);

        // Random:
        __ Bind(&random);
        __ rdrand(rbx);
        __ andl(rbx, 0xfffffff0);
        __ cmpl(rbx, 0);
        __ j(Equal, &force, false/*is_far*/);
        __ jmp(&done, false/*is_far*/);

        // Save ACC, FACC, rsp, rbp, pc then jump to resume point
        __ Bind(&force);
        __ SaveState0(SCRATCH);
        __ movq(SCRATCH, Operand(CO, Coroutine::kOffsetSysPC)); // SCRATCH = suspend point
        __ jmp(SCRATCH); // Jump to suspend point

        // Done:
        __ Bind(&done);
    }
    
//    void EmitGoto(MacroAssembler *masm) override {
//        InstrImmABScope instr_scope(masm);
//        // TODO A of tracing
//        instr_scope.GetBTo(rbx);
//        __ movl(Operand(rbp, BytecodeStackFrame::kOffsetPC), rbx);
//        __ StartBC();
//    }
//    
//    void EmitGotoIfTrue(MacroAssembler *masm) override {
//        __ cmpl(ACC, 0);
//        Label done;
//        __ j(Equal, &done, false/*is_far*/);
//
//        InstrImmABScope instr_scope(masm);
//        // TODO A of tracing
//        instr_scope.GetBTo(rbx);
//        __ movl(Operand(rbp, BytecodeStackFrame::kOffsetPC), rbx);
//        __ StartBC();
//        
//        __ Bind(&done);
//    }
//    
//    void EmitGotoIfFalse(MacroAssembler *masm) override {
//        __ cmpl(ACC, 0);
//        Label done;
//        __ j(NotEqual, &done, false/*is_far*/);
//
//        InstrImmABScope instr_scope(masm);
//        // TODO A of tracing
//        instr_scope.GetBTo(rbx);
//        __ movl(Operand(rbp, BytecodeStackFrame::kOffsetPC), rbx);
//        __ StartBC();
//        
//        __ Bind(&done);
//    }
    
    void EmitBackwardJump(MacroAssembler *masm) override {
        InstrImmABScope instr_scope(masm);
        // TODO A of tracing
        instr_scope.GetBTo(rbx);
        __ subl(Operand(rbp, BytecodeStackFrame::kOffsetPC), rbx);
        __ StartBC();
    }

    void EmitForwardJump(MacroAssembler *masm) override {
        InstrImmABScope instr_scope(masm);
        instr_scope.GetBTo(rbx);
        __ addl(Operand(rbp, BytecodeStackFrame::kOffsetPC), rbx);
        __ StartBC();
    }
    
    void EmitBackwardJumpIfTrue(MacroAssembler *masm) override {
        __ cmpl(ACC, 0);
        Label done;
        __ j(Equal, &done, false/*is_far*/);

        InstrImmABScope instr_scope(masm);
        // TODO A of tracing
        instr_scope.GetBTo(rbx);
        __ subl(Operand(rbp, BytecodeStackFrame::kOffsetPC), rbx);
        __ StartBC();

        __ Bind(&done);
    }
    
    void EmitForwardJumpIfTrue(MacroAssembler *masm) override {
        __ cmpl(ACC, 0);
        Label done;
        __ j(Equal, &done, false/*is_far*/);

        InstrImmABScope instr_scope(masm);
        instr_scope.GetBTo(rbx);
        __ addl(Operand(rbp, BytecodeStackFrame::kOffsetPC), rbx);
        __ StartBC();

        __ Bind(&done);
    }
    
    void EmitBackwardJumpIfFalse(MacroAssembler *masm) override {
        __ cmpl(ACC, 0);
        Label done;
        __ j(NotEqual, &done, false/*is_far*/);

        InstrImmABScope instr_scope(masm);
        // TODO A of tracing
        instr_scope.GetBTo(rbx);
        __ subl(Operand(rbp, BytecodeStackFrame::kOffsetPC), rbx);
        __ StartBC();
        
        __ Bind(&done);
    }
    
    void EmitForwardJumpIfFalse(MacroAssembler *masm) override {
        __ cmpl(ACC, 0);
        Label done;
        __ j(NotEqual, &done, false/*is_far*/);

        InstrImmABScope instr_scope(masm);
        instr_scope.GetBTo(rbx);
        __ addl(Operand(rbp, BytecodeStackFrame::kOffsetPC), rbx);
        __ StartBC();

        __ Bind(&done);
    }

    // Calling -------------------------------------------------------------------------------------
    void EmitCallFunction(MacroAssembler *masm) override {
        __ Abort("TODO:");
    }
    
    void EmitCallNativeFunction(MacroAssembler *masm) override {
        InstrImmABScope instr_scope(masm);
        // TODO: slot
        
        // Adjust Caller Stack
        instr_scope.GetBTo(rbx);
        __ addl(rbx, 15); // ebx = ebx + 16 - 1
        __ andl(rbx, 0xfffffff0); // ebx &= -16
        __ subq(rsp, rbx); // Adjust sp to aligment of 16 bits(2 bytes)

        __ movq(SCRATCH, Operand(ACC, Closure::kOffsetCode));
        __ leaq(SCRATCH, Operand(SCRATCH, Code::kOffsetEntry));
        __ call(SCRATCH); // Call stub code
        // The stub code should switch to system stack and call real function.

        // Recover Caller Stack
        instr_scope.GetBTo(rbx);
        __ addl(rbx, 15); // ebx = ebx + 16 - 1
        __ andl(rbx, 0xfffffff0); // ebx &= -16
        __ addq(rsp, rbx); // Adjust sp to aligment of 16 bits(2 bytes)

        EmitCheckException(masm);
    }

    void EmitCallBytecodeFunction(MacroAssembler *masm) override {
        InstrImmABScope instr_scope(masm);
        // TODO: slot
    
        // Adjust Caller Stack
        instr_scope.GetBTo(rbx);
        __ addl(rbx, 15); // ebx = ebx + 16 - 1
        __ andl(rbx, 0xfffffff0); // ebx &= -16
        __ subq(rsp, rbx); // Adjust sp to aligment of 16 bits(2 bytes)

        __ movq(Argv_0, ACC);
        __ movq(SCRATCH, space_->interpreter_pump_code()->entry());
        __ call(SCRATCH);
        
        // Recover BC Register
        __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetBytecodeArray));
        // SCRATCH = &bytecodes->instructions
        __ addq(SCRATCH, BytecodeArray::kOffsetEntry);
        __ movl(rbx, Operand(rbp, BytecodeStackFrame::kOffsetPC));
        // XXX: Recover BC register first!
        __ leaq(BC, Operand(SCRATCH, rbx, times_4, 0)); // [SCRATCH + rbx * 4]

        // Recover Caller Stack
        instr_scope.GetBTo(rbx);
        __ addl(rbx, 15); // ebx = ebx + 16 - 1
        __ andl(rbx, 0xfffffff0); // ebx &= -16
        __ addq(rsp, rbx); // Adjust sp to aligment of 16 bits(2 bytes)
    }
    
    // Coroutine *RunCoroutine(uint32_t flags, Closure *entry_point, Address params,
    //                         uint32_t params_bytes_size)
    void EmitRunCoroutine(MacroAssembler *masm) override {
        InstrImmAFScope instr_scope(masm);
        // Adjust Caller Stack
        __ addl(rbx, 15); // ebx = ebx + 16 - 1
        __ andl(rbx, 0xfffffff0); // ebx &= -16
        __ movl(Argv_3, rbx); // 'uint32_t params_bytes_size'
        instr_scope.GetFTo(Argv_0); // 'uint32_t flags'
        //__ movl(Argv_0, rbx); // 'uint32_t flags'
        __ movq(Argv_1, ACC); // 'Closure *entry_point' ACC store enter point
        __ movq(Argv_2, rsp); // 'Address params'
        __ subq(Argv_2, Argv_3);

        // Switch and run RunCoroutine
        // Must inline call this function!
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::RunCoroutine));
        EmitCheckException(masm);
    }
    
    void EmitNewObject(MacroAssembler *masm) override {
        InstrImmABScope instr_scope(masm);
        // TODO: slot

        // Load class pointer
        instr_scope.GetBTo(rbx); // class
        __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetConstPool));
        __ movq(Argv_0, Operand(SCRATCH, rbx, times_4, 0));
        __ movq(Argv_1, 0);
    
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::NewObject));
        EmitCheckException(masm);
    }

    // Checking ------------------------------------------------------------------------------------
    void EmitCheckStack(MacroAssembler *masm) override {
        __ cmpq(rsp, Operand(CO, Coroutine::kOffsetStackGuard0));
        Label ok;
        __ j(GreaterEqual, &ok, false/*is_far*/);
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::NewStackoverflowPanic));
        __ Throw(SCRATCH, rbx);
        __ Bind(&ok);
        __ JumpNextBC();
    }
    
    void EmitAssertNotNull(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ cmpq(Operand(rbp, rbx, times_2, 0), 0);
        Label ok;
        __ LikelyJ(NotEqual, &ok, false/*is_far*/);
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::NewNilPointerPanic));
        __ Throw(SCRATCH, rbx);

        __ Bind(&ok);
    }
    
    // Others ---------------------------------------------------------------------------------------
    void EmitClose(MacroAssembler *masm) override {
        InstrImmAScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetConstPool));
        __ movq(Argv_0, Operand(SCRATCH, rbx, times_4, 0)); // func
        __ movl(Argv_1, 0); // flags
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::CloseFunction));
    }

    void EmitContact(MacroAssembler *masm) override {
        InstrImmABScope instr_scope(masm);
        // TODO: slot
        
        instr_scope.GetBTo(rbx);
        __ movq(Argv_0, rsp);
        __ subq(Argv_0, rbx);
        __ movq(Argv_1, rsp);
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::StringContact));
    }

    void EmitThrow(MacroAssembler *masm) override {
        InstrBaseScope instr_scope(masm);
        __ cmpq(ACC, 0);
        Label ok;
        __ j(NotEqual, &ok, false/*is_far*/);
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::NewNilPointerPanic));
        __ Throw(SCRATCH, rbx);

        __ Bind(&ok);
        __ movq(Argv_0, ACC);
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::MakeStacktrace));
        __ cmpq(ACC, 0);
        Label done;
        __ j(NotEqual, &done, false/*is_far*/);

        // TODO: OOM
        __ nop();

        __ Bind(&done);
        __ Throw(SCRATCH, rbx);
    }
private:
    void EmitCompare32(MacroAssembler *masm, Cond cond) {
        InstrStackABScope instr_scope(masm);
        //__ Breakpoint();
        __ movl(rax, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ cmpl(rax, Operand(rbp, rbx, times_2, 0));
        Label br_true;
        __ j(cond, &br_true, false/*is_far*/);
        __ xorq(rax, rax);
        Label done;
        __ jmp(&done, false/*is_far*/);
        __ Bind(&br_true);
        __ movq(rax, 1);
        __ Bind(&done);
    }

    void EmitCompare64(MacroAssembler *masm, Cond cond) {
        InstrStackABScope instr_scope(masm);
        __ movq(rax, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ cmpq(rax, Operand(rbp, rbx, times_2, 0));
        Label br_true;
        __ j(cond, &br_true, false/*is_far*/);
        __ xorq(rax, rax);
        Label done;
        __ jmp(&done, false/*is_far*/);
        __ Bind(&br_true);
        __ movq(rax, 1);
        __ Bind(&done);
    }
    
    void EmitComparef32(MacroAssembler *masm, Cond cond) {
        InstrStackABScope instr_scope(masm);
        __ movss(xmm0, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ comiss(xmm0, Operand(rbp, rbx, times_2, 0));
        Label br_true;
        __ j(cond, &br_true, false/*is_far*/);
        __ xorq(rax, rax);
        Label done;
        __ jmp(&done, false/*is_far*/);
        __ Bind(&br_true);
        __ movq(rax, 1);
        __ Bind(&done);
    }
    
    void EmitComparef64(MacroAssembler *masm, Cond cond) {
        InstrStackABScope instr_scope(masm);
        __ movsd(xmm0, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ comisd(xmm0, Operand(rbp, rbx, times_2, 0));
        Label br_true;
        __ j(cond, &br_true, false/*is_far*/);
        __ xorq(rax, rax);
        Label done;
        __ jmp(&done, false/*is_far*/);
        __ Bind(&br_true);
        __ movq(rax, 1);
        __ Bind(&done);
    }
    
    void EmitCheckException(MacroAssembler *masm) {
        // Test if throw exception in native function
        // Can not throw c++ exception in native function!
        __ cmpq(Operand(CO, Coroutine::kOffsetException), 0);
        Label done;
        __ j(Equal, &done, false/*is_far*/); // if (!co->exception) { goto done: }

        // Move native function throws exception to ACC
        __ movq(ACC, Operand(CO, Coroutine::kOffsetException));
        // Then throw in mai env:
        __ Throw(SCRATCH, rbx);

        __ Bind(&done);
    }
}; // class BytecodeEmitter


/*static*/ AbstractBytecodeEmitter *AbstractBytecodeEmitter::New(MetadataSpace *space) {
    return new BytecodeEmitter(space);
}

} // namespace lang

} // namespace mai
