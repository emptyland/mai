#ifndef MAI_NYAA_BUILTIN_H_
#define MAI_NYAA_BUILTIN_H_

#include "base/base.h"
#include "mai/error.h"

namespace mai {
    
namespace nyaa {
    
class NyString;
class NyMap;
class NyTable;
class NyaaCore;

class Arguments;
class Nyaa;
    
#define DECL_BUILTIN_TYPES(V) \
    V(Float64) \
    V(Long) \
    V(String) \
    V(ByteArray) \
    V(Int32Array) \
    V(Array) \
    V(Table) \
    V(Map) \
    V(Script) \
    V(Function) \
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
    NyString *kInnerSize = nullptr;
    NyString *kInnerBase = nullptr;
    NyString *kInnerWeak = nullptr;
    NyString *kNil = nullptr;
    NyString *kTrue = nullptr;
    NyString *kFalse = nullptr;
    NyString *kEmpty = nullptr;
}; // struct ConstStrPool
    
extern const char *kRawBuiltinKzs[];
extern const size_t kRawBuiltinKzsSize;
    
// All [strong ref]
struct BuiltinMetatablePool {
#define DEFINE_METATABLE(type) NyMap *k##type = nullptr;
    DECL_BUILTIN_TYPES(DEFINE_METATABLE)
#undef DEFINE_METATABLE
    
    friend class NyaaCore;
private:
    Error Boot(NyaaCore *N);
}; // struct BuiltinMetatablePool
    
enum DelegatedKind : int {
    kPropertyGetter,
    kPropertySetter,
    kArg0,
    kArg1,
    kArg2,
    kArg3,
    kUniversal,
};
    
enum BuiltinType : int {
#define DEFINE_TYPE(type) kType##type,
    DECL_BUILTIN_TYPES(DEFINE_TYPE)
#undef DEFINE_TYPE
};

struct NyaaNaFnEntry {
    const char *name;
    int (*nafn)(Arguments *, Nyaa *);
};
    
extern const NyaaNaFnEntry kBuiltinFnEntries[];
    
using f64_t = double;

} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_BUILTIN_H_
