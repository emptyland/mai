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
    V(MoveMem2Reg, Memory, Register) \
    V(MoveReg2Mem, Register, Memory) \
    V(MoveFP2Mem, FPRegister, Memory) \
    V(MoveMem2FP, Memory, FPRegister) \
    V(Jump, Block)
    
class Instruction;
class Operand;
class MemoryOperand;
class RegisterOperand;
class FPRegisterOperand;
class InstructionBundle;
class Block;
    
enum IRCode {
#define DEFINE_ENUM(name, ...) k##name,
    DECL_LIR_CODE(DEFINE_ENUM)
#undef DEFINE_ENUM
}; // enum IRCode

struct Architecture final {

#if defined(MAI_ARCH_X64)
    static constexpr int kMaxRegisters = 16;
    static constexpr int kMaxFPRegisters = 16;
    static constexpr int kMaxAllocatableRegisters = 10;
#endif // defined(MAI_ARCH_X64)
    
    static const RegisterOperand *kAllRegisters[kMaxRegisters];
    static const FPRegisterOperand *kAllFPRegisters[kMaxFPRegisters];
    static const RegisterOperand *kAllocatableRegisters[kMaxAllocatableRegisters];
    
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
    
class Operand {
public:
    enum Kind {
        kLabel,
        kMemory,
        kRegister,
        kFPRegister,
    };
    constexpr explicit Operand(Kind kind) : kind_(kind) {}

    DEF_VAL_GETTER(Kind, kind);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Operand);
private:
    Kind kind_;
}; // class Operand

class RegisterOperand : public Operand {
public:
    constexpr RegisterOperand(int code)
        : Operand(kRegister)
        , code_(code) {}

    DEF_VAL_GETTER(int, code);
    
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
    MemoryOperand(int base, int offset)
        : Operand(kMemory)
        , base_(base)
        , offset_(offset) {}
    
    DEF_VAL_GETTER(int, base);
    DEF_VAL_GETTER(int, offset);
    
    void *operator new (size_t size, base::Arena *arena) {
        return arena->Allocate(size);
    }
    
    const RegisterOperand &BaseRegister() const {
        return *Architecture::kAllRegisters[base_];
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

    Block(int label, base::Arena *arena)
        : Operand(kLabel)
        , label_(label)
        , instrs_(arena) {}
    
    DEF_VAL_GETTER(int, label);
    
    InstrList::const_iterator instr_begin() const { return instrs_.cbegin(); }
    InstrList::const_iterator instr_end() const { return instrs_.cend(); }
    size_t instrs_size() const { return instrs_.size(); }
    Instruction *instr(size_t i) { return instrs_[i]; }
    
    void Add(Instruction *instr) { instrs_.push_back(instr); }

    static const Block *Ensure(const Operand *op) {
        DCHECK_EQ(kLabel, op->kind());
        return static_cast<const Block *>(op);
    }
    
    static Block *Ensure(Operand *op) {
        DCHECK_EQ(kLabel, op->kind());
        return static_cast<Block *>(op);
    }
private:
    int label_;
    InstrList instrs_;
}; // class Block


class Instruction final {
public:
    DEF_VAL_GETTER(IRCode, op);
    DEF_VAL_GETTER(int, n_inputs);
    DEF_PTR_GETTER(const Operand, output);

    const Operand *input(int i) const {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, n_inputs_);
        return input_[i];
    }

    const MemoryOperand &MemoryOutput() const {
        return *MemoryOperand::Ensure(DCHECK_NOTNULL(output_));
    }

    const FPRegisterOperand &FPRegisterOutput() const {
        return *FPRegisterOperand::Ensure(DCHECK_NOTNULL(output_));
    }

    const RegisterOperand &RegisterOutput() const {
        return *RegisterOperand::Ensure(DCHECK_NOTNULL(output_));
    }

    const Block &LabelOutput() const { return *Block::Ensure(DCHECK_NOTNULL(output_)); }

    const MemoryOperand &MemoryInput(int i) const { return *MemoryOperand::Ensure(input(i)); }

    const FPRegisterOperand &FPRegisterInput(int i) const {
        return *FPRegisterOperand::Ensure(input(i));
    }

    const RegisterOperand &RegisterInput(int i) const {
        return *RegisterOperand::Ensure(input(i));
    }
    
    static Instruction *New(IRCode op, const Operand *dst, base::Arena *arena) {
        void *chunk = arena->Allocate(sizeof(Instruction));
        Instruction *instr = new (chunk) Instruction(op, dst, 0);
        return instr;
    }

    static Instruction *New(IRCode op, const Operand *dst, const Operand *src, base::Arena *arena) {
        void *chunk = arena->Allocate(sizeof(Instruction));
        Instruction *instr = new (chunk) Instruction(op, dst, 1);
        instr->input_[0] = src;
        return instr;
    }
    
    static Instruction *New(IRCode op, const Operand *dst, const Operand *lhs, const Operand *rhs,
                            base::Arena *arena) {
        void *chunk = arena->Allocate(sizeof(Instruction) + sizeof(rhs));
        Instruction *instr = new (chunk) Instruction(op, dst, 2);
        instr->input_[0] = lhs;
        instr->input_[1] = rhs;
        return instr;
    }

    DISALLOW_IMPLICIT_CONSTRUCTORS(Instruction);
private:
    Instruction(IRCode op, const Operand *output, int n_inputs);
    
    IRCode op_;
    int n_inputs_;
    const Operand *const output_;
    const Operand *input_[1];
}; // class Instruction
    
class InstructionBundle final {
public:
    using BlockList = base::ArenaVector<Block *>;
    
    InstructionBundle(base::Arena *arena)
        : arena_(arena)
        , blocks_(arena) {}

    Block *NewBlock(int label) {
        Block *blk = new (arena_) Block(label, arena_);
        blocks_.push_back(blk);
        return blk;
    }
private:
    base::Arena *arena_;
    BlockList blocks_;
}; // class InstructionBundle

} // namespace lir
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_LOW_LEVEL_IR_H_
