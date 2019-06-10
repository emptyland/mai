#ifndef MAI_NYAA_BUILTIN_H_
#define MAI_NYAA_BUILTIN_H_

#include "base/base.h"
#include "mai/error.h"

namespace mai {
    
namespace nyaa {
    
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
    V(Int) \
    V(String) \
    V(ByteArray) \
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
    NyCode *kEntryTrampoline = nullptr;
    NyCode *kCallStub = nullptr;
    
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
        kVoid,
    };
    Kind kind;
    int32_t index;
    
    int32_t Encode() const { return kind == kConst ? -index-1 : index; }
    
    static IVal Void() { return {kVoid, -1}; }
    static IVal Local(int32_t idx) { return {kLocal, idx}; }
    static IVal Upval(int32_t idx) { return {kUpval, idx}; }
    static IVal Const(int32_t idx) { return {kConst, idx}; }
    static IVal Function(int32_t idx) { return {kFunction, idx}; }
    static IVal Global(int32_t idx) { return {kGlobal, idx}; }
}; // struct IVal
    
struct Operator {
    enum ID {
        kEntry,
        kAdd,
        kSub,
        kMul,
        kDiv,
        kMod,
        kNeg,
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
//        kIndex,
//        kNewindex,
    };
}; // struct Operator
    
static constexpr const uint64_t kUdoKidBegin = 1000;

struct NyaaNaFnEntry {
    const char *name;
    void (*nafn)(const FunctionCallbackInfo<Object> &);
};
    
extern const NyaaNaFnEntry kBuiltinFnEntries[];
    
//Error LoadClassCoroutine(NyaaCore *N);
    
using f64_t = double;

} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_BUILTIN_H_
