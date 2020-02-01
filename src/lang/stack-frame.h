#ifndef MAI_LANG_STACK_FRAME_H_
#define MAI_LANG_STACK_FRAME_H_

#include "lang/mm.h"

namespace mai {

namespace lang {

struct StackFrame {
    enum Maker: int32_t {
        kTrampoline,
        kStub,
    };
    
    static constexpr int32_t kOffsetSavedBP = 0;
    static constexpr int32_t kOffsetReturnAddress = kOffsetSavedBP + sizeof(Address);
    static constexpr int32_t kOffsetMaker = kOffsetSavedBP - static_cast<int>(sizeof(Maker));
}; // struct StackFrame


struct StubStackFrame : public StackFrame {
    static constexpr int32_t kMaker = kStub;
    
    static constexpr int32_t kSize = RoundUp(-kOffsetMaker, kStackAligmentSize);
}; // struct StubStackFrame


} // namespace lang

} // namespace mai

#endif // MAI_LANG_STACK_FRAME_H_
