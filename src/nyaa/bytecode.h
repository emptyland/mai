#ifndef MAI_NYAA_BYTE_CODE_H_
#define MAI_NYAA_BYTE_CODE_H_

#include "base/base.h"

namespace mai {
    
namespace nyaa {
    
#define DECL_BYTECODES(V) \
    V(LoadImm,    2, a, b) \
    V(Move,       2, a, b) \
    V(LoadConst,  2, a, b) \
    V(LoadGlobal, 2, a, b) \
    V(LoadNil,    2, a, b) \
    V(Index,      2, a, b) \
    V(IndexConst, 2, a, b) \
    V(Add,        3, a, b, c) \
    V(New,        3, a, b, c) \
    V(Call,       3, a, b, c) \
    V(Ret,        2, a, b)

struct Bytecode {
    enum ID : uint8_t {
//        kLoadImm, // load local[ra], imm
//        kMove, // move local[ra], local[rb]
//        kLoadConst, // load const[ra]
//        kLoadGlobal, // load local[ra], global[const[rb]]
//        kLoadNil, // loadnil local[ra]
//        kIndex,
//        kIndexConst,
//
//        kAdd, // add local[ra], local[rb], local[rc]
//
//        kNew,
//        kCall, // call local[ra], rb, rc
//        kRet, // ret n
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
