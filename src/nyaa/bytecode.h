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
    V(LoadUp,      2, a, b) \
    V(LoadNil,     2, a, b) \
    V(GetField,    3, a, b, c) \
    V(SetField,    3, a, b, c) \
    V(Self,        3, a, b, c) \
    V(GetUpField,  2, a, b) \
    V(SetUPField,  3, a, b, c) \
    V(StoreGlobal, 2, a, b) \
    V(StoreUp,     2, a, b) \
    V(NewMap,      3, a, b, c) \
    V(NewClass,    3, a, b, c) \
    V(Add,         3, a, b, c) \
    V(Sub,         3, a, b, c) \
    V(Mul,         3, a, b, c) \
    V(Div,         3, a, b, c) \
    V(Equal,       3, a, b, c) \
    V(NotEqual,    3, a, b, c) \
    V(LessThan,    3, a, b, c) \
    V(LessEqual,   3, a, b, c) \
    V(GreaterThan, 3, a, b, c) \
    V(GreaterEqual,3, a, b, c) \
    V(Test,        3, a, b, c) \
    V(JumpImm,     1, a) \
    V(JumpConst,   1, a) \
    V(Closure,     2, a, b) \
    V(Vargs,       2, a, b) \
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
