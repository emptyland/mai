#include "nyaa/bytecode.h"

namespace mai {

namespace nyaa {

const char *Bytecode::kName[kMaxCode] = {
#define DEF_NAME(name, type) #name,
    DECL_BYTECODES(DEF_NAME)
#undef DEF_NAME
};

} // namespace nyaa

} // namespace mai
