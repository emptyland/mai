#ifndef MAI_NYAA_BUILTIN_H_
#define MAI_NYAA_BUILTIN_H_

#include "base/base.h"
#include "mai/error.h"

namespace mai {
    
namespace nyaa {
    
class NyString;
class NyTable;
class NyaaCore;

class Arguments;
class Nyaa;

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
    NyString *kInnerID = nullptr;
    NyString *kInnerSize = nullptr;
    NyString *kInnerBase = nullptr;
    NyString *kNil = nullptr;
    NyString *kTrue = nullptr;
    NyString *kFalse = nullptr;
    NyString *kEmpty = nullptr;
}; // struct ConstStrPool
    
extern const char *kRawBuiltinKzs[];
extern const size_t kRawBuiltinKzsSize;
    
// All [strong ref]
struct BuiltinMetatablePool {
    NyTable *kString = nullptr;
    NyTable *kTable = nullptr;
    NyTable *kDelegated = nullptr;
    NyTable *kScript = nullptr;
    NyTable *kThread = nullptr;
    NyTable *kByteArray = nullptr;
    NyTable *kInt32Array = nullptr;
    NyTable *kArray = nullptr;
    // TODO:
    
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

struct NyaaNaFnEntry {
    const char *name;
    int (*nafn)(Arguments *, Nyaa *);
};
    
extern const NyaaNaFnEntry kBuiltinFnEntries[];

} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_BUILTIN_H_
