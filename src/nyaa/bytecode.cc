#include "nyaa/bytecode.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {
    
/*static*/ int32_t Bytecode::ParseInt32Param(View<Byte> bytes, int scale) {
    DCHECK_GE(bytes.n, scale);
    switch(scale) {
        case 1: {
            int8_t b = bytes.z[0];
            return b;
        } break;
        case 2: {
            uint8_t lo = bytes.z[0];
            uint8_t hi = bytes.z[1];
            return static_cast<int32_t>(static_cast<uint16_t>(hi) << 8 | lo);
        } break;
        case 4: {
            uint8_t lo0 = bytes.z[0];
            uint8_t lo1 = bytes.z[1];
            uint8_t hi0 = bytes.z[2];
            uint8_t hi1 = bytes.z[3];
            return static_cast<int32_t>(static_cast<uint32_t>(hi1) << 24 |
                                        static_cast<uint32_t>(hi0) << 16 |
                                        static_cast<uint32_t>(lo1) << 8 | lo0);
        } break;
        default:
            DLOG(FATAL) << "bad scale: " << scale;
            break;
    }
}
    
/*static*/ const char *Bytecode::kNames[] = {
    "", //kDouble,
    "", //kQuadruple,
#define DEFINE_NAME(name, ...) #name,
    DECL_BYTECODES(DEFINE_NAME)
#undef DEFINE_NAME
};

} // namespace nyaa
    
} // namespace mai
