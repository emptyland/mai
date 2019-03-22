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
        kPushConst, // push const[idx]
        kPushGlobal, // push g, const[idx]
        kPushNil, // push nil, n
        
        kAdd, // add local[idx], local[idx]
        
        kCall, // call local[idx], n_param, n_accept
        kReturn, // return imm
    };
};

} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_BYTE_CODE_H_
