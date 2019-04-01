#ifndef MAI_NYAA_BYTE_CODE_H_
#define MAI_NYAA_BYTE_CODE_H_

#include "base/base.h"

namespace mai {
    
namespace nyaa {

struct Bytecode {
    enum ID : uint8_t {
        kDouble,
        kQuadruple,
        kLoadImm, // load local[ra], imm
        kMove, // move local[ra], local[rb]
        kLoadConst, // load const[ra]
        kLoadGlobal, // load local[ra], global[const[rb]]
        kLoadNil, // loadnil local[ra]
        kIndex,
        kIndexConst,

        kAdd, // add local[ra], local[rb], local[rc]
        
        kNew,
        kCall, // call local[ra], rb, rc
        kRet, // ret n
    };
};

} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_BYTE_CODE_H_
