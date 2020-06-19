#include "lang/simplified-code-generator.h"
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

#define EMIT_SIMPLE_ARITHMETIC(bc, op) \
    case k##bc##8: \
        __ movl(ACC, StackOperand(0)); \
        __ op##b(ACC, StackOperand(1)); \
        break; \
    case k##bc##16: \
        __ movl(ACC, StackOperand(0)); \
        __ op##w(ACC, StackOperand(1)); \
        break; \
    case k##bc##32: \
        __ movl(ACC, StackOperand(0)); \
        __ op##l(ACC, StackOperand(1)); \
        break; \
    case k##bc##64: \
        __ movq(ACC, StackOperand(0)); \
        __ op##q(ACC, StackOperand(1)); \
        break; \
    case k##bc##f32: \
        __ movss(FACC, StackOperand(0)); \
        __ op##ss(FACC, StackOperand(1)); \
        break; \
    case k##bc##f64: \
        __ movsd(FACC, StackOperand(0)); \
        __ op##sd(FACC, StackOperand(1)); \
        break

#if defined(DEBUG) || defined(_DEBUG)
#define CHECK_CAPTURED_VAR_INDEX(callee, index) \
    __ cmpl(Operand(callee, Closure::kOffsetCapturedVarSize), index); \
    Label ok1; \
    __ j(Greater, &ok1, false); \
    __ Abort("CapturedVarIndex out of bound!"); \
    __ Bind(&ok1)
#else
#define CHECK_CAPTURED_VAR_INDEX(callee, index) (void)0
#endif // defined(DEBUG) || defined(_DEBUG)

// For Base-line JIT Compiler
// Generator simplified machhine code
class X64SimplifiedCodeGenerator final {
public:
    X64SimplifiedCodeGenerator(const CompilationInfo *compilation_info, bool enable_debug,
                               bool enable_jit, base::Arena *arena);
    
    void Initialize();
    
    void Generate();
    
    MacroAssembler *masm() { return &masm_; }
    
    class Environment : public base::ArenaObject {
    public:
        Environment(X64SimplifiedCodeGenerator *owns, const Function *fun);
        
        ~Environment() {
            DCHECK_EQ(owns_->env_, this);
            owns_->env_ = prev_;
        }
        
        DEF_PTR_GETTER(const Function, fun);
        bool is_top() const { return prev_ == nullptr; }
        
        void AddLinearPC(uint32_t pc) { linear_pc_.push_back(pc); }
        void AddAsmPC(uint32_t pc, int32_t asm_pc) { asm_offset_[pc] = asm_pc; }
        
        int32_t FindAsmPC(uint32_t pc) {
            auto iter = asm_offset_.find(pc);
            DCHECK(iter != asm_offset_.end());
            return iter->second;
        }
        
        uint32_t smallest_pc() const { return linear_pc_.front(); }
        uint32_t largest_pc() const { return linear_pc_.back(); }
    private:
        X64SimplifiedCodeGenerator *const owns_;
        const Function *const fun_;
        base::ArenaVector<uint32_t> linear_pc_;
        base::ArenaMap<uint32_t, int32_t> asm_offset_;
        Environment *prev_ = nullptr;
    }; // class Environment
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(X64SimplifiedCodeGenerator);
private:
    void Select();
    
    bool IsInlineCall(const BytecodeNode *) const {
        if (const BytecodeNode *next = GetNextBytecodeOrNull()) {
            return next->id() == kCheckStack;
        } else {
            return false;
        }
    }
    
    uint32_t GetNextPC() const {
        return position_ + 1 >= compilation_info_->associated_pc().size() ? -1 :
            compilation_info_->associated_pc()[position_ + 1];
    }
    
    const BytecodeNode *GetNextBytecodeOrNull() const {
        return position_ + 1 >= compilation_info_->linear_path().size() ? nullptr :
               bc_[position_ + 1];
    }
    
    const Function *GetNextFunctionOrNull() const {
        auto iter = compilation_info_->invoke_info().find(position_ + 1);
        return iter == compilation_info_->invoke_info().end() ? nullptr : iter->second.fun;
    }
    
    bool IsPCOutOfBound(uint32_t pc) const {
        return !IsPCInBound(pc);
    }
    
    bool IsPCInBound(uint32_t pc) const {
        return pc >= env_->smallest_pc() && pc <= env_->largest_pc();
    }
    
    Operand StackOperand(int index) const {
        return Operand(rbp, -ParseStackOffset(bc()->param(index)));
    }

    Operand ConstOperand(Register scratch, int index) const {
        return Operand(scratch, ParseConstOffset(bc()->param(index)));
    }
    
    Operand GlobalOperand(Register scratch, int index) const {
        return Operand(scratch, ParseGlobalOffset(bc()->param(index)));
    }
    
    Operand CapturedSlotOperand(Register scratch, int index) const {
        int32_t offset = Closure::kOffsetCapturedVar
                       + bc()->param(index) * sizeof(Closure::CapturedVar);
        return Operand(scratch, offset);
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
    
    void MakeExitGuard() {
        uint32_t last_pc = DCHECK_NOTNULL(env_)->largest_pc();
        // to next pc
        Deoptimize(last_pc + 1);
    }
    
    void CallBytecodeFunction();
    
    void MakeInlineStackFrame(const Function *fun);
    
    void UpdatePC() {
        __ movl(Operand(rbp, BytecodeStackFrame::kOffsetPC), pc());
    }
    
    void CheckNotNil(Register dest) {
        __ cmpq(dest, 0);
        Label ok;
        __ LikelyJ(NotEqual, &ok, false/*is_far*/);
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
    
    template<class T>
    void CheckArithmetic(int index) {
        switch (sizeof(T)) {
            case 1:
                __ cmpb(StackOperand(index), 0);
                break;
            case 2:
                __ cmpw(StackOperand(index), 0);
                break;
            case 4:
                __ cmpl(StackOperand(index), 0);
                break;
            case 8:
                __ cmpq(StackOperand(index), 0);
                break;
            default:
                NOREACHED();
                break;
        }
        Label ok;
        __ j(NotEqual, &ok, false/*is_far*/);
        BreakableCall(Runtime::NewArithmeticPanic);
        __ Throw(SCRATCH, rbx);
        __ Bind(&ok);
    }
    
    template<class T>
    inline void CheckArrayBound(Register array, Register index) {
        __ cmpl(index, 0);
        Label out_of_bound;
        __ j(Less, &out_of_bound, false/*is_far*/); // < 0
        __ cmpl(index, Operand(array, Array<T>::kOffsetLength));
        __ j(GreaterEqual, &out_of_bound, false/*is_far*/); // >= length
        Label ok;
        __ jmp(&ok, false/*is_far*/);
        __ Bind(&out_of_bound);
        BreakableCall(Runtime::NewOutOfBoundPanic);
        __ Throw(SCRATCH, rbx);
        __ int3();
        __ Bind(&ok);
    }
    
    void Deoptimize(uint32_t pc) {
        __ movq(SCRATCH, Operand(CO, Coroutine::kOffsetHotPath));
        __ cmpq(SCRATCH, 0);
        Label exit;
        __ j(Equal, &exit, false/*is_far*/);
        __ cmpl(Operand(SCRATCH, Kode::kOffsetSlot), compilation_info_->start_slot());
        __ j(NotEqual, &exit, false/*is_far*/);
        __ movq(Operand(SCRATCH, Kode::kOffsetSlot), 0);
        __ mfence();

        __ Bind(&exit);
        __ movl(Operand(rbp, BytecodeStackFrame::kOffsetPC), pc);
        __ StartBC();
    }
    
    void ForwardJumpIf(bool cond);
    
    void BackwardJumpIf(bool cond);

    void CompareImplicitLengthString(Cond cond);
    
    const BytecodeNode *bc() const { return bc_[position_]; }
    uint32_t pc() const { return compilation_info_->associated_pc()[position_]; }
    const Function *function() const { return env_->fun(); }

    const CompilationInfo *const compilation_info_;
    const bool enable_debug_;
    const bool enable_jit_;
    base::Arena *const arena_;
    base::ArenaMap<const Function *, base::ArenaVector<uint32_t>> function_linear_pc_;
    BytecodeNode **bc_ = nullptr;
    size_t position_ = 0;
    Environment *env_ = nullptr;
    //base::ArenaDeque<const Function *> fun_env_;
    MacroAssembler masm_;
}; // class X64SimplifiedCodeGenerator


X64SimplifiedCodeGenerator::Environment::Environment(X64SimplifiedCodeGenerator *owns,
                                                     const Function *fun)
    : owns_(owns)
    , fun_(fun)
    , linear_pc_(owns->arena_)
    , asm_offset_(owns->arena_) {
    prev_ = owns_->env_;
    owns_->env_ = this;

    auto iter = owns_->function_linear_pc_.find(fun);
    DCHECK(iter != owns_->function_linear_pc_.end());
    for (auto pc : iter->second) {
        AddLinearPC(pc);
    }

    std::sort(linear_pc_.begin(), linear_pc_.end());
}


X64SimplifiedCodeGenerator::X64SimplifiedCodeGenerator(const CompilationInfo *compilation_info,
                                                       bool enable_debug, bool enable_jit,
                                                       base::Arena *arena)
    : compilation_info_(compilation_info)
    , enable_debug_(enable_debug)
    , enable_jit_(enable_jit)
    , arena_(arena)
    , function_linear_pc_(arena) {
}


void X64SimplifiedCodeGenerator::Initialize() {
    bc_ = arena_->NewArray<BytecodeNode *>(compilation_info_->linear_path().size());
    for (size_t i = 0; i < compilation_info_->linear_path().size(); i++) {
        bc_[i] = BytecodeNode::From(arena_, compilation_info_->linear_path()[i]);
    }

    base::ArenaDeque<const Function *> env(arena_);
    env.push_back(compilation_info_->start_fun());
    
    for (size_t i = 0; i < compilation_info_->linear_path().size(); i++) {
        if (bc_[i]->id() == kCheckStack) {
            auto iter = compilation_info_->invoke_info().find(i);
            DCHECK(iter != compilation_info_->invoke_info().end());

            env.push_back(iter->second.fun);
        }

        auto iter = function_linear_pc_.find(env.back());
        if (iter == function_linear_pc_.end()) {
            base::ArenaVector<uint32_t> linear_pc(arena_);
            linear_pc.push_back(compilation_info_->associated_pc()[i]);
            function_linear_pc_.insert(std::make_pair(env.back(), std::move(linear_pc)));
        } else {
            iter->second.push_back(compilation_info_->associated_pc()[i]);
        }

        if (bc_[i]->id() == kReturn) {
            env.pop_back();
        }
    }

    DCHECK_EQ(1, env.size());
}

void X64SimplifiedCodeGenerator::Generate() {
    (void)new (arena_) Environment(this, compilation_info_->start_fun());
    
    for (position_ = 0; position_ < compilation_info_->linear_path().size(); position_++) {
        if (bc()->id() == kCheckStack) {
            auto iter = compilation_info_->invoke_info().find(position_);
            DCHECK(iter != compilation_info_->invoke_info().end());

            (void)new (arena_) Environment(this, iter->second.fun);
        }

        env_->AddAsmPC(pc(), masm()->pc());
        Select();

        if (bc()->id() == kReturn) {
            env_->~Environment();
        }
    }
    MakeExitGuard();
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
        // LdaArray
        // -----------------------------------------------------------------------------------------
        case kLdaArrayAt8:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movl(rdx, StackOperand(1));
            CheckArrayBound<int8_t>(SCRATCH/*array*/, rdx/*index*/);
            __ xorl(ACC, ACC);
            __ movb(ACC, Operand(SCRATCH, rdx, times_1, Array<int8_t>::kOffsetElems));
            break;

        case kLdaArrayAt16:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movl(rdx, StackOperand(1));
            CheckArrayBound<int16_t>(SCRATCH/*array*/, rdx/*index*/);
            __ xorl(ACC, ACC);
            __ movw(ACC, Operand(SCRATCH, rdx, times_2, Array<int16_t>::kOffsetElems));
            break;

        case kLdaArrayAt32:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movl(rdx, StackOperand(1));
            CheckArrayBound<int32_t>(SCRATCH/*array*/, rdx/*index*/);
            __ movl(ACC, Operand(SCRATCH, rdx, times_4, Array<int32_t>::kOffsetElems));
            break;

        case kLdaArrayAt64:
        case kLdaArrayAtPtr:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movl(rdx, StackOperand(1));
            CheckArrayBound<int64_t>(SCRATCH/*array*/, rdx/*index*/);
            __ movq(ACC, Operand(SCRATCH, rdx, times_8, Array<int64_t>::kOffsetElems));
            break;

        case kLdaArrayAtf32:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movl(rdx, StackOperand(1));
            CheckArrayBound<float>(SCRATCH/*array*/, rdx/*index*/);
            __ movss(FACC, Operand(SCRATCH, rdx, times_4, Array<float>::kOffsetElems));
            break;

        case kLdaArrayAtf64:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movl(rdx, StackOperand(1));
            CheckArrayBound<double>(SCRATCH/*array*/, rdx/*index*/);
            __ movsd(FACC, Operand(SCRATCH, rdx, times_8, Array<double>::kOffsetElems));
            break;

        // -----------------------------------------------------------------------------------------
        // LdaCaptured
        // -----------------------------------------------------------------------------------------
        case kLdaCaptured32: {
            __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetCallee));
            CHECK_CAPTURED_VAR_INDEX(SCRATCH, bc()->param(0));
            __ movq(SCRATCH, CapturedSlotOperand(SCRATCH, 0));
            __ movl(ACC, Operand(SCRATCH, CapturedValue::kOffsetValue));
        } break;

        case kLdaCaptured64:
        case kLdaCapturedPtr: {
               __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetCallee));
               CHECK_CAPTURED_VAR_INDEX(SCRATCH, bc()->param(0));
               __ movq(SCRATCH, CapturedSlotOperand(SCRATCH, 0));
               __ movq(ACC, Operand(SCRATCH, CapturedValue::kOffsetValue));
           } break;

        case kLdaCapturedf32: {
            __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetCallee));
            CHECK_CAPTURED_VAR_INDEX(SCRATCH, bc()->param(0));
            __ movq(SCRATCH, CapturedSlotOperand(SCRATCH, 0));
            __ movss(FACC, Operand(SCRATCH, CapturedValue::kOffsetValue));
        } break;

        case kLdaCapturedf64: {
            __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetCallee));
            CHECK_CAPTURED_VAR_INDEX(SCRATCH, bc()->param(0));
            __ movq(SCRATCH, CapturedSlotOperand(SCRATCH, 0));
            __ movsd(FACC, Operand(SCRATCH, CapturedValue::kOffsetValue));
        } break;
            
        // -----------------------------------------------------------------------------------------
        // Star
        // -----------------------------------------------------------------------------------------
        case kStar32:
            __ movl(StackOperand(0), ACC);
            break;

        case kStar64:
        case kStarPtr:
            __ movq(StackOperand(0), ACC);
            break;

        case kStaf32:
            __ movss(StackOperand(0), FACC);
            break;

        case kStaf64:
            __ movsd(StackOperand(0), FACC);
            break;

        // -----------------------------------------------------------------------------------------
        // StaGlobal
        // -----------------------------------------------------------------------------------------
        case kStaGlobal32:
            // Hard inline address
            __ movq(SCRATCH, reinterpret_cast<Address>(STATE->global_space()));
            __ movl(GlobalOperand(SCRATCH, 0), ACC);
            break;

        case kStaGlobal64:
        case kStaGlobalPtr:
            __ movq(SCRATCH, reinterpret_cast<Address>(STATE->global_space()));
            __ movq(GlobalOperand(SCRATCH, 0), ACC);
            break;

        case kStaGlobalf32:
            __ movq(SCRATCH, reinterpret_cast<Address>(STATE->global_space()));
            __ movss(GlobalOperand(SCRATCH, 0), FACC);
            break;

        case kStaGlobalf64:
            __ movq(SCRATCH, reinterpret_cast<Address>(STATE->global_space()));
            __ movsd(GlobalOperand(SCRATCH, 0), FACC);
            break;

        // -----------------------------------------------------------------------------------------
        // StaProperty
        // -----------------------------------------------------------------------------------------
        case kStaProperty8:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movb(Operand(SCRATCH, bc()->param(1)), ACC);
            break;

        case kStaProperty16:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movw(Operand(SCRATCH, bc()->param(1)), ACC);
            break;

        case kStaProperty32:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movl(Operand(SCRATCH, bc()->param(1)), ACC);
            break;

        case kStaProperty64:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movq(Operand(SCRATCH, bc()->param(1)), ACC);
            break;

        case kStaPropertyPtr:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movq(Operand(SCRATCH, bc()->param(1)), ACC);
            __ WriteBarrier(SCRATCH, bc()->param(1), enable_jit_);
            break;

        case kStaPropertyf32:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movss(Operand(SCRATCH, bc()->param(1)), FACC);
            break;
            
        case kStaPropertyf64:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movsd(Operand(SCRATCH, bc()->param(1)), FACC);
            break;
            
        // -----------------------------------------------------------------------------------------
        // StaArray
        // -----------------------------------------------------------------------------------------
        case kStaArrayAt8:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movl(rdx, StackOperand(1));
            CheckArrayBound<int8_t>(SCRATCH, rdx);
            __ movb(Operand(SCRATCH, rdx, times_1, Array<int8_t>::kOffsetElems), ACC);
            break;

        case kStaArrayAt16:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movl(rdx, StackOperand(1));
            CheckArrayBound<int16_t>(SCRATCH, rdx);
            __ movw(Operand(SCRATCH, rdx, times_2, Array<int16_t>::kOffsetElems), ACC);
            break;

        case kStaArrayAt32:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movl(rdx, StackOperand(1));
            CheckArrayBound<int32_t>(SCRATCH, rdx);
            __ movl(Operand(SCRATCH, rdx, times_4, Array<int32_t>::kOffsetElems), ACC);
            break;

        case kStaArrayAt64:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movl(rdx, StackOperand(1));
            CheckArrayBound<int64_t>(SCRATCH, rdx);
            __ movq(Operand(SCRATCH, rdx, times_8, Array<int64_t>::kOffsetElems), ACC);
            break;

        case kStaArrayAtPtr: {
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movl(rdx, StackOperand(1));
            CheckArrayBound<Any *>(SCRATCH, rdx);

            Operand address(SCRATCH, rdx, times_ptr_size, Array<Any *>::kOffsetElems);
            __ movq(address, ACC);
            __ WriteBarrier(SCRATCH, address, enable_jit_);
        } break;

        case kStaArrayAtf32:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movl(rdx, StackOperand(1));
            CheckArrayBound<float>(SCRATCH, rdx);
            __ movl(Operand(SCRATCH, rdx, times_4, Array<float>::kOffsetElems), ACC);
            break;

        case kStaArrayAtf64:
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movl(rdx, StackOperand(1));
            CheckArrayBound<double>(SCRATCH, rdx);
            __ movl(Operand(SCRATCH, rdx, times_8, Array<double>::kOffsetElems), ACC);
            break;
            
        // -----------------------------------------------------------------------------------------
        // StaCaptured
        // -----------------------------------------------------------------------------------------
        case kStaCaptured32: {
            __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetCallee));
            CHECK_CAPTURED_VAR_INDEX(SCRATCH, bc()->param(0));
            __ movq(SCRATCH, CapturedSlotOperand(SCRATCH, 0));
            __ movl(Operand(SCRATCH, CapturedValue::kOffsetValue), ACC);
        } break;

        case kStaCaptured64: {
            __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetCallee));
            CHECK_CAPTURED_VAR_INDEX(SCRATCH, bc()->param(0));
            __ movq(SCRATCH, CapturedSlotOperand(SCRATCH, 0));
            __ movq(Operand(SCRATCH, CapturedValue::kOffsetValue), ACC);
        } break;

        case kStaCapturedPtr: {
            __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetCallee));
            CHECK_CAPTURED_VAR_INDEX(SCRATCH, bc()->param(0));
            __ movq(SCRATCH, CapturedSlotOperand(SCRATCH, 0));
            __ movq(Operand(SCRATCH, CapturedValue::kOffsetValue), ACC);
            __ WriteBarrier(SCRATCH, CapturedValue::kOffsetValue, enable_jit_);
        } break;

        case kStaCapturedf32: {
            __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetCallee));
            CHECK_CAPTURED_VAR_INDEX(SCRATCH, bc()->param(0));
            __ movq(SCRATCH, CapturedSlotOperand(SCRATCH, 0));
            __ movss(Operand(SCRATCH, CapturedValue::kOffsetValue), FACC);
        } break;
            
        case kStaCapturedf64: {
            __ movq(SCRATCH, Operand(rbp, BytecodeStackFrame::kOffsetCallee));
            CHECK_CAPTURED_VAR_INDEX(SCRATCH, bc()->param(0));
            __ movq(SCRATCH, CapturedSlotOperand(SCRATCH, 0));
            __ movsd(Operand(SCRATCH, CapturedValue::kOffsetValue), FACC);
        } break;

        // -----------------------------------------------------------------------------------------
        // Move
        // -----------------------------------------------------------------------------------------
        case kMove32:
            __ movl(rbx, StackOperand(1));
            __ movl(StackOperand(0), rbx);
            break;

        case kMove64:
        case kMovePtr:
            __ movq(rbx, StackOperand(1));
            __ movq(StackOperand(0), rbx);
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
        // Arithmetic
        // -----------------------------------------------------------------------------------------
        EMIT_SIMPLE_ARITHMETIC(Add, add);
        EMIT_SIMPLE_ARITHMETIC(Sub, sub);

        case kMul8:
            __ movl(ACC, StackOperand(0));
            __ mulb(StackOperand(1));
            break;
        
        case kMul16:
            __ movl(ACC, StackOperand(0));
            __ mulw(StackOperand(1));
            break;
            
        case kMul32:
            __ movl(ACC, StackOperand(0));
            __ mull(StackOperand(1));
            break;
            
        case kMul64:
            __ movq(ACC, StackOperand(0));
            __ mulq(StackOperand(1));
            break;
            
        case kMulf32:
            __ movss(FACC, StackOperand(0));
            __ mulss(FACC, StackOperand(1));
            break;
            
        case kMulf64:
            __ movsd(FACC, StackOperand(0));
            __ mulsd(FACC, StackOperand(1));
            break;
        
        case kIMul8:
            __ movl(ACC, StackOperand(0));
            __ imulb(StackOperand(1));
            break;
            
        case kIMul16:
            __ movl(ACC, StackOperand(0));
            __ imulw(ACC, StackOperand(1));
            break;
        
        case kIMul32:
            __ movl(ACC, StackOperand(0));
            __ imull(ACC, StackOperand(1));
            break;
            
        case kIMul64:
            __ movq(ACC, StackOperand(0));
            __ imulq(ACC, StackOperand(1));
            break;
            
        case kDiv8:
            __ movl(ACC, StackOperand(0));
            CheckArithmetic<uint8_t>(1/*index*/);
            __ divb(StackOperand(1));
            __ andl(ACC, 0xff);
            break;
            
        case kDiv16:
            __ movl(ACC, StackOperand(0));
            CheckArithmetic<uint16_t>(1/*index*/);
            __ xorl(rdx, rdx);
            __ divw(StackOperand(1));
            __ andl(ACC, 0xffff);
            break;
            
        case kDiv32:
            __ movl(ACC, StackOperand(0));
            CheckArithmetic<uint32_t>(1/*index*/);
            __ xorl(rdx, rdx);
            __ divl(StackOperand(1));
            break;
            
        case kDiv64:
            __ movl(ACC, StackOperand(0));
            CheckArithmetic<uint8_t>(1/*index*/);
            __ xorq(rdx, rdx);
            __ divq(StackOperand(1));
            break;

        case kDivf32:
            __ movss(FACC, StackOperand(0));
            __ divss(FACC, StackOperand(1));
            break;

        case kDivf64:
            __ movsd(FACC, StackOperand(0));
            __ divsd(FACC, StackOperand(1));
            break;
            
        case kIDiv8:
            __ movl(ACC, StackOperand(0));
            CheckArithmetic<int8_t>(1/*index*/);
            __ idivb(StackOperand(1));
            __ andl(ACC, 0xff);
            break;
            
        case kIDiv16:
            __ movl(ACC, StackOperand(0));
            CheckArithmetic<int16_t>(1/*index*/);
            __ xorl(rdx, rdx);
            __ idivw(StackOperand(1));
            break;
            
        case kIDiv32:
            __ movl(ACC, StackOperand(0));
            CheckArithmetic<int32_t>(1/*index*/);
            __ xorl(rdx, rdx);
            __ idivl(StackOperand(1));
            break;
            
        case kIDiv64:
            __ movl(ACC, StackOperand(0));
            CheckArithmetic<int64_t>(1/*index*/);
            __ xorq(rdx, rdx);
            __ idivq(StackOperand(1));
            break;

        case kMod8:
            __ movl(ACC, StackOperand(0));
            CheckArithmetic<uint8_t>(1/*index*/);
            // al <- ax / operand
            // ah <- remainder
            __ divb(StackOperand(1));
            __ shrl(ACC, 8);
            __ andl(ACC, 0xff);
            break;

        case kMod16:
            __ movl(ACC, StackOperand(0));
            CheckArithmetic<uint16_t>(1/*index*/);
            // ax:dx <- ax / operand
            __ xorl(rdx, rdx);
            __ divw(StackOperand(1));
            __ movl(ACC, rdx);
            break;

        case kMod32:
            __ movl(ACC, StackOperand(0));
            CheckArithmetic<uint32_t>(1/*index*/);
            // rax:rdx <- rax / operand
            __ divl(StackOperand(1));
            __ movl(ACC, rdx);
            break;

        case kMod64:
            __ movq(ACC, StackOperand(0));
            CheckArithmetic<uint64_t>(1/*index*/);
            // rax:rdx <- rax / operand
            __ divq(StackOperand(1));
            __ movq(ACC, rdx);
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
            CompareImplicitLengthString(Equal);
            break;

        case kTestStringNotEqual:
            CompareImplicitLengthString(NotEqual);
            break;

        case kTestStringLessThan:
            CompareImplicitLengthString(Less);
            break;

        case kTestStringLessThanOrEqual:
            CompareImplicitLengthString(LessEqual);
            break;

        case kTestStringGreaterThan:
            CompareImplicitLengthString(Greater);
            break;

        case kTestStringGreaterThanOrEqual:
            CompareImplicitLengthString(GreaterEqual);
            break;

        case kTestAs: {
            __ movq(SCRATCH, StackOperand(0));
            CheckNotNil(SCRATCH);
            __ movq(ACC, Operand(SCRATCH, Any::kOffsetKlass));
            __ andq(ACC, ~1);

            __ movq(Argv_0, SCRATCH);
            //__ movq(rdx, Operand(rbp, BytecodeStackFrame::kOffsetConstPool));
            __ movq(rdx, reinterpret_cast<Address>(function()->const_pool()));
            __ movq(Argv_1, ConstOperand(rdx, 1));
            Call(Runtime::IsSameOrBaseOf);
            __ cmpl(rax, 0);
            Label ok;
            __ j(NotEqual, &ok, false/*is_far*/);
            __ movq(ACC, Operand(SCRATCH, Any::kOffsetKlass));
            __ andq(ACC, ~1);
            __ movq(Argv_0, ACC);
            //__ movq(rdx, Operand(rbp, BytecodeStackFrame::kOffsetConstPool));
            __ movq(rdx, reinterpret_cast<Address>(function()->const_pool()));
            __ movq(Argv_1, ConstOperand(rdx, 1));
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
            Call(Runtime::IsSameOrBaseOf);
        } break;
            
        // -----------------------------------------------------------------------------------------
        // Jumping
        // -----------------------------------------------------------------------------------------
        case kForwardJump: {
            int slot = bc()->param(0);
            (void)slot;
            int delta = bc()->param(1);
            int dest = pc() + delta;
            
            if (IsPCOutOfBound(dest)) {
                Deoptimize(dest);
            }
            DCHECK_EQ(GetNextPC(), dest);
        } break;

        case kForwardJumpIfTrue:
            ForwardJumpIf(true);
            break;

        case kForwardJumpIfFalse:
            //__ Breakpoint();
            ForwardJumpIf(false);
            break;

        case kBackwardJump: {
            int slot = bc()->param(0);
            (void)slot;
            int delta = bc()->param(1);
            DCHECK_GE(delta, 0);
            int dest = pc() - delta;
            DCHECK_GE(dest, 0);
            
            if (IsPCOutOfBound(dest)) {
                Deoptimize(dest);
            } else {
                int32_t asm_pc = env_->FindAsmPC(dest);
                //__ Breakpoint();
                __ jmp(asm_pc);
            }
        } break;

        case kBackwardJumpIfTrue:
            BackwardJumpIf(true);
            break;

        case kBackwardJumpIfFalse:
            BackwardJumpIf(false);
            break;

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

        case kCallBytecodeFunction:
            CallBytecodeFunction();
            break;
            
        case kReturn: {
            //__ Breakpoint();
            if (function()->exception_table_size() > 0) {
                __ UninstallCaughtHandler();
            }
            if (env_->is_top()) { // TOP calling
                __ addq(rsp, function()->stack_size()); // Recover stack
                __ popq(rbp);
                __ ret(0);
            } else {
                // Inline return
                __ addq(rsp, function()->stack_size()); // Recover stack
                __ popq(rbp);
                __ popq(SCRATCH); // Unused dummy return address
                
                // Recover Caller Stack
                __ addq(rsp,
                        static_cast<int32_t>(function()->prototype()->GetParametersPlacedSize()));
            }
        } break;
        
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
            
        default: {
        #if defined(DEBUG) || defined(_DEBUG)
            base::StringBuildingPrinter printer;
            bc()->Print(&printer);
            NOREACHED() << printer.buffer();
        #endif
        } break;
    }
}

void X64SimplifiedCodeGenerator::ForwardJumpIf(bool cond) {
    int slot = bc()->param(0);
    (void)slot;
    int delta = bc()->param(1);
    DCHECK_GE(delta, 0);
    int dest = pc() + delta;
    DCHECK_GE(dest, 0);

    if (IsPCOutOfBound(dest)) {
        // False Guard
        __ cmpl(ACC, 0);
        Label guard;
        if (cond) {
            __ j(Equal, &guard, false/*is_far*/);
        } else {
            __ j(NotEqual, &guard, false/*is_far*/);
        }

        Deoptimize(dest);
        __ Bind(&guard);
    } else {
        DCHECK_EQ(GetNextPC(), dest);
        
        // True Guard
        __ cmpl(ACC, 0);
        Label guard;
        if (cond) {
            __ j(NotEqual, &guard, false/*is_far*/);
        } else {
            __ j(Equal, &guard, false/*is_far*/);
        }

        Deoptimize(dest);
        __ Bind(&guard);
    }
}

void X64SimplifiedCodeGenerator::BackwardJumpIf(bool cond) {
    int slot = bc()->param(0);
    (void)slot;
    int delta = bc()->param(1);
    DCHECK_GE(delta, 0);
    int dest = pc() - delta;
    DCHECK_GE(dest, 0);
    
    __ cmpl(ACC, 0);
    Label guard;
    if (cond) {
        __ j(Equal, &guard, false/*is_far*/);
    } else {
        __ j(NotEqual, &guard, false/*is_far*/);
    }

    // True branch
    if (IsPCInBound(dest)) {
        int32_t asm_pc = env_->FindAsmPC(dest);
        __ jmp(asm_pc);
    } else {
        Deoptimize(dest);
    }

    // False branch
    __ Bind(&guard);
}

void X64SimplifiedCodeGenerator::CompareImplicitLengthString(Cond cond) {
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

void X64SimplifiedCodeGenerator::CallBytecodeFunction() {
    UpdatePC(); // Update current PC first

    // Adjust Caller Stack
    __ subq(rsp, RoundUp(bc()->param(1), kStackAligmentSize));

    const Function *fun = DCHECK_NOTNULL(GetNextFunctionOrNull());
    __ movq(Argv_0, ACC);

    __ movq(rbx, STATE->metadata_space()->call_bytecode_return_address());
    __ pushq(rbx); // Dummy return address
    MakeInlineStackFrame(fun);
    
    // Recover Caller Stack
    // __ addq(rsp, RoundUp(bc()->param(1), kStackAligmentSize));
}

void X64SimplifiedCodeGenerator::MakeInlineStackFrame(const Function *fun) {
    //__ Breakpoint();
    __ pushq(rbp);
    __ movq(rbp, rsp);
    __ subq(rsp, fun->stack_size());
    
    __ movl(Operand(rbp, BytecodeStackFrame::kOffsetMaker), BytecodeStackFrame::kMaker);
    // Set up pc = 0
    __ movl(Operand(rbp, BytecodeStackFrame::kOffsetPC), 0);
    // Set up callee
    __ movq(Operand(rbp, BytecodeStackFrame::kOffsetCallee), Argv_0);
    // Set up bytecode array
    __ movq(SCRATCH, reinterpret_cast<Address>(fun->bytecode()));
    __ movq(Operand(rbp, BytecodeStackFrame::kOffsetBytecodeArray), SCRATCH);
    // Set up const_pool
    __ movq(SCRATCH, reinterpret_cast<Address>(fun->const_pool()));
    __ movq(Operand(rbp, BytecodeStackFrame::kOffsetConstPool), SCRATCH);

    if (fun->exception_table_size() > 0) {
        __ InstallCaughtHandler(enable_jit_);
    }
}

Kode *GenerateSimplifiedCode(const CompilationInfo *compilation_info, bool enable_debug,
                             bool enable_jit, base::Arena *arena) {
    X64SimplifiedCodeGenerator g(compilation_info, enable_debug, enable_jit, arena);
    g.Initialize();
    g.Generate();
    Kode *code =
        compilation_info->mach()->NewCode(Code::OPTIMIZATION, 1,
                                          static_cast<uint32_t>(g.masm()->buf().size()),
                                          reinterpret_cast<const uint8_t *>(&g.masm()->buf()[0]), 0);
    code->set_slot(compilation_info->start_slot());
    return code;
}

} // namespace lang

} // namespace mai
