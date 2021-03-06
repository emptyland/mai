#ifndef MAI_NYAA_BUILTIN_H_
#define MAI_NYAA_BUILTIN_H_

#include "base/base.h"
#include "mai/error.h"

namespace mai {
    
namespace nyaa {

namespace hir {
class Value;
}; // namespace dag
    
class Object;
class NyString;
class NyMap;
class NyTable;
class NyCode;
class NyaaCore;

template<class T> class FunctionCallbackInfo;
class Nyaa;
    
#define DECL_BUILTIN_TYPES(V) \
    V(Float64) \
    V(String) \
    V(ByteArray) \
    V(BytecodeArray) \
    V(Int32Array) \
    V(Array) \
    V(Table) \
    V(Map) \
    V(Closure) \
    V(Function) \
    V(Code) \
    V(Delegated) \
    V(Thread)

// All [strong ref]
struct BuiltinStrPool {
    NyString *kInnerInit = nullptr;
    NyString *kInnerIndex = nullptr;
    NyString *kInnerNewindex = nullptr;
    NyString *kInnerCall = nullptr;
    NyString *kInnerStr = nullptr;
    NyString *kInnerAdd = nullptr;
    NyString *kInnerSub = nullptr;
    NyString *kInnerMul = nullptr;
    NyString *kInnerDiv = nullptr;
    NyString *kInnerMod = nullptr;
    NyString *kInnerUnm = nullptr;
    NyString *kInnerConcat = nullptr;
    NyString *kInnerEq = nullptr;
    NyString *kInnerLt = nullptr;
    NyString *kInnerLe = nullptr;
    NyString *kInnerGC = nullptr;
    NyString *kInnerType = nullptr;
    NyString *kInnerOffset = nullptr;
    NyString *kInnerSize = nullptr;
    NyString *kInnerBase = nullptr;
    NyString *kInnerWeak = nullptr;
    NyString *kInnerG = nullptr;
    NyString *kNil = nullptr;
    NyString *kTrue = nullptr;
    NyString *kFalse = nullptr;
    NyString *kEmpty = nullptr;
    NyString *kRunning = nullptr;
    NyString *kSuspended = nullptr;
    NyString *kDead = nullptr;
    NyString *kResume = nullptr;
    NyString *kCoroutine = nullptr;
    NyString *kString = nullptr;
    NyString *kFloat = nullptr;
    NyString *kInt = nullptr;
    NyString *kMap = nullptr;
    NyString *kTable = nullptr;
    NyString *kDelegated = nullptr;
    NyString *kFunction = nullptr;
    NyString *kCode = nullptr;
    NyString *kClosure = nullptr;
    NyString *kThread = nullptr;
    NyString *kByteArray = nullptr;
    NyString *kInt32Array = nullptr;
    NyString *kArray = nullptr;
}; // struct ConstStrPool
    
extern const char *kRawBuiltinKzs[];
extern const size_t kRawBuiltinKzsSize;
extern const size_t kRawBuiltinCodeSize;

// All [strong ref]
struct BuiltinMetatablePool {
#define DEFINE_METATABLE(type) NyMap *k##type = nullptr;
    DECL_BUILTIN_TYPES(DEFINE_METATABLE)
#undef DEFINE_METATABLE
    
    friend class NyaaCore;
private:
    Error Boot(NyaaCore *N);
}; // struct BuiltinMetatablePool


// All [strong ref]
struct BuiltinCodePool final {
    static constexpr size_t kMaxBytecodeHandles = 256;
    
    NyCode *kInterpreterPump = nullptr;
    NyCode *kEntryTrampoline = nullptr;

    NyCode *kBytecodeHandlers[kMaxBytecodeHandles];
    friend class NyaaCore;
private:
    Error Boot(NyaaCore *N);
}; // struct BuiltinCodePool
    
extern const size_t kRawBuiltinkmtSize;
    
enum DelegatedKind : int {
    kFunctionCallback,
};
    
enum BuiltinType : int {
    kTypeNil,
    kTypeSmi,
    kTypeUdo,
#define DEFINE_TYPE(type) kType##type,
    DECL_BUILTIN_TYPES(DEFINE_TYPE)
#undef DEFINE_TYPE
};
    
extern const char *kBuiltinTypeName[];
    
struct IVal {
    enum Kind {
        kLocal,
        kGlobal, // index = const_pool_idx
        kConst,
        kFunction, // index = proto_pool_idx
        kUpval,
        kHIR,
        kVoid,
    };
    Kind kind;
    
    union {
        int32_t index;
        hir::Value *node;
    };
    
    int32_t Encode() const { return kind == kConst ? -index-1 : index; }
    
    static IVal Void() { return {.kind = kVoid, .index = -1}; }
    static IVal Local(int32_t idx) { return {.kind = kLocal, .index = idx}; }
    static IVal Upval(int32_t idx) { return {.kind = kUpval, .index = idx}; }
    static IVal Const(int32_t idx) { return {.kind = kConst, .index = idx}; }
    static IVal Function(int32_t idx) { return {.kind = kFunction, .index = idx}; }
    static IVal Global(int32_t idx) { return {.kind = kGlobal, .index = idx}; }
    static IVal HIR(hir::Value *node) { return {.kind = kHIR, .node = node}; }
}; // struct IVal
    
struct Operator {
    enum ID {
        kEntry,
        kAdd,
        kSub,
        kMul,
        kDiv,
        kMod,
        kUnm,
        kConcat,
        kShl,
        kShr,
        kEQ,
        kNE,
        kLE,
        kLT,
        kGE,
        kGT,
        kAnd,
        kOr,
        kNot,
        kBitAnd,
        kBitOr,
        kBitXor,
        kBitInv,
        kIndex,
        kNewindex,
    };
}; // struct Operator
    
static constexpr const uint64_t kUdoKidBegin = 1000;

struct NyaaNaFnEntry {
    const char *name;
    void (*nafn)(const FunctionCallbackInfo<Object> &);
};
    
extern const NyaaNaFnEntry kBuiltinFnEntries[];
    
struct UpvalDesc {
    NyString *name; // [strong ref]
    int32_t in_stack;
    int32_t index;
}; // struct UpvalDesc
    
using f64_t = double;

} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_BUILTIN_H_
