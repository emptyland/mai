#include "lang/macro-assembler-x64.h"
#include "lang/stack-frame.h"
#include "lang/coroutine.h"
#include "lang/runtime.h"
#include "lang/isolate-inl.h"
#include "lang/pgo.h"
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

void MacroAssembler::SaveBytecodeEnv(bool enable_jit) {
    movq(Operand(CO, Coroutine::kOffsetBP1), rbp);
    movq(Operand(CO, Coroutine::kOffsetSP1), rsp);
    movq(rbp, Operand(CO, Coroutine::kOffsetSysBP)); // recover system bp
    movq(rsp, Operand(CO, Coroutine::kOffsetSysSP)); // recover system sp
    movl(Operand(CO, Coroutine::kOffsetState), Coroutine::kFallIn);

    pushq(SCRATCH);
    pushq(CO);
    pushq(BC);
    pushq(BC_ARRAY);
    if (enable_jit) {
        pushq(PROFILER);
        pushq(TRACER);
    }
}

void MacroAssembler::RecoverBytecodeEnv(bool enable_jit) {
    if (enable_jit) {
        popq(TRACER);
        popq(PROFILER);
    }
    popq(BC_ARRAY); // Switch back to mai stack
    popq(BC);
    popq(CO);
    popq(SCRATCH);

    movl(Operand(CO, Coroutine::kOffsetState), Coroutine::kRunning);
    movq(rbp, Operand(CO, Coroutine::kOffsetBP1)); // recover mai sp
    movq(rsp, Operand(CO, Coroutine::kOffsetSP1)); // recover mai bp
}

void MacroAssembler::SaveState0(Register scratch) {
//    movq(Operand(CO, Coroutine::kOffsetACC), ACC);
//    movsd(Operand(CO, Coroutine::kOffsetFACC), FACC);
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

    movq(Argv_0, reinterpret_cast<Address>(const_cast<char *>(message)));
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
void Generate_SwitchSystemStackCall(MacroAssembler *masm, bool enable_jit) {
    StackFrameScope frame_scope(masm, StubStackFrame::kSize);
    __ movq(Operand(rbp, StubStackFrame::kOffsetMaker), StubStackFrame::kMaker);
    __ movq(Operand(rbp, StubStackFrame::kOffsetCallee), 0);

    __ SaveBytecodeEnv(enable_jit);
    __ call(r10); // Call real function
    __ RecoverBytecodeEnv(enable_jit);
}

// Prototype: void Trampoline(Coroutine *co)
void Generate_Trampoline(MacroAssembler *masm, Address switch_call, Address pump, bool enable_jit,
                         int *suspend_point_pc) {
    StackFrameScope frame_scope(masm, TrampolineStackFrame::kSize);
    //==============================================================================================
    // NOTICE: The fucking clang++ optimizer should proecte: r12~r15 and rbx registers.
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
    if (enable_jit) {
        //__ Breakpoint();
        __ movq(PROFILER, Operand(SCRATCH, Isolate::kOffsetHotCountSlots));
    }
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
    __ movq(SCRATCH, Operand(CO, Coroutine::kOffsetCaught));
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
    __ xorq(ACC, ACC);
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
void Generate_InterpreterPump(MacroAssembler *masm, Address switch_call, bool enable_jit) {
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
    __ InlineSwitchSystemStackCall(arch::MethodAddress(&Function::DispatchException), enable_jit);
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

void Patch_Tracing(MacroAssembler *masm) {
    //__ Breakpoint();
    __ movq(rbx, Operand(TRACER, Tracer::kOffsetPathSize));
    __ cmpq(rbx, Operand(TRACER, Tracer::kOffsetLimitSize));
    Label abort;
    __ j(GreaterEqual, &abort, true/*is_far*/);
    __ cmpq(rbx, Operand(TRACER, Tracer::kOffsetPathCapacity));
    Label trace;
    __ j(Less, &trace, false/*is_far*/);
    
    // Resize path
    __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::GrowTracingPath), true);
    __ movq(TRACER, rax);
    __ movq(rbx, Operand(TRACER, Tracer::kOffsetPathSize));
    
    // Label: trace
    __ Bind(&trace);
    __ movq(SCRATCH, Operand(TRACER, Tracer::kOffsetPath));
    __ movl(rcx, Operand(rbp, BytecodeStackFrame::kOffsetPC)); // current PC
    __ shlq(rcx, 32);
    __ movl(rdx, Operand(BC, 0)); // current byte-code
    __ orq(rcx, rdx);
    __ movq(Operand(SCRATCH, rbx, times_8, 0), rcx);

    __ incq(Operand(TRACER, Tracer::kOffsetPathSize));
    Label exit;
    __ jmp(&exit, false/*is_far*/);
    
    // Label: abort
    __ Bind(&abort);
    // Address *FinalizeTracing(int **slots)
    __ subq(rsp, kStackAligmentSize);
    __ leaq(Argv_0, Operand(rsp, 0));
    __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::AbortTracing), true);
    __ movq(BC_ARRAY, rax);
    __ movq(PROFILER, Operand(rsp, 0));
    __ addq(rsp, kStackAligmentSize);

    __ Bind(&exit);
    __ nop();
    __ nop();
    __ nop();
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
    
    BytecodeEmitter(MetadataSpace *space, bool generated_debug_code, bool generated_profiling_code)
        : PartialBytecodeEmitter(DCHECK_NOTNULL(space))
        , enable_debug(generated_debug_code)
        , enable_jit(generated_profiling_code) {}
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
        CheckNotNil(masm, SCRATCH);
        instr_scope.GetBToRBX();
        __ xorq(ACC, ACC);
        __ movb(ACC, Operand(SCRATCH, rbx, times_1, 0));
    }
    
    void EmitLdaProperty16(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        instr_scope.GetBToRBX();
        __ xorq(ACC, ACC);
        __ movw(ACC, Operand(SCRATCH, rbx, times_1, 0));
    }

    void EmitLdaProperty32(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        instr_scope.GetBToRBX();
        __ movl(ACC, Operand(SCRATCH, rbx, times_1, 0));
    }
    
    void EmitLdaProperty64(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        //__ Breakpoint();
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        instr_scope.GetBToRBX();
        __ movq(ACC, Operand(SCRATCH, rbx, times_1, 0));
    }
    
    void EmitLdaPropertyPtr(MacroAssembler *masm) override { EmitLdaProperty64(masm); }
    
    void EmitLdaPropertyf32(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        instr_scope.GetBToRBX();
        __ movss(FACC, Operand(SCRATCH, rbx, times_1, 0));
    }

    void EmitLdaPropertyf64(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        instr_scope.GetBToRBX();
        __ movsd(FACC, Operand(SCRATCH, rbx, times_1, 0));
    }

    void EmitLdaArrayAt8(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        instr_scope.GetBToRBX();
        __ xorq(rdx, rdx);
        __ movl(rdx, Operand(rbp, rbx, times_2, 0));
        CheckArrayBound<int32_t>(masm, SCRATCH, rdx);
        __ xorl(ACC, ACC);
        __ movb(ACC, Operand(SCRATCH, rdx, times_1, Array<int8_t>::kOffsetElems));
    }
   
    void EmitLdaArrayAt16(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        instr_scope.GetBToRBX();
        __ xorq(rdx, rdx);
        __ movl(rdx, Operand(rbp, rbx, times_2, 0));
        CheckArrayBound<int32_t>(masm, SCRATCH, rdx);
        __ xorl(ACC, ACC);
        __ movw(ACC, Operand(SCRATCH, rdx, times_2, Array<int16_t>::kOffsetElems));
    }

    void EmitLdaArrayAt32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        instr_scope.GetBToRBX();
        __ xorq(rax, rax);
        __ movl(rax, Operand(rbp, rbx, times_2, 0));
        CheckArrayBound<int32_t>(masm, SCRATCH, rax);
        __ movl(ACC, Operand(SCRATCH, rax, times_4, Array<int32_t>::kOffsetElems));
    }

    void EmitLdaArrayAt64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        instr_scope.GetBToRBX();
        __ xorq(rax, rax);
        __ movl(rax, Operand(rbp, rbx, times_2, 0));
        CheckArrayBound<int64_t>(masm, SCRATCH, rax);
        __ movq(ACC, Operand(SCRATCH, rax, times_8, Array<int64_t>::kOffsetElems));
    }

    void EmitLdaArrayAtPtr(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        instr_scope.GetBToRBX();
        __ xorq(rax, rax);
        __ movl(rax, Operand(rbp, rbx, times_2, 0));
        CheckArrayBound<Any *>(masm, SCRATCH, rax);
        __ movq(ACC, Operand(SCRATCH, rax, times_ptr_size, Array<Any *>::kOffsetElems));
    }

    void EmitLdaArrayAtf32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        instr_scope.GetBToRBX();
        __ xorq(rax, rax);
        __ movl(rax, Operand(rbp, rbx, times_2, 0));
        CheckArrayBound<float>(masm, SCRATCH, rax);
        __ movss(FACC, Operand(SCRATCH, rax, times_4, Array<float>::kOffsetElems));
    }

    void EmitLdaArrayAtf64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        instr_scope.GetBToRBX();
        __ xorq(rax, rax);
        __ movl(rax, Operand(rbp, rbx, times_2, 0));
        CheckArrayBound<double>(masm, SCRATCH, rax);
        __ movsd(FACC, Operand(SCRATCH, rax, times_8, Array<double>::kOffsetElems));
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
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::WriteBarrierWithAddress),
                                       enable_jit);

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
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::WriteBarrierWithOffset), enable_jit);

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

    void EmitStaArrayAt8(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        instr_scope.GetBToRBX();
        __ xorq(rdx, rdx);
        __ movl(rdx, Operand(rbp, rbx, times_2, 0));
        CheckArrayBound<int32_t>(masm, SCRATCH, rdx);
        __ movb(Operand(SCRATCH, rdx, times_1, Array<int8_t>::kOffsetElems), ACC);
    }
    
    void EmitStaArrayAt16(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        instr_scope.GetBToRBX();
        __ xorq(rdx, rdx);
        __ movl(rdx, Operand(rbp, rbx, times_2, 0));
        CheckArrayBound<int32_t>(masm, SCRATCH, rdx);
        __ movw(Operand(SCRATCH, rdx, times_2, Array<int16_t>::kOffsetElems), ACC);
    }
    
    void EmitStaArrayAt32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        instr_scope.GetBToRBX();
        __ xorq(rdx, rdx);
        __ movl(rdx, Operand(rbp, rbx, times_2, 0));
        CheckArrayBound<int32_t>(masm, SCRATCH, rdx);
        __ movl(Operand(SCRATCH, rdx, times_4, Array<int32_t>::kOffsetElems), ACC);
    }
    
    void EmitStaArrayAt64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        instr_scope.GetBToRBX();
        __ xorq(rdx, rdx);
        __ movl(rdx, Operand(rbp, rbx, times_2, 0));
        CheckArrayBound<int64_t>(masm, SCRATCH, rdx);
        __ movq(Operand(SCRATCH, rdx, times_8, Array<int64_t>::kOffsetElems), ACC);
    }
    
    void EmitStaArrayAtPtr(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        instr_scope.GetBToRBX();
        __ xorq(rdx, rdx);
        __ movl(rdx, Operand(rbp, rbx, times_2, 0));
        CheckArrayBound<Any *>(masm, SCRATCH, rdx);
        __ movq(Operand(SCRATCH, rdx, times_ptr_size, Array<Any *>::kOffsetElems), ACC);
    }
    
    void EmitStaArrayAtf32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        instr_scope.GetBToRBX();
        __ xorq(rdx, rdx);
        __ movl(rdx, Operand(rbp, rbx, times_2, 0));
        CheckArrayBound<float>(masm, SCRATCH, rdx);
        __ movss(Operand(SCRATCH, rdx, times_4, Array<float>::kOffsetElems), FACC);
    }

    void EmitStaArrayAtf64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        instr_scope.GetBToRBX();
        __ xorq(rdx, rdx);
        __ movl(rdx, Operand(rbp, rbx, times_2, 0));
        CheckArrayBound<double>(masm, SCRATCH, rdx);
        __ movsd(Operand(SCRATCH, rdx, times_8, Array<double>::kOffsetElems), FACC);
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
        //__ Breakpoint();
        __ movq(rax, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetAToRBX();
        __ movq(Operand(rbp, rbx, times_2, 0), rax);
    }

    void EmitMovePtr(MacroAssembler *masm) override { EmitMove64(masm); }
    
    // Binary Operation ----------------------------------------------------------------------------
    void EmitAdd8(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ addb(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitAdd16(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        //__ Breakpoint();
        __ addw(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitAdd32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        //__ Breakpoint();
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
    
    void EmitSub8(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ subb(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitSub16(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ subw(ACC, Operand(rbp, rbx, times_2, 0));
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
    
    void EmitMul8(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        // rax:rdx <- rax * operand
        __ mulb(Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitMul16(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        // rax:rdx <- rax * operand
        __ mulw(Operand(rbp, rbx, times_2, 0));
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
    
    void EmitIMul8(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ imulb(Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitIMul16(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ imulw(ACC, Operand(rbp, rbx, times_2, 0));
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
    
    void EmitDiv8(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        
        instr_scope.GetBToRBX();
        CheckArithmetic(masm, rbx, 4/*size*/);
        // al <- ax / operand
        // ah <- remainder
        __ divb(Operand(rbp, rbx, times_2, 0));
        __ andl(ACC, 0xff);
    }
    
    void EmitDiv16(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));

        instr_scope.GetBToRBX();
        CheckArithmetic(masm, rbx, 4/*size*/);
    
        // ax:dx <- ax / operand
        __ xorl(rdx, rdx);
        __ divw(Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitDiv32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        __ xorl(rdx, rdx);

        instr_scope.GetBToRBX();
        CheckArithmetic(masm, rbx, 4/*size*/);
        // rax:rdx <- rax / operand
        __ divl(Operand(rbp, rbx, times_2, 0));
    }

    void EmitDiv64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(ACC, Operand(rbp, rbx, times_2, 0));
        __ xorq(rdx, rdx);
        
        instr_scope.GetBToRBX();
        CheckArithmetic(masm, rbx, 8/*size*/);
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
    
    void EmitIDiv8(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));

        instr_scope.GetBToRBX();
        CheckArithmetic(masm, rbx, 4/*size*/);

        __ idivb(Operand(rbp, rbx, times_2, 0));
        __ andl(ACC, 0xff);
    }
    
    void EmitIDiv16(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));

        instr_scope.GetBToRBX();
        CheckArithmetic(masm, rbx, 4/*size*/);

        __ xorl(rdx, rdx);
        __ idivw(Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitIDiv32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));

        instr_scope.GetBToRBX();
        CheckArithmetic(masm, rbx, 4/*size*/);

        __ xorl(rdx, rdx);
        __ idivl(Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitIDiv64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(ACC, Operand(rbp, rbx, times_2, 0));

        instr_scope.GetBToRBX();
        CheckArithmetic(masm, rbx, 8/*size*/);
        
        __ xorq(rdx, rdx);
        __ idivq(Operand(rbp, rbx, times_2, 0));
    }

    void EmitMod8(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));

        instr_scope.GetBToRBX();
        CheckArithmetic(masm, rbx, 4/*size*/);

        // al <- ax / operand
        // ah <- remainder
        __ divb(Operand(rbp, rbx, times_2, 0));
        __ shrl(ACC, 8);
        __ andl(ACC, 0xff);
    }

    void EmitMod16(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));

        instr_scope.GetBToRBX();
        CheckArithmetic(masm, rbx, 4/*size*/);

        // ax:dx <- ax * operand
        __ xorl(rdx, rdx);
        __ divw(Operand(rbp, rbx, times_2, 0));
        __ movl(ACC, rdx);
    }
    
    void EmitMod32(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));

        instr_scope.GetBToRBX();
        CheckArithmetic(masm, rbx, 4/*size*/);

        // rax:rdx <- rax * operand
        __ divl(Operand(rbp, rbx, times_2, 0));
        __ movl(ACC, rdx);
    }
    
    void EmitMod64(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movq(ACC, Operand(rbp, rbx, times_2, 0));
        
        instr_scope.GetBToRBX();
        CheckArithmetic(masm, rbx, 8/*size*/);

        // rax:rdx <- rax * operand
        __ divq(Operand(rbp, rbx, times_2, 0));
        __ movq(ACC, rdx);
    }
    
    void EmitNot(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ cmpl(Operand(rbp, rbx, times_2, 0), 0);
        __ set(Equal, ACC);
        __ andl(ACC, 0xff);
    }
    
    void EmitUMinus8(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        __ negb(ACC);
    }
    
    void EmitUMinus16(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        __ negw(ACC);
    }
    
    void EmitUMinus32(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        __ negl(ACC);
    }
    
    void EmitUMinus64(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movq(ACC, Operand(rbp, rbx, times_2, 0));
        __ negq(ACC);
    }
    
    void EmitBitwiseNot8(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        __ notb(ACC);
    }
    
    void EmitBitwiseNot16(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        __ notw(ACC);
    }

    void EmitBitwiseNot32(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        __ notl(ACC);
    }

    void EmitBitwiseNot64(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movq(ACC, Operand(rbp, rbx, times_2, 0));
        __ notq(ACC);
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
    
    void EmitBitwiseShl8(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movl(rcx, Operand(rbp, rbx, times_2, 0));
        __ shlb(ACC);
    }
    
    void EmitBitwiseShl16(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movl(rcx, Operand(rbp, rbx, times_2, 0));
        __ shlw(ACC);
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
    
    void EmitBitwiseShr8(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movl(rcx, Operand(rbp, rbx, times_2, 0));
        __ sarb(ACC);
    }
    
    void EmitBitwiseShr16(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movl(rcx, Operand(rbp, rbx, times_2, 0));
        __ sarw(ACC);
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

    void EmitBitwiseLogicShr8(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movl(rcx, Operand(rbp, rbx, times_2, 0));
        __ shrb(ACC);
    }
    
    void EmitBitwiseLogicShr16(MacroAssembler *masm) override {
        InstrStackABScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movl(rcx, Operand(rbp, rbx, times_2, 0));
        __ shrw(ACC);
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
    void EmitTestLessThan8(MacroAssembler *masm) override { EmitCompare8(masm, Less); }
    
    void EmitTestLessThanOrEqual8(MacroAssembler *masm) override { EmitCompare8(masm, LessEqual); }
    
    void EmitTestGreaterThan8(MacroAssembler *masm) override { EmitCompare8(masm, Greater); }
    
    void EmitTestGreaterThanOrEqual8(MacroAssembler *masm) override {
        EmitCompare8(masm, GreaterEqual);
    }
    
    void EmitTestLessThan16(MacroAssembler *masm) override { EmitCompare16(masm, Less); }
    
    void EmitTestLessThanOrEqual16(MacroAssembler *masm) override {
        EmitCompare16(masm, LessEqual);
    }
    
    void EmitTestGreaterThan16(MacroAssembler *masm) override { EmitCompare16(masm, Greater); }
    
    void EmitTestGreaterThanOrEqual16(MacroAssembler *masm) override {
        EmitCompare16(masm, GreaterEqual);
    }

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
    
    void EmitTestPtrEqual(MacroAssembler *masm) override { EmitCompare64(masm, Equal); }
    
    void EmitTestPtrNotEqual(MacroAssembler *masm) override { EmitCompare64(masm, NotEqual); }
    
    void EmitTestStringEqual(MacroAssembler *masm) override {
        //EmitTestStringEqualOrNot(masm, false/*inv*/);
        EmitCompareImplicitLengthString(masm, Equal);
    }
    
    void EmitTestStringNotEqual(MacroAssembler *masm) override {
        //EmitTestStringEqualOrNot(masm, true/*inv*/);
        EmitCompareImplicitLengthString(masm, NotEqual);
    }
    
    void EmitTestStringLessThan(MacroAssembler *masm) override {
        EmitCompareImplicitLengthString(masm, Less);
    }
    
    void EmitTestStringLessThanOrEqual(MacroAssembler *masm) override {
        EmitCompareImplicitLengthString(masm, LessEqual);
    }
    
    void EmitTestStringGreaterThan(MacroAssembler *masm) override {
        EmitCompareImplicitLengthString(masm, Greater);
    }
    
    void EmitTestStringGreaterThanOrEqual(MacroAssembler *masm) override {
        EmitCompareImplicitLengthString(masm, GreaterEqual);
    }
    
    void EmitTestAs(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        __ movq(ACC, Operand(SCRATCH, Any::kOffsetKlass));
        __ andq(ACC, ~1);

        instr_scope.GetBToRBX();
        __ movq(Argv_0, SCRATCH);
        __ movq(rdx, Operand(rbp, BytecodeStackFrame::kOffsetConstPool));
        __ movq(Argv_1, Operand(rdx, rbx, times_4, 0));
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::IsSameOrBaseOf), enable_jit);
        __ cmpl(rax, 0);
        Label ok;
        __ j(NotEqual, &ok, false/*is_far*/);
        __ movq(ACC, Operand(SCRATCH, Any::kOffsetKlass));
        __ andq(ACC, ~1);
        __ movq(Argv_0, ACC);
        __ movq(rdx, Operand(rbp, BytecodeStackFrame::kOffsetConstPool));
        __ movq(Argv_1, Operand(rdx, rbx, times_4, 0));
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::NewBadCastPanic), enable_jit);
        __ Throw(SCRATCH, rbx);

        __ Bind(&ok);
        __ movq(ACC, SCRATCH);
    }
    
    void EmitTestIs(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, rbx, times_2, 0));
        CheckNotNil(masm, SCRATCH);
        __ movq(ACC, Operand(SCRATCH, Any::kOffsetKlass));
        __ andq(ACC, ~1);

        instr_scope.GetBToRBX();
        __ movq(Argv_0, SCRATCH);
        __ movq(rdx, Operand(rbp, BytecodeStackFrame::kOffsetConstPool));
        __ movq(Argv_1, Operand(rdx, rbx, times_4, 0));
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::IsSameOrBaseOf), enable_jit);
    }
    
    // Casting -------------------------------------------------------------------------------------
    void EmitTruncate32To8(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        __ andl(ACC, 0xff);
    }
    
    void EmitTruncate32To16(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        __ andl(ACC, 0xffff);
    }
    
    void EmitTruncate64To32(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitZeroExtend8To32(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ xorl(ACC, ACC);
        __ movzxb(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitZeroExtend16To32(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ xorl(ACC, ACC);
        __ movzxw(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitZeroExtend32To64(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ xorq(ACC, ACC);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitSignExtend8To16(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movsxb(ACC, Operand(rbp, rbx, times_2, 0));
        __ andl(ACC, 0xffff);
    }
    
    void EmitSignExtend8To32(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movsxb(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitSignExtend16To32(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movsxw(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitSignExtend32To64(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ movsxd(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitF32ToI32(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ cvtss2sil(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitF64ToI32(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ cvtsd2sil(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitF32ToU32(MacroAssembler *masm) override { EmitF32ToI32(masm); }
    
    void EmitF64ToU32(MacroAssembler *masm) override { EmitF64ToI32(masm); }
    
    void EmitF32ToI64(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ cvtss2siq(ACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitF64ToI64(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ cvtsd2siq(ACC, Operand(rbp, rbx, times_2, 0));
    }

    void EmitF32ToU64(MacroAssembler *masm) override { EmitF32ToI64(masm); }
    
    void EmitF64ToU64(MacroAssembler *masm) override { EmitF64ToI64(masm); }
    
    void EmitI32ToF32(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ cvtsi2ssl(FACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitU32ToF32(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        // Extend to 64 bits
        __ xorq(ACC, ACC);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        __ cvtsi2ssq(FACC, ACC);
    }
    
    void EmitI64ToF32(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ cvtsi2ssq(FACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitU64ToF32(MacroAssembler *masm) override { EmitI64ToF32(masm); }
    
    void EmitI32ToF64(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ cvtsi2sdl(FACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitU32ToF64(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        // Extend to 64 bits
        __ xorq(ACC, ACC);
        __ movl(ACC, Operand(rbp, rbx, times_2, 0));
        __ cvtsi2sdq(FACC, ACC);
    }
    
    void EmitI64ToF64(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ cvtsi2sdq(FACC, Operand(rbp, rbx, times_2, 0));
    }
    
    void EmitU64ToF64(MacroAssembler *masm) override { EmitI64ToF64(masm); }
    
    void EmitF32ToF64(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ cvtss2sd(FACC, Operand(rbp, rbx, times_2, 0));
    }

    void EmitF64ToF32(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ cvtsd2ss(FACC, Operand(rbp, rbx, times_2, 0));
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
    
    void EmitBackwardJump(MacroAssembler *masm) override {
        InstrImmABScope instr_scope(masm);
        // Profiling
        if (enable_jit) {
            __ PrefixLock();
            __ decl(Operand(PROFILER, rbx, times_4, 0));
            Label try_tracing;
            __ j(Negative, &try_tracing, false/*is_far*/);
            Label exit;
            __ jmp(&exit, false/*is_far*/);
            
            // Label: try tracing
            __ Bind(&try_tracing);
            
            // Address *TryTracing(Function *fun, int32_t slot, int32_t pc, Tracer **)
            __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetCallee));
            __ movq(Argv_0, Operand(SCRATCH, Closure::kOffsetProto));
            __ movl(Argv_1, rbx);
            __ movl(Argv_2, Operand(rbp, BytecodeStackFrame::kOffsetPC));
            __ subq(rsp, kStackAligmentSize);
            __ leaq(Argv_3, Operand(rsp, 0));
            __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::TryTracing), enable_jit);
            __ movq(BC_ARRAY, rax);
            __ movq(TRACER, Operand(rsp, 0));
            __ addq(rsp, kStackAligmentSize);
            // Label: exit
            __ Bind(&exit);
        }

        instr_scope.GetBTo(rbx);
        __ subl(Operand(rbp, BytecodeStackFrame::kOffsetPC), rbx);
        __ StartBC();
    }
    
    static void TracingBackwardJump(MacroAssembler *masm) {
        InstrImmABScope instr_scope(masm);
        __ cmpl(rbx, Operand(TRACER, Tracer::kOffsetGuardSlot));
        Label exit;
        __ j(NotEqual, &exit, true/*is_far*/);
        
        __ decl(Operand(TRACER, Tracer::kOffsetRepeatedCount));
        Label finalize;
        __ j(Negative, &finalize, false/*is_far*/);
        // void RepeatTracing()
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::RepeatTracing), true);
        __ jmp(&exit, true/*is_far*/);
        
        __ Bind(&finalize);
        __ subq(rsp, kStackAligmentSize);
        __ leaq(Argv_0, Operand(rsp, 0));
        // Address *FinalizeTracing(int **slots)
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::FinalizeTracing), true);
        __ movq(BC_ARRAY, rax);
        __ movq(PROFILER, Operand(rsp, 0));
        __ addq(rsp, kStackAligmentSize);
        
        // Label: Exit
        __ Bind(&exit);
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

        __ movq(Argv_0, ACC);
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
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::RunCoroutine), enable_jit);
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
    
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::NewObject), enable_jit);
        EmitCheckException(masm);
    }
    
    void EmitArray(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movl(Argv_1, Operand(rbp, rbx, times_2, 0));

        __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetConstPool));
        instr_scope.GetBTo(rbx);
        __ movq(Argv_0, Operand(SCRATCH, rbx, times_4, 0));
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::NewArray), enable_jit);
        EmitCheckException(masm);
    }
    
    void EmitArrayWith(MacroAssembler *masm) override {
        InstrImmABScope instr_scope(masm);
        __ xorq(ACC, ACC);
        __ movl(ACC, rbx); // Save to ACC

        __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetConstPool));
        instr_scope.GetBTo(rbx);
        __ movq(Argv_0, Operand(SCRATCH, rbx, times_4, 0));
        __ movq(Argv_1, rsp);
        __ subq(Argv_1, ACC);
        __ movq(Argv_2, rsp);

        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::NewArrayWith), enable_jit);
        EmitCheckException(masm);
    }
    
    void EmitPutAll(MacroAssembler *masm) override {
        InstrStackImmABScope instr_scope(masm);
        __ movq(Argv_0, Operand(rbp, rbx, times_2, 0));
        
        instr_scope.GetBToRBX();
        __ movq(Argv_1, rsp);
        __ subq(Argv_1, rbx);
        __ movq(Argv_2, rsp);
        
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::MapPutAll), enable_jit);
        EmitCheckException(masm);
    }
    
    void EmitRandom(MacroAssembler *masm) override {
        InstrBaseScope instr_scope(masm);
        __ rdrand(ACC);
    }

    // Checking ------------------------------------------------------------------------------------
    void EmitCheckStack(MacroAssembler *masm) override {
        __ cmpq(rsp, Operand(CO, Coroutine::kOffsetStackGuard0));
        Label ok;
        __ j(GreaterEqual, &ok, false/*is_far*/);
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::NewStackoverflowPanic), enable_jit);
        __ Throw(SCRATCH, rbx);
        __ Bind(&ok);
        __ JumpNextBC();
    }
    
    
    static void TracingCheckStack(MacroAssembler *masm) {
        __ cmpq(rsp, Operand(CO, Coroutine::kOffsetStackGuard0));
        Label ok;
        __ j(GreaterEqual, &ok, false/*is_far*/);
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::NewStackoverflowPanic), true);
        __ Throw(SCRATCH, rbx);
        __ Bind(&ok);

        // void Runtime::TraceInvoke(Function *fun, int32_t slot)
        __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetCallee));
        __ movq(Argv_0, Operand(SCRATCH, Closure::kOffsetProto));
        __ movl(Argv_1, 0);
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::TraceInvoke), true);
        
        __ JumpNextBC();
    }
    
    void EmitAssertNotNull(MacroAssembler *masm) override {
        InstrStackAScope instr_scope(masm);
        __ cmpq(Operand(rbp, rbx, times_2, 0), 0);
        Label ok;
        __ LikelyJ(NotEqual, &ok, false/*is_far*/);
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::NewNilPointerPanic), enable_jit);
        __ Throw(SCRATCH, rbx);

        __ Bind(&ok);
    }
    
    // Others ---------------------------------------------------------------------------------------
    void EmitClose(MacroAssembler *masm) override {
        InstrImmAScope instr_scope(masm);
        __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetConstPool));
        __ movq(Argv_0, Operand(SCRATCH, rbx, times_4, 0)); // func
        __ movl(Argv_1, 0); // flags
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::CloseFunction), enable_jit);
    }

    void EmitContact(MacroAssembler *masm) override {
        InstrImmABScope instr_scope(masm);
        // TODO: slot
        
        instr_scope.GetBTo(rbx);
        __ movq(Argv_0, rsp);
        __ subq(Argv_0, rbx);
        __ movq(Argv_1, rsp);
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::StringContact), enable_jit);
    }

    void EmitThrow(MacroAssembler *masm) override {
        InstrBaseScope instr_scope(masm);
        __ cmpq(ACC, 0);
        Label ok;
        __ j(NotEqual, &ok, false/*is_far*/);
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::NewNilPointerPanic), enable_jit);
        __ Throw(SCRATCH, rbx);

        __ Bind(&ok);
        __ movq(Argv_0, ACC);
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::MakeStacktrace), enable_jit);
        __ cmpq(ACC, 0);
        Label done;
        __ j(NotEqual, &done, false/*is_far*/);

        // TODO: OOM
        __ nop();

        __ Bind(&done);
        __ Throw(SCRATCH, rbx);
    }
private:
    void EmitTestStringEqualOrNot(MacroAssembler *masm, bool inv) {
        InstrStackABScope instr_scope(masm);
        __ movq(rsi, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movq(rdi, Operand(rbp, rbx, times_2, 0));

        Label br_equal;
        __ movl(rax, Operand(rsi, String::kOffsetLength));
        __ cmpl(rdx, Operand(rdi, String::kOffsetLength));
        __ cmovl(Less, rbx, rdx);
        __ cmovl(GreaterEqual, rbx, Operand(rsi, String::kOffsetLength)); // Get min length to rcx

        __ movq(r8, rbx); // save min length to r8
        __ andl(rbx, 0xfffffff0);
        
        __ movl(rax, 16); // string.1 len
        __ movl(rdx, 16); // string.2 len
        Label loop;
        __ Bind(&loop);
        Label remain;
        __ subl(rbx, 16);
        __ j(Carry, &remain, false/*is_far*/);
        __ movdqu(xmm0, Operand(rsi, rbx, times_1, String::kOffsetElems));
        __ pcmpestri(xmm0, Operand(rdi, rbx, times_1, String::kOffsetElems),
                     PCMP::EqualEach|PCMP::NegativePolarity);
        Label br_not_equal;
        __ j(Carry, &br_not_equal, false/*is_far*/);
        __ jmp(&loop, false/*is_far*/);

        __ Bind(&remain);
        __ movl(rbx, r8); // len
        __ andl(rbx, 0xf);
        __ movl(rax, rbx); // string.1 len
        __ movl(rdx, rbx); // string.2 len

        __ movl(rbx, r8);
        __ andl(rbx, 0xfffffff0);
        __ movdqu(xmm0, Operand(rsi, rbx, times_1, String::kOffsetElems));
        __ pcmpestri(xmm0, Operand(rdi, rbx, times_1, String::kOffsetElems),
                     PCMP::EqualEach|PCMP::NegativePolarity);
        __ j(Carry, &br_not_equal, false/*is_far*/);
        
        __ Bind(&br_equal);
        if (inv) {
            __ xorq(ACC, ACC);
        } else {
            __ movq(ACC, 1);
        }
        Label done;
        __ jmp(&done, false/*is_far*/);
        
        __ Bind(&br_not_equal);
        if (inv) {
            __ movq(ACC, 1);
        } else {
            __ xorq(ACC, ACC);
        }
        __ Bind(&done);
    }
    
    void EmitCompareImplicitLengthString(MacroAssembler *masm, Cond cond) {
        InstrStackABScope instr_scope(masm);
        __ movq(rsi, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movq(rdi, Operand(rbp, rbx, times_2, 0));
        
        __ leaq(rax, Operand(rsi, String::kOffsetElems));
        __ leaq(rdx, Operand(rdi, String::kOffsetElems));
        __ subq(rax, rdx);
        __ subq(rdx, 16);
        
        Label loop;
        __ Bind(&loop);
        __ addq(rdx, 16);
        __ movdqu(xmm0, Operand(rdx, 0));
        __ pcmpistri(xmm0, Operand(rdx, rax, times_1, 0), PCMP::EqualEach|PCMP::NegativePolarity);
        __ j(Above, &loop, false/*is_far*/);
        Label diff;
        __ j(Carry, &diff, false/*is_far*/);
        __ xorq(rbx, rbx);
        Label done;
        __ jmp(&done, false/*is_far*/);

        __ Bind(&diff);
        __ addq(rax, rdx);
        __ movzxb(rbx, Operand(rax, rcx, times_1, 0));
        __ movzxb(rdx, Operand(rdx, rcx, times_1, 0));
        __ subl(rbx, rdx);

        __ Bind(&done);
        __ xorl(ACC, ACC);
        __ cmpl(rbx, 0);
        __ set(cond, ACC);
    }
    
    void EmitCompare8(MacroAssembler *masm, Cond cond) {
        InstrStackABScope instr_scope(masm);
        __ movl(rax, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ cmpb(rax, Operand(rbp, rbx, times_2, 0));
        __ set(cond, ACC);
        __ andl(ACC, 0xff);
    }

    void EmitCompare16(MacroAssembler *masm, Cond cond) {
        InstrStackABScope instr_scope(masm);
        __ movl(rax, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ cmpw(rax, Operand(rbp, rbx, times_2, 0));
        __ set(cond, ACC);
        __ andl(ACC, 0xff);
    }

    void EmitCompare32(MacroAssembler *masm, Cond cond) {
        InstrStackABScope instr_scope(masm);
        __ movl(rax, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ cmpl(rax, Operand(rbp, rbx, times_2, 0));
        __ set(cond, ACC);
        __ andl(ACC, 0xff);
    }

    void EmitCompare64(MacroAssembler *masm, Cond cond) {
        InstrStackABScope instr_scope(masm);
        __ movq(rax, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ cmpq(rax, Operand(rbp, rbx, times_2, 0));
        __ set(cond, ACC);
        __ andl(ACC, 0xff);
    }
    
    /*
     UNORDERED: ZF,PF,CF111; GREATER_THAN: ZF,PF,CF000; LESS_THAN: ZF,PF,CF001; EQUAL: ZF,PF,CF100;
     */
    void EmitComparef32(MacroAssembler *masm, Cond cond) {
        InstrStackABScope instr_scope(masm);
        __ movss(xmm0, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movss(xmm1, Operand(rbp, rbx, times_2, 0));
        // <= jbe
        // < jb
        // != jne jp
        switch (cond) {
            case x64::Equal: {
                Label ok;
                __ ucomiss(xmm0, xmm1);
                __ j(NotEqual, &ok, false/*is_far*/);
                __ j(ParityEven, &ok, false/*is_far*/);
                __ movl(ACC, 1);
                Label done;
                __ jmp(&done, false/*is_far*/);
                __ Bind(&ok);
                __ xorq(ACC, ACC);
                __ Bind(&done);
            } break;
            case x64::NotEqual: {
                Label ok;
                __ ucomiss(xmm0, xmm1);
                __ j(NotEqual, &ok, false/*is_far*/);
                __ j(ParityEven, &ok, false/*is_far*/);
                __ xorq(ACC, ACC);
                Label done;
                __ jmp(&done, false/*is_far*/);
                __ Bind(&ok);
                __ movl(ACC, 1);
                __ Bind(&done);
            } break;
            case x64::Less:
                __ ucomiss(xmm0, xmm1);
                __ set(Below, ACC);
                __ andl(ACC, 0xff);
                break;
            case x64::LessEqual:
                __ ucomiss(xmm0, xmm1);
                __ set(BelowEqual, ACC);
                __ andl(ACC, 0xff);
                break;
            case x64::Greater:
                __ ucomiss(xmm1, xmm0);
                __ set(Below, ACC);
                __ andl(ACC, 0xff);
                break;
            case x64::GreaterEqual:
                __ ucomiss(xmm1, xmm0);
                __ set(BelowEqual, ACC);
                __ andl(ACC, 0xff);
                break;
            default:
                NOREACHED();
                break;
        }
    }
    
    void EmitComparef64(MacroAssembler *masm, Cond cond) {
        InstrStackABScope instr_scope(masm);
        __ movsd(xmm0, Operand(rbp, rbx, times_2, 0));
        instr_scope.GetBToRBX();
        __ movsd(xmm1, Operand(rbp, rbx, times_2, 0));
        // <= jbe
        // < jb
        // != jne jp
        switch (cond) {
            case x64::Equal: {
                Label ok;
                __ ucomisd(xmm0, xmm1);
                __ j(NotEqual, &ok, false/*is_far*/);
                __ j(ParityEven, &ok, false/*is_far*/);
                __ movl(ACC, 1);
                Label done;
                __ jmp(&done, false/*is_far*/);
                __ Bind(&ok);
                __ xorq(ACC, ACC);
                __ Bind(&done);
            } break;
            case x64::NotEqual: {
                Label ok;
                __ ucomisd(xmm0, xmm1);
                __ j(NotEqual, &ok, false/*is_far*/);
                __ j(ParityEven, &ok, false/*is_far*/);
                __ xorq(ACC, ACC);
                Label done;
                __ jmp(&done, false/*is_far*/);
                __ Bind(&ok);
                __ movl(ACC, 1);
                __ Bind(&done);
            } break;
            case x64::Less:
                __ ucomisd(xmm0, xmm1);
                __ set(Below, ACC);
                __ andl(ACC, 0xff);
                break;
            case x64::LessEqual:
                __ ucomisd(xmm0, xmm1);
                __ set(BelowEqual, ACC);
                __ andl(ACC, 0xff);
                break;
            case x64::Greater:
                __ ucomisd(xmm1, xmm0);
                __ set(Below, ACC);
                __ andl(ACC, 0xff);
                break;
            case x64::GreaterEqual:
                __ ucomisd(xmm1, xmm0);
                __ set(BelowEqual, ACC);
                __ andl(ACC, 0xff);
                break;
            default:
                NOREACHED();
                break;
        }
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
    
    void CheckArithmetic(MacroAssembler *masm, Register index, int size) {
        switch (size) {
            case 1:
                __ cmpb(Operand(rbp, index, times_2, 0), 0);
                break;
            case 2:
                __ cmpw(Operand(rbp, index, times_2, 0), 0);
                break;
            case 4:
                __ cmpl(Operand(rbp, index, times_2, 0), 0);
                break;
            case 8:
                __ cmpq(Operand(rbp, index, times_2, 0), 0);
                break;
            default:
                NOREACHED();
                break;
        }
        Label ok;
        __ j(NotEqual, &ok, false/*is_far*/);
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::NewArithmeticPanic), enable_jit);
        __ Throw(SCRATCH, rbx);

        __ Bind(&ok);
    }
    
    inline void CheckNotNil(MacroAssembler *masm, Register ptr) {
        __ cmpq(ptr, 0);
        Label ok;
        __ LikelyJ(NotEqual, &ok, false/*is_far*/);
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::NewNilPointerPanic), enable_jit);
        __ Throw(SCRATCH, rbx);
        __ int3();
        __ Bind(&ok);
    }

    template<class T>
    inline void CheckArrayBound(MacroAssembler *masm, Register array, Register index) {
        __ cmpl(index, 0);
        Label out_of_bound;
        __ j(Less, &out_of_bound, false/*is_far*/); // < 0
        __ cmpl(index, Operand(array, Array<T>::kOffsetLength));
        __ j(GreaterEqual, &out_of_bound, false/*is_far*/); // >= length
        Label ok;
        __ jmp(&ok, false/*is_far*/);
        __ Bind(&out_of_bound);
        __ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::NewOutOfBoundPanic), enable_jit);
        __ Throw(SCRATCH, rbx);
        __ int3();
        __ Bind(&ok);
    }
    
    const bool enable_debug;
    const bool enable_jit;
}; // class BytecodeEmitter

void Patch_BackwardJump(MacroAssembler *masm) {
    BytecodeEmitter::TracingBackwardJump(masm);
}

void Patch_CheckStack(MacroAssembler *masm) {
    BytecodeEmitter::TracingCheckStack(masm);
}

/*static*/ AbstractBytecodeEmitter *AbstractBytecodeEmitter::New(MetadataSpace *space,
                                                                 bool generated_debug_code,
                                                                 bool generated_profiling_code) {
    return new BytecodeEmitter(space, generated_debug_code, generated_profiling_code);
}

} // namespace lang

} // namespace mai
