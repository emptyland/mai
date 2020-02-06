#ifndef MAI_LANG_RUNTIME_H_
#define MAI_LANG_RUNTIME_H_

#include "base/base.h"

namespace mai {

namespace lang {

// The runtime functions definition
struct Runtime {
    
    // Debug abort message output and fast abort code execution
    static void DebugAbort(const char *message);
    
}; // struct Runtime

} // namespace lang

} // namespace mai

#endif // MAI_LANG_RUNTIME_H_
