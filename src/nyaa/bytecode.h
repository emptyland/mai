#ifndef MAI_NYAA_BYTECODE_H_
#define MAI_NYAA_BYTECODE_H_

#include "base/arena-utils.h"
#include "base/arena.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {

namespace nyaa {

#define DECL_BYTECODES(V) \
    V(CheckStack, Type0) \
    V(LdaSmi, TypeAI) \
    V(LdaConst, TypeAB) \
    V(Ldar, TypeAB) \
    V(Star, TypeAB) \
    V(Call, TypeABC) \
    V(Return, TypeABC)

struct Bytecode {
    enum Code {
    #define DEF_ENUM(name, type) k##name,
        DECL_BYTECODES(DEF_ENUM)
    #undef DEF_ENUM
        kMaxCode,
    };

    static const char *kName[kMaxCode];
}; // struct Bytecode

using Instruction = uint32_t;

class InstructionNode {
public:
    static constexpr int32_t kMaxI24 =  0x00ffffff;
    
    InstructionNode(Bytecode::Code code) : code_(code) {}
    
    virtual Instruction Encode() const = 0;
    
    DEF_VAL_GETTER(Bytecode::Code, code);
    
    uint8_t code_byte() const { return static_cast<uint8_t>(code_); }
    
    void *operator new (size_t size, base::Arena *arena) {
        return arena->Allocate(size);
    }
    
    void operator delete (void *p) = delete;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(InstructionNode);
protected:
    static uint32_t ZigZag(int32_t val) {
        return static_cast<uint32_t>(val < 0 ? ((-val) << 1 | 1u) : val << 1);
    }

private:
    Bytecode::Code code_;
};

class InstructionNodeType0 final : public InstructionNode {
public:
    InstructionNodeType0(Bytecode::Code code) : InstructionNode(code) {}
    
    virtual Instruction Encode() const override {
        return (static_cast<uint32_t>(code()) << 24) | 0x00ffffffu;
    }

    DISALLOW_IMPLICIT_CONSTRUCTORS(InstructionNodeType0);
}; // class InstructionNodeType0

template<class T>
class InstructionNodeType1 : public InstructionNode {
public:
    InstructionNodeType1(Bytecode::Code code, T b) : InstructionNode(code), b_(b) {}
    
    DEF_VAL_GETTER(T, b);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(InstructionNodeType1);
private:
    T b_;
}; // class InstructionNodeType1

template<class T>
class InstructionNodeType2 : public InstructionNode {
public:
    InstructionNodeType2(Bytecode::Code code, T b, T c) : InstructionNode(code), b_(b), c_(c) {}
    
    DEF_VAL_GETTER(T, b);
    DEF_VAL_GETTER(T, c);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(InstructionNodeType2);
private:
    T b_;
    T c_;
}; // class InstructionNodeType2

template<class T>
class InstructionNodeType3 : public InstructionNode {
public:
    InstructionNodeType3(Bytecode::Code code, T b, T c, T d)
        : InstructionNode(code), b_(b), c_(c), d_(d) {}
    
    DEF_VAL_GETTER(T, b);
    DEF_VAL_GETTER(T, c);
    DEF_VAL_GETTER(T, d);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(InstructionNodeType3);
private:
    T b_;
    T c_;
    T d_;
}; // class InstructionNodeType3

class InstructionNodeTypeI final : public InstructionNodeType1<int32_t> {
public:
    using InstructionNode::ZigZag;
    
    InstructionNodeTypeI(Bytecode::Code code, int32_t b)
        : InstructionNodeType1<int32_t>(code, b) {
        DCHECK_LE(ZigZag(b), 0x00ffffff);
    }
    
    virtual Instruction Encode() const override {
        return (static_cast<uint32_t>(code()) << 24) | ZigZag(b());
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(InstructionNodeTypeI);
}; // class InstructionNodeTypeAI

} // namespace nyaa

} // namespace mai

#endif // MAI_NYAA_BYTECODE_H_
