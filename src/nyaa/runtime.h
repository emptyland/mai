#ifndef MAI_NYAA_RUNTIME_H_
#define MAI_NYAA_RUNTIME_H_

#include "base/base.h"

namespace mai {

namespace nyaa {

struct StackFrame {
    enum Type {
        kEntryTrampolineFrame,
        kInterpreterPump,
        kInterpreterFrame,
    };
    
    constexpr static int32_t kMakerOffet = -1 * kPointerSize;
}; // struct StackFrame



struct InterpreterFrame final : public StackFrame {
    constexpr static Type kType = kInterpreterFrame;
    
    constexpr static int32_t kCalleeOffet = -2 * kPointerSize;
    constexpr static int32_t kBytecodeArrayOffet = -3 * kPointerSize;
    constexpr static int32_t kConstantPoolOffet = -4 * kPointerSize;
}; // struct InterpreterFrame

} // namespace nyaa

} // namespace mai

#endif // MAI_NYAA_RUNTIME_H_
