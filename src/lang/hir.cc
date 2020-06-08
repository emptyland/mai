#include "lang/hir.h"


namespace mai {

namespace lang {

/*static*/ const char *HOperator::kNames[HMaxOpcode] = {
#define DEFINE_CODE(name, ...) #name,
    DECLARE_HIR_OPCODES(DEFINE_CODE)
#undef DEFINE_CODE
};

void HOperatorFactory::Initialize() {
    // TODO:
}

} // namespace lang

} // namespace mai
