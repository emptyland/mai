#pragma once
#ifndef MAI_LANG_STACK_FRAME_H_
#define MAI_LANG_STACK_FRAME_H_

#include "lang/mm.h"

namespace mai {

namespace lang {

struct StackFrame {
    enum Maker: int32_t {
        kTrampoline,
        kStub,
        kBytecode,
    };
    
    static constexpr int32_t kOffsetSavedBP = 0;
    static constexpr int32_t kOffsetReturnAddress = kOffsetSavedBP + sizeof(Address);
    static constexpr int32_t kOffsetMaker = kOffsetSavedBP - static_cast<int>(sizeof(Maker));
}; // struct StackFrame


struct StubStackFrame : public StackFrame {
    static constexpr int32_t kMaker = kStub;
    
    static constexpr int32_t kSize = RoundUp(-kOffsetMaker, kStackAligmentSize);
}; // struct StubStackFrame


struct BytecodeStackFrame : public StackFrame {
    static constexpr int32_t kMaker = kBytecode;
    
    static constexpr int32_t kOffsetPC = kOffsetMaker - static_cast<int>(sizeof(uint32_t));
    static constexpr int32_t kOffsetCallee = kOffsetPC - static_cast<int>(kPointerSize);
    static constexpr int32_t kOffsetBytecodeArray = kOffsetCallee - static_cast<int>(kPointerSize);
    static constexpr int32_t kOffsetConstPool = kOffsetBytecodeArray - static_cast<int>(kPointerSize);
    static constexpr int32_t kOffsetCaughtPoint = kOffsetConstPool - 32;
    static constexpr int32_t kOffsetLocalVars = kOffsetCaughtPoint;
    static constexpr int32_t kOffsetHeaderSize = -kOffsetLocalVars;
}; // struct BytecodeStackFrame


} // namespace lang

} // namespace mai

#endif // MAI_LANG_STACK_FRAME_H_
