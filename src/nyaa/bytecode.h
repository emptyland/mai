#ifndef MAI_NYAA_BYTE_CODE_H_
#define MAI_NYAA_BYTE_CODE_H_

#include "base/base.h"

namespace mai {
    
namespace nyaa {

struct Bytecode {
    enum ID : uint8_t {
        kDouble,
        kQuadruple,
        kPushImm, // push imm
        kPushLocal, // push local[idx]
        kPushConst, // push const[idx]
        kPushGlobal, // push g[const[idx]]
        kPushNil, // push nil, n
        kIndex, // getfield local[-2], local[-1]
        kIndexConst, // getfield local[-1], const[idx]
        kPop, // pop n

        kAdd, // add local[idx], local[idx]
        
        kNew, // new local[idx], n_param
        kCall, // call local[idx], n_args, n_accepts
        kReturn, // return imm
    };
};

} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_BYTE_CODE_H_
