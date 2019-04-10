#ifndef MAI_NYAA_BYTE_CODE_H_
#define MAI_NYAA_BYTE_CODE_H_

#include "base/base.h"

namespace mai {
    
namespace nyaa {
    
#define DECL_BYTECODES(V) \
    V(LoadImm,     2, a, b) \
    V(Move,        2, a, b) \
    V(LoadConst,   2, a, b) \
    V(LoadGlobal,  2, a, b) \
    V(LoadNil,     2, a, b) \
    V(GetField,    2, a, b) \
    V(SetField,    3, a, b, c)\
    V(StoreGlobal, 2, a, b) \
    V(Add,         3, a, b, c) \
    V(Sub,         3, a, b, c) \
    V(Mul,         3, a, b, c) \
    V(Div,         3, a, b, c) \
    V(New,         3, a, b, c) \
    V(Call,        3, a, b, c) \
    V(Ret,         2, a, b)

struct Bytecode {
    enum ID : uint8_t {
        kDouble,
        kQuadruple,
#define DEFINE_ENUM(name, ...) k##name,
        DECL_BYTECODES(DEFINE_ENUM)
#undef DEFINE_ENUM
    };
    
    static int32_t ParseInt32Param(View<Byte> bytes, int scale);
    
    static const char *kNames[];
};

} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_BYTE_CODE_H_
