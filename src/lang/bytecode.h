#pragma once
#ifndef MAI_LANG_BYTECODE_H_
#define MAI_LANG_BYTECODE_H_

#include "base/arena.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {
namespace base {
class AbstractPrinter;
} // namespace base
namespace lang {

// The byte desciption:
//
// Prefix:
//     Lda* : Load to ACC/FACC
//     Sta* : Store from ACC/FACC
// Postfix:
//     32 : 32 bits register data
//     64 : 64 bits register data
//    Ptr : Pointer size register data
//    f32 : 32 bits floating register data
//    f64 : 64 bits floating register data
// Operand kind:
//    kStackOffset : Stack offset, for access stack data
//    kConstOffset : Constant pool offset, for load constant data
//    kGlobalOffset : Global pool offset, for load/store global data
//    kCapturedVarIndex : The index of captured-variable slot, for access closure's captured-var
//    kAddressOffset : The address offset of object, for access object properties
//    kImmediate : 32 bits immediate number in instruction.
//    kCode : Some code numbers use by instruction parameter.
//


//--------------------------------------------------------------------------------------------------
// RunCoroutine: Create a coroutine and run
// Kind: TypeFA
// Params:
//     ACC: Closure type coroutine entry point
//     F(Code): Code of coroutine creation
//     A(Immediate): Params's bytes size for coroutine entry
// Return:
//     ACC: Coroutine's coid
//

//--------------------------------------------------------------------------------------------------
// NewObject: Create a user definition object
// Kind: TypeFA
// Params:
//     F(Code): Code of object creation
//     A(ConstOffset): Class pointer in constant pool offset
// Return:
//     ACC: Object pointer
//

//--------------------------------------------------------------------------------------------------
// LdaVtableFunction : Load class's method to ACC
// Kind: TypeAB
// Params:
//     A(StackOffset): Stack offset of object pointer
//     B(Immediate): Index of method
// Return:
//     ACC: Closure pointer

//--------------------------------------------------------------------------------------------------
// Close : Close stack variable and make a closure
// Kind: TypeA
// Params:
//     A(ConstOffset): Constant pool offset of Function pointer
// Return:
//     ACC: Closure pointer

#define DECLARE_ALL_BYTECODE(V) \
    DECLARE_LDAR_BYTECODE(V) \
    DECLARE_STAR_BYTECODE(V) \
    DECLARE_MOVE_BYTECODE(V) \
    DECLARE_CAST_BYTECODE(V) \
    DECLARE_ARITH_BYTECODE(V) \
    DECLARE_BITWISE_BYTECODE(V) \
    DECLARE_TEST_BYTECODE(V) \
    V(Throw, BytecodeType::N) \
    V(Yield, BytecodeType::A, BytecodeParam::kCode) \
    V(Goto, BytecodeType::AB, BytecodeParam::kCode, BytecodeParam::kImmediate) \
    V(GotoIfTrue, BytecodeType::AB, BytecodeParam::kCode, BytecodeParam::kImmediate) \
    V(GotoIfFalse, BytecodeType::AB, BytecodeParam::kCode, BytecodeParam::kImmediate) \
    V(CallBytecodeFunction, BytecodeType::A, BytecodeParam::kImmediate) \
    V(CallNativeFunction, BytecodeType::A, BytecodeParam::kImmediate) \
    V(CallFunction, BytecodeType::A, BytecodeParam::kImmediate) \
    V(RunCoroutine, BytecodeType::FA, BytecodeParam::kCode, BytecodeParam::kImmediate) \
    V(Close, BytecodeType::A, BytecodeParam::kConstOffset) \
    V(Return, BytecodeType::N) \
    V(NewBuiltinObject, BytecodeType::FA, BytecodeParam::kCode, BytecodeParam::kImmediate) \
    V(NewObject, BytecodeType::FA, BytecodeParam::kCode, BytecodeParam::kConstOffset) \
    V(CheckStack, BytecodeType::N) \
    V(AssertNotNull, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(Contact, BytecodeType::A, BytecodeParam::kImmediate)

//V(LdaArgument32, BytecodeType::A, BytecodeParam::kStackOffset) \
//V(LdaArgument64, BytecodeType::A, BytecodeParam::kStackOffset) \
//V(LdaArgumentPtr, BytecodeType::A, BytecodeParam::kStackOffset) \
//V(LdaArgumentf32, BytecodeType::A, BytecodeParam::kStackOffset) \
//V(LdaArgumentf64, BytecodeType::A, BytecodeParam::kStackOffset)

#define DECLARE_LDAR_BYTECODE(V) \
    V(Ldar32, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(Ldar64, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(LdarPtr, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(Ldaf32, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(Ldaf64, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(LdaZero, BytecodeType::N) \
    V(LdaSmi32, BytecodeType::A, BytecodeParam::kImmediate) \
    V(LdaTrue, BytecodeType::N) \
    V(LdaFalse, BytecodeType::N) \
    V(LdaCaptured32, BytecodeType::A, BytecodeParam::kCapturedVarIndex) \
    V(LdaCaptured64, BytecodeType::A, BytecodeParam::kCapturedVarIndex) \
    V(LdaCapturedPtr, BytecodeType::A, BytecodeParam::kCapturedVarIndex) \
    V(LdaCapturedf32, BytecodeType::A, BytecodeParam::kCapturedVarIndex) \
    V(LdaCapturedf64, BytecodeType::A, BytecodeParam::kCapturedVarIndex) \
    V(LdaConst32, BytecodeType::A, BytecodeParam::kConstOffset) \
    V(LdaConst64, BytecodeType::A, BytecodeParam::kConstOffset) \
    V(LdaConstPtr, BytecodeType::A, BytecodeParam::kConstOffset) \
    V(LdaConstf32, BytecodeType::A, BytecodeParam::kConstOffset) \
    V(LdaConstf64, BytecodeType::A, BytecodeParam::kConstOffset) \
    V(LdaGlobal32, BytecodeType::A, BytecodeParam::kGlobalOffset) \
    V(LdaGlobal64, BytecodeType::A, BytecodeParam::kGlobalOffset) \
    V(LdaGlobalPtr, BytecodeType::A, BytecodeParam::kGlobalOffset) \
    V(LdaGlobalf32, BytecodeType::A, BytecodeParam::kGlobalOffset) \
    V(LdaGlobalf64, BytecodeType::A, BytecodeParam::kGlobalOffset) \
    V(LdaProperty8, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kAddressOffset) \
    V(LdaProperty16, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kAddressOffset) \
    V(LdaProperty32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kAddressOffset) \
    V(LdaProperty64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kAddressOffset) \
    V(LdaPropertyPtr, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kAddressOffset) \
    V(LdaPropertyf32, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kAddressOffset) \
    V(LdaPropertyf64, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kAddressOffset) \
    V(LdaArrayAt8, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(LdaArrayAt16, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(LdaArrayAt32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(LdaArrayAt64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(LdaArrayAtPtr, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(LdaArrayAtf32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(LdaArrayAtf64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(LdaVtableFunction, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kImmediate)

#define DECLARE_STAR_BYTECODE(V) \
    V(Star32, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(Star64, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(StarPtr, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(Staf32, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(Staf64, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(StaCaptured32, BytecodeType::A, BytecodeParam::kCapturedVarIndex) \
    V(StaCaptured64, BytecodeType::A, BytecodeParam::kCapturedVarIndex) \
    V(StaCapturedPtr, BytecodeType::A, BytecodeParam::kCapturedVarIndex) \
    V(StaCapturedf32, BytecodeType::A, BytecodeParam::kCapturedVarIndex) \
    V(StaCapturedf64, BytecodeType::A, BytecodeParam::kCapturedVarIndex) \
    V(StaGlobal32, BytecodeType::A, BytecodeParam::kGlobalOffset) \
    V(StaGlobal64, BytecodeType::A, BytecodeParam::kGlobalOffset) \
    V(StaGlobalPtr, BytecodeType::A, BytecodeParam::kGlobalOffset) \
    V(StaGlobalf32, BytecodeType::A, BytecodeParam::kGlobalOffset) \
    V(StaGlobalf64, BytecodeType::A, BytecodeParam::kGlobalOffset) \
    V(StaProperty8, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kAddressOffset) \
    V(StaProperty16, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kAddressOffset) \
    V(StaProperty32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kAddressOffset) \
    V(StaProperty64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kAddressOffset) \
    V(StaPropertyPtr, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kAddressOffset) \
    V(StaPropertyf32, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kAddressOffset) \
    V(StaPropertyf64, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kAddressOffset) \
    V(StaArrayAt8, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(StaArrayAt16, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(StaArrayAt32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(StaArrayAt64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(StaArrayAtPtr, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(StaArrayAtf32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(StaArrayAtf64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \

#define DECLARE_MOVE_BYTECODE(V) \
    V(Move32,  BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(Move64,  BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(MovePtr, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \

#define DECLARE_CAST_BYTECODE(V) \
    V(Truncate32To8, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(Truncate32To16, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(Truncate64To32, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(ZeroExtend8To32, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(ZeroExtend16To32, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(ZeroExtend32To64, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(SignExtend8To32, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(SignExtend16To32, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(SignExtend32To64, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(F32ToI32, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(F64ToI32, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(F32ToU32, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(F64ToU32, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(F32ToI64, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(F64ToI64, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(F32ToU64, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(F64ToU64, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(I32ToF32, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(U32ToF32, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(I64ToF32, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(U64ToF32, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(I32ToF64, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(U32ToF64, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(I64ToF64, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(U64ToF64, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(F64ToF32, BytecodeType::A, BytecodeParam::kStackOffset) \
    V(F32ToF64, BytecodeType::A, BytecodeParam::kStackOffset)

#define DECLARE_ARITH_BYTECODE(V) \
    V(Add32,  BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(Add64,  BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(Addf32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(Addf64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(Sub32,  BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(Sub64,  BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(Subf32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(Subf64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(Mul32,  BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(Mul64,  BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(Mulf32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(Mulf64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(IMul32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(IMul64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(Div32,  BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(Div64,  BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(Divf32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(Divf64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(IDiv32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(IDiv64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(Mod32,  BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(Mod64,  BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset)

#define DECLARE_BITWISE_BYTECODE(V) \
    V(BitwiseOr32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(BitwiseOr64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(BitwiseAnd32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(BitwiseAnd64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(BitwiseNot32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(BitwiseNot64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(BitwiseShl32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(BitwiseShl64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(BitwiseShr32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(BitwiseShr64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(BitwiseLogicShr32, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kStackOffset) \
    V(BitwiseLogicShr64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset)

#define DECLARE_TEST_BYTECODE(V) \
    V(TestEqual32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(TestNotEqual32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(TestLessThan32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(TestLessThanOrEqual32, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kStackOffset) \
    V(TestGreaterThan32, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kStackOffset) \
    V(TestGreaterThanOrEqual32, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kStackOffset) \
    V(TestEqual64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(TestNotEqual64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(TestLessThan64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(TestLessThanOrEqual64, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kStackOffset) \
    V(TestGreaterThan64, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kStackOffset) \
    V(TestGreaterThanOrEqual64, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kStackOffset) \
    V(TestEqualf32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(TestNotEqualf32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(TestLessThanf32, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(TestLessThanOrEqualf32, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kStackOffset) \
    V(TestGreaterThanf32, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kStackOffset) \
    V(TestGreaterThanOrEqualf32, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kStackOffset) \
    V(TestEqualf64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(TestNotEqualf64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(TestLessThanf64, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(TestLessThanOrEqualf64, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kStackOffset) \
    V(TestGreaterThanf64, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kStackOffset) \
    V(TestGreaterThanOrEqualf64, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kStackOffset) \
    V(TestStringEqual, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(TestStringNotEqual, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kStackOffset) \
    V(TestStringLessThan, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kStackOffset) \
    V(TestStringLessThanOrEqual, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kStackOffset) \
    V(TestStringGreaterThan, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kStackOffset) \
    V(TestStringGreaterThanOrEqual, BytecodeType::AB, BytecodeParam::kStackOffset, \
      BytecodeParam::kStackOffset) \
    V(TestPtrEqual, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(TestPtrNotEqual, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(TestIn, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kStackOffset) \
    V(TestIs, BytecodeType::AB, BytecodeParam::kStackOffset, BytecodeParam::kConstOffset)

class MetadataSpace;
class Isolate;
class MacroAssembler;
class Code;

//using u8_t  = uint8_t;
//using u12_t = uint16_t;
//using u24_t = uint32_t;
using BytecodeInstruction = uint32_t;

// Yield Instruction Command Code:
constexpr uint32_t YIELD_FORCE   = 1;
constexpr uint32_t YIELD_PROPOSE = 2;
constexpr uint32_t YIELD_RANDOM  = 3;

struct BytecodeType {
    enum Kind {
        N, // No params
        A, // One 24 bits params
        FA, // Two params, F: 8 bits, A: 16 bits
        AB, // Two params, A: 12 bits, B: 12 bits
    }; // enum Kind
}; // struct BytecodeType

struct BytecodeParam {
    enum Kind {
        kNone,
        kStackOffset, // Stack offset, for access stack data
        kConstOffset, // Constant pool offset, for load constant data
        kGlobalOffset, // Global pool offset, for load/store global data
        kCapturedVarIndex, // The index of captured-variable slot, for access closure's captured-var
        kAddressOffset, // The address offset of object, for access object properties
        kImmediate, // 32 bits immediate number in instruction.
        kCode, // Some code numbers use by instruction parameter.
    }; // enum BytecodeParam
}; // struct BytecodeParam

enum BytecodeID {
#define DEFINE_ENUM(name, ...) k##name,
    DECLARE_ALL_BYTECODE(DEFINE_ENUM)
#undef DEFINE_ENUM
    kMax_Bytecodes,
}; // enum BytecodeID

static_assert(kMax_Bytecodes < 256, "Too many bytecodes!");

template<BytecodeParam::Kind A = BytecodeParam::kNone,
         BytecodeParam::Kind B = BytecodeParam::kNone,
         BytecodeParam::Kind C = BytecodeParam::kNone,
         BytecodeParam::Kind D = BytecodeParam::kNone>
struct BytecodeParamsTraits {
    static constexpr BytecodeParam::Kind k0 = A;
    static constexpr BytecodeParam::Kind k1 = B;
    static constexpr BytecodeParam::Kind k2 = C;
    static constexpr BytecodeParam::Kind k3 = D;
}; // struct BytecodeParamsTraits


template<BytecodeID ID>
struct BytecodeTraits {
    using Params = BytecodeParamsTraits<>;
    static constexpr BytecodeID kId = ID;
    static constexpr BytecodeType::Kind kType = static_cast<BytecodeType::Kind>(-1);
    static constexpr char kName[] = "";
}; // struct BytecodeTraits


#define DEFINE_BYTECODE_TRAITS(name, kind, ...) \
    template<> \
    struct BytecodeTraits<k##name> { \
        using Params = BytecodeParamsTraits<__VA_ARGS__>; \
        static constexpr BytecodeID kId = k##name; \
        static constexpr BytecodeType::Kind kType = kind; \
        static constexpr char kName[] = #name; \
    };

DECLARE_ALL_BYTECODE(DEFINE_BYTECODE_TRAITS)

#undef DEFINE_BYTECODE_TRAITS


struct BytecodeDesc {
    BytecodeID id; // Identifier of bytecode
    BytecodeType::Kind kind; // Kind of bytecode
    const char *name; // Name of bytecode
    BytecodeParam::Kind params[4]; // Parameters kind of bytecode
}; // struct BytecodeDesc

// All Bytecode Descriptions Table:
extern const BytecodeDesc kBytecodeDesc[kMax_Bytecodes];

class BytecodeNode {
public:
    static constexpr BytecodeInstruction kIDMask    = 0xff000000u;
    static constexpr BytecodeInstruction kFOfFAMask = 0x00ff0000u;
    static constexpr BytecodeInstruction kAOfAMask  = 0x00ffffffu;
    static constexpr BytecodeInstruction kAOfFAMask = 0x0000ffffu;
    static constexpr BytecodeInstruction kAOfABMask = 0x00fff000u;
    static constexpr BytecodeInstruction kBOfABMask = 0x00000fffu;
    
    static constexpr int kMaxUSmi32 = kAOfAMask;
    
    BytecodeNode(BytecodeID id, BytecodeType::Kind kind, int a, int b, int c, int d)
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
    
    void set_param(int i, int p) {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, arraysize(params_));
        params_[i] = p;
    }

    // Build instruction
    BytecodeInstruction To() const;
    
    // Parse instruction
    static BytecodeNode *From(base::Arena *arena, BytecodeInstruction instr);
    
    // Print bytecode to string
    void Print(base::AbstractPrinter *output) const;
    void PrintSimple(base::AbstractPrinter *output) const;
    
    DEF_VAL_GETTER(BytecodeID, id);
    DEF_VAL_GETTER(BytecodeType::Kind, kind);
    
    friend class BytecodeLabel;
    friend class BytecodeArrayBuilder;
    DISALLOW_IMPLICIT_CONSTRUCTORS(BytecodeNode);
private:
    void PrintParam(base::AbstractPrinter *output, BytecodeParam::Kind kind, int param) const;
    
    BytecodeID id_; // ID of bytecode
    BytecodeType::Kind kind_; // Kind of bytecode
    int params_[4]; // Max 4 parameters
}; // class BytecodeNode

template<BytecodeID ID, BytecodeType::Kind Kind = BytecodeTraits<ID>::kType>
struct Bytecodes {
    static inline BytecodeNode *New(base::Arena *arean, int a = 0, int b = 0, int c = 0, int d = 0) {
        return new (arean) BytecodeNode(ID, Kind, a, b, c, d);
    }
}; // struct BytecodeBuilder

template<BytecodeID ID>
struct Bytecodes<ID, BytecodeType::N> {
    static inline BytecodeNode *New(base::Arena *arean) {
        return new (arean) BytecodeNode(ID, BytecodeType::N, 0, 0, 0, 0);
    }
}; // struct BytecodeBuilder

template<BytecodeID ID>
struct Bytecodes<ID, BytecodeType::A> {
    static inline BytecodeNode *New(base::Arena *arean, int a) {
        return new (arean) BytecodeNode(ID, BytecodeType::A, a, 0, 0, 0);
    }
}; // struct BytecodeBuilder

template<BytecodeID ID>
struct Bytecodes<ID, BytecodeType::FA> {
    static inline BytecodeNode *New(base::Arena *arean, int f, int a) {
        DCHECK_GE(f, 0);
        DCHECK_LT(f, 0x100);
        return new (arean) BytecodeNode(ID, BytecodeType::FA, f, a, 0, 0);
    }
}; // struct BytecodeBuilder

template<BytecodeID ID>
struct Bytecodes<ID, BytecodeType::AB> {
    static inline BytecodeNode *New(base::Arena *arean, int a, int b) {
        return new (arean) BytecodeNode(ID, BytecodeType::AB, a, b, 0, 0);
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
