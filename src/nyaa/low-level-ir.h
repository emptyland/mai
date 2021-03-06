#ifndef MAI_NYAA_LOW_LEVEL_IR_H_
#define MAI_NYAA_LOW_LEVEL_IR_H_

#include "base/arena-utils.h"
#include "base/arena.h"
#include "base/slice.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {
    
namespace lir {
    
#define DECL_LIR_CODE(V) \
    DECL_COMMON_LIR_CODE(V) \
    DECL_ARCH_LIR_CODE(V)
    
#define DECL_COMMON_LIR_CODE(V) \
    V(InboxSmi) \
    V(InboxFloat64) \
    V(SmiAdd) \
    V(SmiSub) \
    V(SmiMul) \
    V(SmiDiv) \
    V(SmiMod) \
    V(MoveBaseOfStack) \
    V(Constant) \
    V(SaveCallerRegisters) \
    V(SaveCallerPartialRegisters) \
    V(CallNative) \
    V(RestoreCallerRegisters) \
    V(RestoreCallerPartialRegisters) \
    V(Breakpoint) \
    V(Branch) \
    V(Jump)

#define DECL_ARCH_LIR_CODE(V) \
    V(Move) \
    V(Add) \
    V(Add32) \
    V(Sub) \
    V(Sub32) \
    V(Mul) \
    V(Mul32) \
    V(Div) \
    V(Div32) \
    V(Mod) \
    V(Mod32) \
    V(Cmp) \
    V(Cmp32) \
    V(Ret)
    
class Instruction;
class Operand;
class UnallocatedOperand;
class MemoryOperand;
class RegisterOperand;
class FPRegisterOperand;
class Function;
class Block;
    
enum IRCode {
#define DEFINE_ENUM(name, ...) k##name,
    DECL_LIR_CODE(DEFINE_ENUM)
#undef DEFINE_ENUM
    kMaxIRCodes,
}; // enum IRCode
    
enum IRComparator {
    kEqual,
    kNotEqual,
    kLessThan,
    kLessThanOrEqual,
    kGreaterThan,
    kGreaterThanOrEqual,
    kOverflow,
    kNotOverflow,
};
    
enum JumpPreference {
    kNotSpecial,
    kLikely0,
    kLikely1,
    kUnlikely0,
    kUnlikely1,
};
    
extern const char *kIRCodeNames[kMaxIRCodes];

struct Architecture final {
    static constexpr int kMaxStackSlots = 32;

#if defined(MAI_ARCH_X64)
    static constexpr int kMaxRegisters = 16;
    static constexpr int kMaxArgsRegisters = 8;
    static constexpr int kMaxFPRegisters = 16;
    static constexpr int kMaxAllocatableRegisters = 10;
    
    static constexpr int kVMBPCode = 13; // r13
#endif // defined(MAI_ARCH_X64)
    
    static const RegisterOperand *kAllRegisters[kMaxRegisters];
    static const RegisterOperand *kArgvRegisters[kMaxArgsRegisters];
    static const FPRegisterOperand *kAllFPRegisters[kMaxFPRegisters];
    static const RegisterOperand *kAllocatableRegisters[kMaxAllocatableRegisters];
    static const MemoryOperand *kLowStackSlots[kMaxStackSlots];
    
    static const char *kRegisterNames[kMaxRegisters];
    static const char *kFPRegisterNames[kMaxFPRegisters];

    DISALLOW_ALL_CONSTRUCTORS(Architecture);
}; // struct Platform

class LIRNode {
public:
    void *operator new (size_t size, base::Arena *arena) {
        return arena->Allocate(size);
    }

    void operator delete (void *p) = delete;
}; // class LIRNode

class Operand : public LIRNode {
public:
    enum Kind {
        kUnallocated,
        kLabel,
        kImmediate,
        kMemory,
        kRegister,
        kFPRegister,
    };
    constexpr explicit Operand(Kind kind) : kind_(kind) {}
    
    virtual void PrintTo(FILE *fp) const = 0;

    DEF_VAL_GETTER(Kind, kind);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Operand);
private:
    Kind kind_;
}; // class Operand

class UnallocatedOperand final : public Operand {
public:
    enum Policy {
        kNone,
        kRegisterOrSlot,
        kRegisterOrSlotOrConstant,
        kFixedRegister,
        kFixedFPRegister,
        kMustHasRegister,
        kMustHasSlot,
    };
    
    explicit UnallocatedOperand(int vid, int reserved, Policy policy)
        : Operand(kUnallocated)
        , vid_(vid)
        , reserved_(reserved)
        , policy_(policy) {}

    static UnallocatedOperand *New(int vid, Policy policy, base::Arena *arena) {
        return new (arena) UnallocatedOperand(vid, 1, policy);
    }
    
    static UnallocatedOperand *New(int vid, int reserved, Policy policy, base::Arena *arena) {
        return new (arena) UnallocatedOperand(vid, reserved, policy);
    }
    
    DEF_VAL_GETTER(int, vid);
    DEF_VAL_GETTER(Policy, policy);
    DEF_VAL_GETTER(int, reserved);

    virtual void PrintTo(FILE *fp) const override;
private:
    int vid_;
    int reserved_;
    Policy policy_;
}; // class UnallocatedOperand
    
class ImmediateOperand : public Operand {
public:
    ImmediateOperand(int64_t value, int bits)
        : Operand(kImmediate)
        , bits_(bits)
        , value_(value) {}
    
    ImmediateOperand(Address addr)
        : Operand(kImmediate)
        , bits_(0)
        , addr_(addr) {}
    
    DEF_VAL_GETTER(int, bits);
    int8_t i1_value() const { return !!static_cast<int8_t>(value_); }
    int8_t i8_value() const { return static_cast<int8_t>(value_); }
    int16_t i16_value() const { return static_cast<int16_t>(value_); }
    int32_t i32_value() const { return static_cast<int32_t>(value_); }
    int64_t i64_value() const { return value_; }
    Address addr_value() const { return addr_; }

    virtual void PrintTo(FILE *fp) const override;

    static const ImmediateOperand *Ensure(const Operand *op) {
        DCHECK_EQ(kImmediate, op->kind());
        return static_cast<const ImmediateOperand *>(op);
    }

    static ImmediateOperand *Ensure(Operand *op) {
        DCHECK_EQ(kImmediate, op->kind());
        return static_cast<ImmediateOperand *>(op);
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ImmediateOperand);
private:
    int bits_;
    union {
        int64_t value_;
        Address addr_;
    };
}; // class ImmediateOperand

class RegisterOperand : public Operand {
public:
    constexpr RegisterOperand(int code)
        : Operand(kRegister)
        , code_(code) {}

    DEF_VAL_GETTER(int, code);
    
    virtual void PrintTo(FILE *fp) const override {
        ::fprintf(fp, "%s", Architecture::kRegisterNames[code_]);
    }
    
    static const RegisterOperand *Ensure(const Operand *op) {
        DCHECK_EQ(kRegister, op->kind());
        return static_cast<const RegisterOperand *>(op);
    }
    
    static RegisterOperand *Ensure(Operand *op) {
        DCHECK_EQ(kRegister, op->kind());
        return static_cast<RegisterOperand *>(op);
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(RegisterOperand);
private:
    int code_;
}; // class RegisterOperand
    
class FPRegisterOperand : public Operand {
public:
    constexpr FPRegisterOperand(int code)
        : Operand(kFPRegister)
        , code_(code) {}
    
    DEF_VAL_GETTER(int, code);
    
    virtual void PrintTo(FILE *fp) const override {
        ::fprintf(fp, "%s", Architecture::kFPRegisterNames[code_]);
    }
    
    static const FPRegisterOperand *Ensure(const Operand *op) {
        DCHECK_EQ(kFPRegister, op->kind());
        return static_cast<const FPRegisterOperand *>(op);
    }
    
    static FPRegisterOperand *Ensure(Operand *op) {
        DCHECK_EQ(kFPRegister, op->kind());
        return static_cast<FPRegisterOperand *>(op);
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(FPRegisterOperand);
private:
    int code_;
}; // class FPRegisterOperand
    
class MemoryOperand : public Operand {
public:
    MemoryOperand(const RegisterOperand &base, int offset)
        : Operand(kMemory)
        , base_(base.code())
        , offset_(offset) {}
    
    constexpr MemoryOperand(int base, int offset)
        : Operand(kMemory)
        , base_(base)
        , offset_(offset) {}
    
    DEF_VAL_GETTER(int, base);
    DEF_VAL_GETTER(int, offset);

    const RegisterOperand &BaseRegister() const {
        return *Architecture::kAllRegisters[base_];
    }
    
    virtual void PrintTo(FILE *fp) const override {
        ::fprintf(fp, "%x(%s)", offset_, Architecture::kFPRegisterNames[base_]);
    }

    static const MemoryOperand *Ensure(const Operand *op) {
        DCHECK_EQ(kMemory, op->kind());
        return static_cast<const MemoryOperand *>(op);
    }
    
    static MemoryOperand *Ensure(Operand *op) {
        DCHECK_EQ(kMemory, op->kind());
        return static_cast<MemoryOperand *>(op);
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(MemoryOperand);
private:
    int base_;
    int offset_;
}; // class RegisterOperand
    
    
class Block final : public Operand {
public:
    using InstrList = base::ArenaVector<Instruction *>;
    
    DEF_VAL_GETTER(int, label);
    
    InstrList::const_iterator instr_begin() const { return instrs_.cbegin(); }
    InstrList::const_iterator instr_end() const { return instrs_.cend(); }
    Instruction *instr_last() const { return instrs_.back(); }
    size_t instrs_size() const { return instrs_.size(); }
    Instruction *instr(size_t i) const { return instrs_[i]; }
    
    void Add(Instruction *instr) { instrs_.push_back(instr); }
    
    inline void PrintAll(FILE *fp) const;

    virtual void PrintTo(FILE *fp) const override { ::fprintf(fp, "l%d", label_); }
    
    static Block *New(int label, base::Arena *arena) { return new (arena) Block(label, arena); }

    static const Block *Ensure(const Operand *op) {
        DCHECK_EQ(kLabel, op->kind());
        return static_cast<const Block *>(op);
    }
    
    static Block *Ensure(Operand *op) {
        DCHECK_EQ(kLabel, op->kind());
        return static_cast<Block *>(op);
    }
private:
    Block(int label, base::Arena *arena)
        : Operand(kLabel)
        , label_(label)
        , instrs_(arena) {}
    
    int label_;
    InstrList instrs_;
}; // class Block


class Instruction final {
public:
    DEF_VAL_GETTER(IRCode, op);
    DEF_VAL_GETTER(int, subcode);
    DEF_VAL_PROP_RW(int, hint);
    DEF_VAL_GETTER(int, line);
    DEF_VAL_GETTER(int, n_inputs);
    DEF_PTR_PROP_RW(const Operand, temp);
    DEF_PTR_PROP_RW(const Operand, output);

    const Operand *input(int i) const {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, n_inputs_);
        return input_[i];
    }
    
    void set_input(int i, const Operand *operand) {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, n_inputs_);
        input_[i] = operand;
    }
    
    void PrintTo(FILE *fp);

    static Instruction *New(IRCode op, const Operand *dst, int line, base::Arena *arena) {
        void *chunk = arena->Allocate(RequiredSize(0));
        Instruction *instr = new (chunk) Instruction(op, 0, dst, 0, line);
        return instr;
    }

    static Instruction *New(IRCode op, const Operand *dst, const Operand *src, int line,
                            base::Arena *arena) {
        void *chunk = arena->Allocate(RequiredSize(1));
        Instruction *instr = new (chunk) Instruction(op, 0, dst, 1, line);
        instr->input_[0] = src;
        return instr;
    }
    
    static Instruction *New(IRCode op, const Operand *dst, const Operand *lhs, const Operand *rhs,
                            int line, base::Arena *arena) {
        void *chunk = arena->Allocate(RequiredSize(2));
        Instruction *instr = new (chunk) Instruction(op, 0, dst, 2, line);
        instr->input_[0] = lhs;
        instr->input_[1] = rhs;
        return instr;
    }
    
    static Instruction *New(IRCode op, int subcode, const Operand *dst, int n_inputs, int line,
                            base::Arena *arena) {
        void *chunk = arena->Allocate(RequiredSize(n_inputs));
        return new (chunk) Instruction(op, subcode, dst, n_inputs, line);
    }
    
    static size_t RequiredSize(int n_inputs) {
        return sizeof(Instruction) + sizeof(Operand *) * ((n_inputs < 2) ? 0 : n_inputs - 1);
    }

    DISALLOW_IMPLICIT_CONSTRUCTORS(Instruction);
private:
    Instruction(IRCode op, int subcode, const Operand *output, int n_inputs, int line)
        : op_(op)
        , subcode_(subcode)
        , line_(line)
        , n_inputs_(n_inputs)
        , output_(output) {
        for (int i = 0; i < n_inputs; ++i) {
            input_[i] = nullptr;
        }
    }

    IRCode op_;
    int subcode_;
    int line_;
    int n_inputs_;
    int hint_ = 0;
    const Operand *output_;
    const Operand *temp_ = nullptr;
    const Operand *input_[1];
}; // class Instruction
    
class Function final : public LIRNode {
public:
    using BlockList = base::ArenaVector<Block *>;
    using ParameterList = base::ArenaVector<const Operand *>;

    DEF_VAL_GETTER(BlockList, blocks);
    DEF_VAL_GETTER(ParameterList, parameters);
    
    void AddParameter(const Operand *param) { parameters_.push_back(param); }

    void PrintAll(FILE *fp) const {
        for (auto block : blocks_) {
            ::fprintf(fp, "l%d\n", block->label());
            block->PrintAll(fp);
        }
    }

    Block *NewBlock(int label) {
        Block *blk = Block::New(label, arena_);
        blocks_.push_back(blk);
        return blk;
    }
    
    void AddBlock(Block *block) { blocks_.push_back(block); }
    
    static Function *New(base::Arena *arena) { return new (arena) Function(arena); }
private:
    Function(base::Arena *arena)
        : arena_(arena)
        , parameters_(arena)
        , blocks_(arena) {}
    
    ParameterList parameters_;
    BlockList blocks_;
    base::Arena *arena_;
}; // class InstructionBundle
    
inline void Block::PrintAll(FILE *fp) const {
    for (int64_t i = 0; i < instrs_.size(); ++i) {
        ::fprintf(fp, "    %04" PRIx64 " ", i);
        instrs_[i]->PrintTo(fp);
        ::fprintf(fp, "; line = %d\n", instrs_[i]->line());
    }
}

} // namespace lir
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_LOW_LEVEL_IR_H_
