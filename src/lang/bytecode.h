#pragma once
#ifndef MAI_LANG_BYTECODE_H_
#define MAI_LANG_BYTECODE_H_

#include "base/arena.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {

namespace lang {


#define DECLARE_ALL_BYTECODE(V) \
    DECLARE_LDAR_BYTECODE(V) \
    DECLARE_STAR_BYTECODE(V) \
    DECLARE_MOVE_BYTECODE(V) \
    DECLARE_CAST_BYTECODE(V) \
    DECLARE_ARITH_BYTECODE(V) \
    DECLARE_BITWISE_BYTECODE(V) \
    DECLARE_TEST_BYTECODE(V) \
    V(Throw, N) \
    V(Yield, A, u24) \
    V(Goto, A, u24) \
    V(GotoIfTrue, A, u24) \
    V(GotoIfFalse, A, u24) \
    V(CallBytecodeFunction, A, u24) \
    V(CallNativeFunction, A, u24) \
    V(CallVtableFunction, A, u24) \
    V(Return, N) \
    V(NewBuiltinObject, FA, u8, u16) \
    V(CheckStack, N)

#define DECLARE_LDAR_BYTECODE(V) \
    V(Ldar32,          A, u24) \
    V(Ldar64,          A, u24) \
    V(LdarPtr,         A, u24) \
    V(Ldaf32,          A, u24) \
    V(Ldaf64,          A, u24) \
    V(LdaZero,         N) \
    V(LdaSmi32,        A, u24) \
    V(LdaTrue,         N) \
    V(LdaFalse,        N) \
    V(Ldak32,          A, u24) \
    V(Ldak64,          A, u24) \
    V(Ldakf32,         A, u24) \
    V(Ldakf64,         A, u24) \
    V(LdaGlobal32,     A, u24) \
    V(LdaGlobal64,     A, u24) \
    V(LdaGlobalPtr,    A, u24) \
    V(LdaGlobalf32,    A, u24) \
    V(LdaGlobalf64,    A, u24) \
    V(LdaProperty8,    AB, u12, u12) \
    V(LdaProperty16,   AB, u12, u12) \
    V(LdaProperty32,   AB, u12, u12) \
    V(LdaProperty64,   AB, u12, u12) \
    V(LdaPropertyPtr,  AB, u12, u12) \
    V(LdaPropertyf32,  AB, u12, u12) \
    V(LdaPropertyf64,  AB, u12, u12) \
    V(LdaArrayElem8,   AB, u12, u12) \
    V(LdaArrayElem16,  AB, u12, u12) \
    V(LdaArrayElem32,  AB, u12, u12) \
    V(LdaArrayElem64,  AB, u12, u12) \
    V(LdaArrayElemPtr, AB, u12, u12) \
    V(LdaArrayElemf32, AB, u12, u12) \
    V(LdaArrayElemf64, AB, u12, u12)

#define DECLARE_STAR_BYTECODE(V) \
    V(Star32,          A, u24) \
    V(Star64,          A, u24) \
    V(StarPtr,         A, u24) \
    V(Staf32,          A, u24) \
    V(Staf64,          A, u24) \
    V(StaProperty8,    AB, u12, u12) \
    V(StaProperty16,   AB, u12, u12) \
    V(StaProperty32,   AB, u12, u12) \
    V(StaProperty64,   AB, u12, u12) \
    V(StaPropertyPtr,  AB, u12, u12) \
    V(StaPropertyf32,  AB, u12, u12) \
    V(StaPropertyf64,  AB, u12, u12) \
    V(StaArrayElem8,   AB, u12, u12) \
    V(StaArrayElem16,  AB, u12, u12) \
    V(StaArrayElem32,  AB, u12, u12) \
    V(StaArrayElem64,  AB, u12, u12) \
    V(StaArrayElemPtr, AB, u12, u12) \
    V(StaArrayElemf32, AB, u12, u12) \
    V(StaArrayElemf64, AB, u12, u12)

#define DECLARE_MOVE_BYTECODE(V) \
    V(Move32,  AB, u12, u12) \
    V(Move64,  AB, u12, u12) \
    V(MovePtr, AB, u12, u12)

#define DECLARE_CAST_BYTECODE(V) \
    V(Truncate32To8,    A, u24) \
    V(Truncate32To16,   A, u24) \
    V(Truncate64To32,   A, u24) \
    V(ZeroExtend8To32,  A, u24) \
    V(ZeroExtend16To32, A, u24) \
    V(ZeroExtend32To64, A, u24) \
    V(SignExtend8To32,  A, u24) \
    V(SignExtend16To32, A, u24) \
    V(SignExtend32To64, A, u24) \
    V(F32ToI32,         A, u24) \
    V(F64ToI32,         A, u24) \
    V(F32ToU32,         A, u24) \
    V(F64ToU32,         A, u24) \
    V(F32ToI64,         A, u24) \
    V(F64ToI64,         A, u24) \
    V(F32ToU64,         A, u24) \
    V(F64ToU64,         A, u24) \
    V(I32ToF32,         A, u24) \
    V(U32ToF32,         A, u24) \
    V(I64ToF32,         A, u24) \
    V(U64ToF32,         A, u24) \
    V(I32ToF64,         A, u24) \
    V(U32ToF64,         A, u24) \
    V(I64ToF64,         A, u24) \
    V(U64ToF64,         A, u24) \
    V(F64ToF32,         A, u24) \
    V(F32ToF64,         A, u24)

#define DECLARE_ARITH_BYTECODE(V) \
    V(Add32,  AB, u12, u12) \
    V(Add64,  AB, u12, u12) \
    V(Addf32, AB, u12, u12) \
    V(Addf64, AB, u12, u12) \
    V(Sub32,  AB, u12, u12) \
    V(Sub64,  AB, u12, u12) \
    V(Subf32, AB, u12, u12) \
    V(Subf64, AB, u12, u12) \
    V(Mul32,  AB, u12, u12) \
    V(Mul64,  AB, u12, u12) \
    V(Mulf32, AB, u12, u12) \
    V(Mulf64, AB, u12, u12) \
    V(IMul32, AB, u12, u12) \
    V(IMul64, AB, u12, u12) \
    V(Div32,  AB, u12, u12) \
    V(Div64,  AB, u12, u12) \
    V(Divf32, AB, u12, u12) \
    V(Divf64, AB, u12, u12) \
    V(IDiv32, AB, u12, u12) \
    V(IDiv64, AB, u12, u12) \
    V(Mod32,  AB, u12, u12) \
    V(Mod64,  AB, u12, u12) \

#define DECLARE_BITWISE_BYTECODE(V) \
    V(BitwiseOr32,       AB, u12, u12) \
    V(BitwiseOr64,       AB, u12, u12) \
    V(BitwiseAnd32,      AB, u12, u12) \
    V(BitwiseAnd64,      AB, u12, u12) \
    V(BitwiseNot32,      AB, u12, u12) \
    V(BitwiseNot64,      AB, u12, u12) \
    V(BitwiseShl32,      AB, u12, u12) \
    V(BitwiseShl64,      AB, u12, u12) \
    V(BitwiseShr32,      AB, u12, u12) \
    V(BitwiseShr64,      AB, u12, u12) \
    V(BitwiseLogicShr32, AB, u12, u12) \
    V(BitwiseLogicShr64, AB, u12, u12)

#define DECLARE_TEST_BYTECODE(V) \
    V(TestEqual32,                AB, u12, u12) \
    V(TestNotEqual32,             AB, u12, u12) \
    V(TestLessThan32,             AB, u12, u12) \
    V(TestLessThanOrEqual32,      AB, u12, u12) \
    V(TestGreaterThan32,          AB, u12, u12) \
    V(TestGreaterThanEqual32,     AB, u12, u12) \
    V(TestEqual64,                AB, u12, u12) \
    V(TestNotEqual64,             AB, u12, u12) \
    V(TestLessThan64,             AB, u12, u12) \
    V(TestLessThanOrEqual64,      AB, u12, u12) \
    V(TestGreaterThan64,          AB, u12, u12) \
    V(TestGreaterThanEqual64,     AB, u12, u12) \
    V(TestEqualf32,               AB, u12, u12) \
    V(TestNotEqualf32,            AB, u12, u12) \
    V(TestLessThanf32,            AB, u12, u12) \
    V(TestLessThanOrEqualf32,     AB, u12, u12) \
    V(TestGreaterThanf32,         AB, u12, u12) \
    V(TestGreaterThanEqualf32,    AB, u12, u12) \
    V(TestEqualf64,               AB, u12, u12) \
    V(TestNotEqualf64,            AB, u12, u12) \
    V(TestLessThanf64,            AB, u12, u12) \
    V(TestLessThanOrEqualf64,     AB, u12, u12) \
    V(TestGreaterThanf64,         AB, u12, u12) \
    V(TestGreaterThanEqualf64,    AB, u12, u12) \
    V(TestStringEqual,            AB, u12, u12) \
    V(TestStringNotEqual,         AB, u12, u12) \
    V(TestStringLessThan,         AB, u12, u12) \
    V(TestStringLessThanOrEqual,  AB, u12, u12) \
    V(TestStringGreaterThan,      AB, u12, u12) \
    V(TestStringGreaterThanEqual, AB, u12, u12) \
    V(TestPtrEqual,               AB, u12, u12) \
    V(TestPtrNotEqual,            AB, u12, u12) \
    V(TestIn,                     AB, u12, u12) \
    V(TestIs,                     AB, u12, u12)

class MetadataSpace;
class Isolate;
class MacroAssembler;
class Code;

//using u8_t  = uint8_t;
//using u12_t = uint16_t;
//using u24_t = uint32_t;
using BytecodeInstruction = uint32_t;

enum BytecodeKind {
    kBytecode_TypeN,
    kBytecode_TypeA,
    kBytecode_TypeFA,
    kBytecode_TypeAB,
}; // enum BytecodeKind

enum BytecodeID {
#define DEFINE_ENUM(name, ...) k##name,
    DECLARE_ALL_BYTECODE(DEFINE_ENUM)
#undef DEFINE_ENUM
    kMax_Bytecodes,
}; // enum BytecodeID


template<BytecodeID ID>
struct BytecodeTraits {
    static constexpr BytecodeID kId = ID;
    static constexpr BytecodeKind kType = static_cast<BytecodeKind>(-1);
    static constexpr char kName[] = "";
}; // struct BytecodeTraits


#define DEFINE_BYTECODE_TRAITS(name, kind, ...) \
    template<> \
    struct BytecodeTraits<k##name> { \
        static constexpr BytecodeID kId = k##name; \
        static constexpr BytecodeKind kType = kBytecode_Type##kind; \
        static constexpr char kName[] = #name; \
    };

DECLARE_ALL_BYTECODE(DEFINE_BYTECODE_TRAITS)

#undef DEFINE_BYTECODE_TRAITS


class BytecodeNode {
public:
    static constexpr BytecodeInstruction kIDMask    = 0xff000000u;
    static constexpr BytecodeInstruction kFOfFAMask = 0x00ff0000u;
    static constexpr BytecodeInstruction kAOfAMask  = 0x00ffffffu;
    static constexpr BytecodeInstruction kAOfFAMask = 0x0000ffffu;
    static constexpr BytecodeInstruction kAOfABMask = 0x00fff000u;
    static constexpr BytecodeInstruction kBOfABMask = 0x00000fffu;
    
    BytecodeNode(BytecodeID id, BytecodeKind kind, int a, int b, int c, int d)
        : id_(id)
        , kind_(kind) {
        params_[0] = a;
        params_[1] = b;
        params_[2] = c;
        params_[3] = d;
    }

    void *operator new (size_t size, base::Arena *arean) {
        return arean->Allocate(size);
    }

    int param(int i) const {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, arraysize(params_));
        return params_[i];
    }
    
    BytecodeInstruction To() const;
    static BytecodeNode *From(base::Arena *arena, BytecodeInstruction instr);
    
    DEF_VAL_GETTER(BytecodeID, id);
    DEF_VAL_GETTER(BytecodeKind, kind);
    
    friend class BytecodeLabel;
    friend class BytecodeArrayBuilder;
    DISALLOW_IMPLICIT_CONSTRUCTORS(BytecodeNode);
private:
    BytecodeID id_; // ID of bytecode
    BytecodeKind kind_; // Kind of bytecode
    int params_[4]; // Max 4 parameters
}; // class BytecodeNode

template<BytecodeID ID, BytecodeKind Kind = BytecodeTraits<ID>::kType>
struct Bytecodes {
    static inline BytecodeNode *New(base::Arena *arean, int a = 0, int b = 0, int c = 0, int d = 0) {
        return new (arean) BytecodeNode(ID, Kind, a, b, c, d);
    }
}; // struct BytecodeBuilder

template<BytecodeID ID>
struct Bytecodes<ID, kBytecode_TypeN> {
    static inline BytecodeNode *New(base::Arena *arean) {
        return new (arean) BytecodeNode(ID, kBytecode_TypeN, 0, 0, 0, 0);
    }
}; // struct BytecodeBuilder

template<BytecodeID ID>
struct Bytecodes<ID, kBytecode_TypeA> {
    static inline BytecodeNode *New(base::Arena *arean, int a) {
        return new (arean) BytecodeNode(ID, kBytecode_TypeA, a, 0, 0, 0);
    }
}; // struct BytecodeBuilder

template<BytecodeID ID>
struct Bytecodes<ID, kBytecode_TypeFA> {
    static inline BytecodeNode *New(base::Arena *arean, int f, int a) {
        DCHECK_GE(f, 0);
        DCHECK_LT(f, 0x100);
        return new (arean) BytecodeNode(ID, kBytecode_TypeFA, f, a, 0, 0);
    }
}; // struct BytecodeBuilder

template<BytecodeID ID>
struct Bytecodes<ID, kBytecode_TypeAB> {
    static inline BytecodeNode *New(base::Arena *arean, int a, int b) {
        return new (arean) BytecodeNode(ID, kBytecode_TypeAB, a, b, 0, 0);
    }
}; // struct BytecodeBuilder

class AbstractBytecodeEmitter {
public:
    AbstractBytecodeEmitter() {}
    virtual ~AbstractBytecodeEmitter();
    
#define DEFINE_METHOD(name, ...) \
    virtual void Emit##name(MacroAssembler *masm) = 0;
    DECLARE_ALL_BYTECODE(DEFINE_METHOD)
#undef DEFINE_METHOD
    
    static AbstractBytecodeEmitter *New(MetadataSpace *space);

    DISALLOW_IMPLICIT_CONSTRUCTORS(AbstractBytecodeEmitter);
}; // class AbstractBytecodeEmitter

} // namespace lang

} // namespace mai

#endif // MAI_LANG_BYTECODE_H_
