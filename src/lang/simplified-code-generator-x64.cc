#include "lang/macro-assembler-x64.h"
#include "lang/compiler.h"
#include "lang/bytecode.h"
#include "lang/metadata.h"
#include "lang/isolate-inl.h"
#include "lang/stack-frame.h"
#include "lang/machine.h"
#include "lang/coroutine.h"
#include "lang/runtime.h"
#include "lang/value-inl.h"
#include "asm/utils.h"
#include "base/arenas.h"
#include "base/arena-utils.h"
#include <stack>


namespace mai {

namespace lang {

#define __ masm()->

#define EMIT_COMPARE8(cond) \
    __ movl(rax, StackOperand(0)); \
    __ cmpb(rax, StackOperand(1)); \
    __ set(cond, ACC); \
    __ andl(ACC, 0xff)

#define EMIT_COMPARE16(cond) \
    __ movl(rax, StackOperand(0)); \
    __ cmpw(rax, StackOperand(1)); \
    __ set(cond, ACC); \
    __ andl(ACC, 0xff)

#define EMIT_COMPARE32(cond) \
    __ movl(rax, StackOperand(0)); \
    __ cmpl(rax, StackOperand(1)); \
    __ set(cond, ACC); \
    __ andl(ACC, 0xff)

#define EMIT_COMPARE64(cond) \
    __ movq(rax, StackOperand(0)); \
    __ cmpq(rax, StackOperand(1)); \
    __ set(cond, ACC); \
    __ andl(ACC, 0xff)

// For Base-line JIT Compiler
// Generator simplified machhine code
class X64SimplifiedCodeGenerator final {
public:
    X64SimplifiedCodeGenerator(const CompilationInfo *compilation_info, bool enable_debug,
                               bool enable_jit, base::Arena *arena);
    
    void Initialize();
    
    void Generate();
    
    const BytecodeNode *bc() const { return bc_[position_]; }
    uint32_t pc() const { return compilation_info_->associated_pc()[position_]; }
    const Function *function() const { return fun_env_.top(); }
    MacroAssembler *masm() { return &masm_; }
    
    bool IsInlineCall(const BytecodeNode *) const {
        if (const BytecodeNode *next = GetNextOrNull()) {
            return next->id() == kCheckStack;
        } else {
            return false;
        }
    }
    
    const BytecodeNode *GetNextOrNull() const {
        return position_ + 1 >= compilation_info_->linear_path().size() ? nullptr :
               bc_[position_ + 1];
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(X64SimplifiedCodeGenerator);
private:
    void Select();
    
    Operand StackOperand(int index) const {
        return Operand(rbp, ParseStackOffset(bc()->param(index)));
    }

    Operand ConstOperand(Register scratch, int index) const {
        return Operand(scratch, ParseConstPoolOffset(bc()->param(index)));
    }
    
    Operand GlobalOperand(Register scratch, int index) const {
        return Operand(scratch, ParseGlobalSpaceOffset(bc()->param(index)));
    }
    
    template<class T>
    inline void BreakableCall(T *fun) {
        UpdatePC();
        Call(fun);
    }

    template<class T>
    inline void Call(T *fun) {
        __ InlineSwitchSystemStackCall(arch::FuncAddress(fun), enable_jit_);
    }
    
    void UpdatePC() { __ movl(Operand(rbp, BytecodeStackFrame::kOffsetPC), pc()); }
    
    void CheckNotNil(Register dest) {
        __ cmpq(dest, 0);
        Label ok;
        __ LikelyJ(NotEqual, &ok, false/*is_far*/);
        //__ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::NewNilPointerPanic), enable_jit_);
        BreakableCall(Runtime::NewNilPointerPanic);
        __ Throw(SCRATCH, rbx);
        __ int3();
        __ Bind(&ok);
    }
    
    void CheckException() {
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
    
    void EmitCompareImplicitLengthString(Cond cond);

    const CompilationInfo *const compilation_info_;
    const bool enable_debug_;
    const bool enable_jit_;
    base::Arena *const arena_;
    base::ArenaMap<const Function *, base::ArenaVector<uint32_t>> function_linear_pc_;
    BytecodeNode **bc_ = nullptr;
    size_t position_ = 0;
    std::stack<const Function *> fun_env_;
    MacroAssembler masm_;
}; // class X64SimplifiedCodeGenerator


X64SimplifiedCodeGenerator::X64SimplifiedCodeGenerator(const CompilationInfo *compilation_info,
                                                       bool enable_debug, bool enable_jit,
                                                       base::Arena *arena)
    : compilation_info_(compilation_info)
    , enable_debug_(enable_debug)
    , enable_jit_(enable_jit)
    , arena_(arena)
    , function_linear_pc_(arena) {}


void X64SimplifiedCodeGenerator::Initialize() {
    function_linear_pc_.emplace(compilation_info_->start_fun(),
                                base::ArenaVector<uint32_t>(arena_));
    fun_env_.push(compilation_info_->start_fun());
    
    bc_ = arena_->NewArray<BytecodeNode *>(compilation_info_->linear_path().size());
    for (size_t i = 0; i < compilation_info_->linear_path().size(); i++) {
        bc_[i] = BytecodeNode::From(arena_, compilation_info_->linear_path()[i]);
        if (bc_[i]->id() == kCheckStack) {
            auto iter = compilation_info_->invoke_info().find(i);
            DCHECK(iter != compilation_info_->invoke_info().end());
            fun_env_.push(iter->second.fun);
        }
        
        if (bc_[i]->id() == kReturn) {
            fun_env_.pop();
        }
        
        DCHECK(!fun_env_.empty());
        
        if (auto iter = function_linear_pc_.find(fun_env_.top());
            iter == function_linear_pc_.end()) {
            base::ArenaVector<uint32_t> linear_pc(arena_);
            linear_pc.push_back(compilation_info_->associated_pc()[i]);
            function_linear_pc_.insert(std::make_pair(fun_env_.top(), std::move(linear_pc)));
        } else {
            iter->second.push_back(compilation_info_->associated_pc()[i]);
        }
    }

    //fun_env_.pop();
    DCHECK(!fun_env_.empty());
}

void X64SimplifiedCodeGenerator::Generate() {
    for (position_ = 0; position_ < compilation_info_->linear_path().size(); position_++) {
        if (bc()->id() == kCheckStack) {
            auto iter = compilation_info_->invoke_info().find(position_);
            DCHECK(iter != compilation_info_->invoke_info().end());
            fun_env_.push(iter->second.fun);
        }
        
        Select();

        if (bc()->id() == kReturn) {
            fun_env_.pop();
        }
    }
}

void X64SimplifiedCodeGenerator::Select() {
    switch (bc()->id()) {
        // -----------------------------------------------------------------------------------------
        // Lda
        // -----------------------------------------------------------------------------------------
        case kLdar32:
            __ movl(ACC, StackOperand(0));
            break;

        case kLdar64:
        case kLdarPtr:
            __ movq(ACC, StackOperand(0));
            break;
            
        case kLdaf32:
            __ movss(FACC, StackOperand(0));
            break;
            
        case kLdaf64:
            __ movsd(FACC, StackOperand(0));
            break;
        
        case kLdaTrue:
            __ movl(ACC, 1);
            break;
        
        case kLdaFalse:
            __ xorl(ACC, ACC);
            break;
            
        case kLdaZero:
            __ xorq(ACC, ACC);
            __ xorsd(FACC, FACC);
            break;
            
        case kLdaSmi32:
            __ movq(ACC, bc()->param(0));
            break;
            
        // -----------------------------------------------------------------------------------------
        // LdaConst
        // -----------------------------------------------------------------------------------------
        case kLdaConst32:
            //__ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetConstPool));
            // Hard encode inline address
            __ movq(SCRATCH, bit_cast<Address>(function()->const_pool()));
            __ movl(ACC, ConstOperand(SCRATCH, 0));
            break;
            
        case kLdaConst64:
        case kLdaConstPtr:
            //__ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetConstPool));
            __ movq(SCRATCH, bit_cast<Address>(function()->const_pool()));
            __ movq(ACC, ConstOperand(SCRATCH, 0));
            break;
        
        case kLdaConstf32:
            //__ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetConstPool));
            __ movq(SCRATCH, bit_cast<Address>(function()->const_pool()));
            __ movss(FACC, ConstOperand(SCRATCH, 0));
            break;
            
        case kLdaConstf64:
            //__ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetConstPool));
            __ movq(SCRATCH, bit_cast<Address>(function()->const_pool()));
            __ movsd(FACC, ConstOperand(SCRATCH, 0));
            break;
            
        // -----------------------------------------------------------------------------------------
        // LdaGlobal
        // -----------------------------------------------------------------------------------------
        case kLdaGlobal32:
            // Hard inline address
            __ movq(SCRATCH, bit_cast<Address>(STATE->global_space()));
            __ movl(ACC, GlobalOperand(SCRATCH, 0));
            break;

        case kLdaGlobal64:
        case kLdaGlobalPtr:
            __ movq(SCRATCH, bit_cast<Address>(STATE->global_space()));
            __ movq(ACC, GlobalOperand(SCRATCH, 0));
            break;

        case kLdaGlobalf32:
            __ movq(SCRATCH, bit_cast<Address>(STATE->global_space()));
            __ movss(FACC, GlobalOperand(SCRATCH, 0));
            break;

        case kLdaGlobalf64:
            __ movq(SCRATCH, bit_cast<Address>(STATE->global_space()));
            __ movsd(FACC, GlobalOperand(SCRATCH, 0));
            break;

        // -----------------------------------------------------------------------------------------
        // LdaProperty
        // -----------------------------------------------------------------------------------------
        case kLdaProperty8:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ xorq(ACC, ACC);
            __ movb(ACC, Operand(SCRATCH, bc()->param(1)));
            break;

        case kLdaProperty16:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ xorq(ACC, ACC);
            __ movw(ACC, Operand(SCRATCH, bc()->param(1)));
            break;

        case kLdaProperty32:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movl(ACC, Operand(SCRATCH, bc()->param(1)));
            break;

        case kLdaProperty64:
        case kLdaPropertyPtr:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movq(ACC, Operand(SCRATCH, bc()->param(1)));
            break;

        case kLdaPropertyf32:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movss(FACC, Operand(SCRATCH, bc()->param(1)));
            break;

        case kLdaPropertyf64:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movsd(FACC, Operand(SCRATCH, bc()->param(1)));
            break;
        
        // -----------------------------------------------------------------------------------------
        // Increment
        // -----------------------------------------------------------------------------------------
        case kIncrement32:
            __ movl(ACC, StackOperand(0));
            __ addl(StackOperand(0), bc()->param(1));
            break;

        case kIncrement64:
            __ movq(ACC, StackOperand(0));
            __ addq(StackOperand(0), bc()->param(1));
            break;
            
        case kDecrement32:
            __ movl(ACC, StackOperand(0));
            __ subl(StackOperand(0), bc()->param(1));
            break;
            
        case kDecrement64:
            __ movq(ACC, StackOperand(0));
            __ subq(StackOperand(0), bc()->param(1));
            break;

        // -----------------------------------------------------------------------------------------
        // Test
        // -----------------------------------------------------------------------------------------
        case kTestLessThan8:
            EMIT_COMPARE8(Less);
            break;

        case kTestLessThanOrEqual8:
            EMIT_COMPARE8(LessEqual);
            break;

        case kTestGreaterThan8:
            EMIT_COMPARE8(Greater);
            break;

        case kTestGreaterThanOrEqual8:
            EMIT_COMPARE8(GreaterEqual);
            break;

        case kTestLessThan16:
            EMIT_COMPARE16(Less);
            break;

        case kTestLessThanOrEqual16:
            EMIT_COMPARE16(LessEqual);
            break;

        case kTestGreaterThan16:
            EMIT_COMPARE16(Greater);
            break;

        case kTestGreaterThanOrEqual16:
            EMIT_COMPARE16(GreaterEqual);
            break;

        case kTestEqual32:
            EMIT_COMPARE32(Equal);
            break;

        case kTestNotEqual32:
            EMIT_COMPARE32(NotEqual);
            break;

        case kTestLessThan32:
            EMIT_COMPARE32(Less);
            break;

        case kTestLessThanOrEqual32:
            EMIT_COMPARE32(LessEqual);
            break;

        case kTestGreaterThan32:
            EMIT_COMPARE32(Greater);
            break;

        case kTestGreaterThanOrEqual32:
            EMIT_COMPARE32(GreaterEqual);
            break;

        case kTestEqual64:
            EMIT_COMPARE64(Equal);
            break;

        case kTestNotEqual64:
            EMIT_COMPARE64(NotEqual);
            break;

        case kTestLessThan64:
            EMIT_COMPARE64(Less);
            break;

        case kTestLessThanOrEqual64:
            EMIT_COMPARE64(LessEqual);
            break;

        case kTestGreaterThan64:
            EMIT_COMPARE64(Greater);
            break;

        case kTestGreaterThanOrEqual64:
            EMIT_COMPARE64(GreaterEqual);
            break;

        case kTestEqualf32: {
            __ movss(xmm0, StackOperand(0));
            Label ok;
            __ ucomiss(xmm0, StackOperand(1));
            __ j(NotEqual, &ok, false/*is_far*/);
            __ j(ParityEven, &ok, false/*is_far*/);
            __ movl(ACC, 1);
            Label done;
            __ jmp(&done, false/*is_far*/);
            __ Bind(&ok);
            __ xorq(ACC, ACC);
            __ Bind(&done);
        } break;

        case kTestNotEqualf32: {
            __ movss(xmm0, StackOperand(0));
            Label ok;
            __ ucomiss(xmm0, StackOperand(1));
            __ j(NotEqual, &ok, false/*is_far*/);
            __ j(ParityEven, &ok, false/*is_far*/);
            __ xorq(ACC, ACC);
            Label done;
            __ jmp(&done, false/*is_far*/);
            __ Bind(&ok);
            __ movl(ACC, 1);
            __ Bind(&done);
        } break;

        case kTestLessThanf32:
            __ movss(xmm0, StackOperand(0));
            __ ucomiss(xmm0, StackOperand(1));
            __ set(Below, ACC);
            __ andl(ACC, 0xff);
            break;

        case kTestLessThanOrEqualf32:
            __ movss(xmm0, StackOperand(0));
            __ ucomiss(xmm0, StackOperand(1));
            __ set(BelowEqual, ACC);
            __ andl(ACC, 0xff);
            break;

        case kTestGreaterThanf32:
            __ movss(xmm0, StackOperand(1));
            __ ucomiss(xmm0, StackOperand(0));
            __ set(Below, ACC);
            __ andl(ACC, 0xff);
            break;

        case kTestGreaterThanOrEqualf32:
            __ movss(xmm0, StackOperand(1));
            __ ucomiss(xmm0, StackOperand(0));
            __ set(BelowEqual, ACC);
            __ andl(ACC, 0xff);
            break;

        case kTestEqualf64: {
           __ movsd(xmm0, StackOperand(0));
           Label ok;
           __ ucomisd(xmm0, StackOperand(1));
           __ j(NotEqual, &ok, false/*is_far*/);
           __ j(ParityEven, &ok, false/*is_far*/);
           __ movl(ACC, 1);
           Label done;
           __ jmp(&done, false/*is_far*/);
           __ Bind(&ok);
           __ xorq(ACC, ACC);
           __ Bind(&done);
       } break;

        case kTestNotEqualf64: {
            __ movsd(xmm0, StackOperand(0));
            Label ok;
            __ ucomisd(xmm0, StackOperand(1));
            __ j(NotEqual, &ok, false/*is_far*/);
            __ j(ParityEven, &ok, false/*is_far*/);
            __ xorq(ACC, ACC);
            Label done;
            __ jmp(&done, false/*is_far*/);
            __ Bind(&ok);
            __ movl(ACC, 1);
            __ Bind(&done);
        } break;

        case kTestLessThanf64:
            __ movsd(xmm0, StackOperand(0));
            __ ucomisd(xmm0, StackOperand(1));
            __ set(Below, ACC);
            __ andl(ACC, 0xff);
            break;

        case kTestLessThanOrEqualf64:
            __ movsd(xmm0, StackOperand(0));
            __ ucomisd(xmm0, StackOperand(1));
            __ set(BelowEqual, ACC);
            __ andl(ACC, 0xff);
            break;

        case kTestGreaterThanf64:
            __ movsd(xmm0, StackOperand(1));
            __ ucomisd(xmm0, StackOperand(0));
            __ set(Below, ACC);
            __ andl(ACC, 0xff);
            break;
            
        case kTestGreaterThanOrEqualf64:
            __ movsd(xmm0, StackOperand(1));
            __ ucomisd(xmm0, StackOperand(0));
            __ set(BelowEqual, ACC);
            __ andl(ACC, 0xff);
            break;

        case kTestPtrEqual:
            EMIT_COMPARE64(Equal);
            break;
            
        case kTestPtrNotEqual:
            EMIT_COMPARE64(NotEqual);
            break;

        case kTestStringEqual:
            EmitCompareImplicitLengthString(Equal);
            break;

        case kTestStringNotEqual:
            EmitCompareImplicitLengthString(NotEqual);
            break;

        case kTestStringLessThan:
            EmitCompareImplicitLengthString(Less);
            break;

        case kTestStringLessThanOrEqual:
            EmitCompareImplicitLengthString(LessEqual);
            break;

        case kTestStringGreaterThan:
            EmitCompareImplicitLengthString(Greater);
            break;

        case kTestStringGreaterThanOrEqual:
            EmitCompareImplicitLengthString(GreaterEqual);
            break;

        case kTestAs: {
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movq(ACC, Operand(SCRATCH, Any::kOffsetKlass));
            __ andq(ACC, ~1);

            __ movq(Argv_0, SCRATCH);
            //__ movq(rdx, Operand(rbp, BytecodeStackFrame::kOffsetConstPool));
            __ movq(rdx, bit_cast<Address>(function()->const_pool()));
            __ movq(Argv_1, ConstOperand(rdx, 1));
            //__ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::IsSameOrBaseOf), enable_jit_);
            Call(Runtime::IsSameOrBaseOf);
            __ cmpl(rax, 0);
            Label ok;
            __ j(NotEqual, &ok, false/*is_far*/);
            __ movq(ACC, Operand(SCRATCH, Any::kOffsetKlass));
            __ andq(ACC, ~1);
            __ movq(Argv_0, ACC);
            //__ movq(rdx, Operand(rbp, BytecodeStackFrame::kOffsetConstPool));
            __ movq(rdx, bit_cast<Address>(function()->const_pool()));
            __ movq(Argv_1, ConstOperand(rdx, 1));
            //__ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::NewBadCastPanic), enable_jit_);
            BreakableCall(Runtime::NewBadCastPanic);
            __ Throw(SCRATCH, rbx);

            __ Bind(&ok);
            __ movq(ACC, SCRATCH);
        } break;

        case kTestIs: {
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movq(ACC, Operand(SCRATCH, Any::kOffsetKlass));
            __ andq(ACC, ~1);

            __ movq(Argv_0, SCRATCH);
            __ movq(rdx, bit_cast<Address>(function()->const_pool()));
            __ movq(Argv_1, ConstOperand(rdx, 1));
            //__ InlineSwitchSystemStackCall(arch::FuncAddress(Runtime::IsSameOrBaseOf), enable_jit_);
            Call(Runtime::IsSameOrBaseOf);
        } break;
            
        // -----------------------------------------------------------------------------------------
        // Calling
        // -----------------------------------------------------------------------------------------
        case kCallFunction:
            TODO();
            break;
            
        case kCallNativeFunction: {
            // Update current PC
            UpdatePC();

            // Adjust Caller Stack
            // Adjust sp to aligment of 16 bits(2 bytes)
            __ subq(rsp, RoundUp(bc()->param(1), kStackAligmentSize));

            __ movq(Argv_0, ACC);
            __ movq(SCRATCH, Operand(ACC, Closure::kOffsetCode));
            __ leaq(SCRATCH, Operand(SCRATCH, Code::kOffsetEntry));
            __ call(SCRATCH); // Call stub code
            // The stub code should switch to system stack and call real function.

            // Recover Caller Stack
            // Adjust sp to aligment of 16 bits(2 bytes)
            __ addq(rsp, RoundUp(bc()->param(1), kStackAligmentSize));

            CheckException();
        } break;

        case kCallBytecodeFunction: {
            // TODO: slot
            UpdatePC(); // Update current PC first
        
            // Adjust Caller Stack
            __ subq(rsp, RoundUp(bc()->param(1), kStackAligmentSize));

            if (IsInlineCall(bc())) {
                // TODO:
            } else {
                __ movq(Argv_0, ACC);
                __ movq(SCRATCH,  STATE->metadata_space()->interpreter_pump_code()->entry());
                __ call(SCRATCH);
                
                // Recover BC Register
                __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetBytecodeArray));
                // SCRATCH = &bytecodes->instructions
                __ addq(SCRATCH, BytecodeArray::kOffsetEntry);
                __ movl(rbx, Operand(rbp, BytecodeStackFrame::kOffsetPC));
                // XXX: Recover BC register first!
                __ leaq(BC, Operand(SCRATCH, rbx, times_4, 0)); // [SCRATCH + rbx * 4]
            }

            // Recover Caller Stack
            __ addq(rsp, RoundUp(bc()->param(1), kStackAligmentSize));
        } break;
            
        case kReturn:
            TODO();
            break;
        
        // -----------------------------------------------------------------------------------------
        // Others
        // -----------------------------------------------------------------------------------------
        case kCheckStack: {
            __ cmpq(rsp, Operand(CO, Coroutine::kOffsetStackGuard0));
            Label ok;
            __ j(GreaterEqual, &ok, false/*is_far*/);
            BreakableCall(Runtime::NewStackoverflowPanic);
            __ Throw(SCRATCH, rbx);
            __ Bind(&ok);
        } break;
            
        default:
            NOREACHED();
            break;
    }
}

void X64SimplifiedCodeGenerator::EmitCompareImplicitLengthString(Cond cond) {
    __ movq(rsi, StackOperand(0));
    __ movq(rdi, StackOperand(1));

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

Kode *GenerateSimplifiedCode(const CompilationInfo *compilation_info, bool enable_debug,
                             bool enable_jit, base::Arena *arena) {
    X64SimplifiedCodeGenerator g(compilation_info, enable_debug, enable_jit, arena);
    g.Initialize();
    g.Generate();
    return Machine::This()->NewCode(Code::OPTIMIZATION, 1,
                                    static_cast<uint32_t>(g.masm()->buf().size()),
                                    reinterpret_cast<const uint8_t *>(&g.masm()->buf()[0]), 0);
}

} // namespace lang

} // namespace mai
