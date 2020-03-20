#pragma once
#ifndef MAI_LANG_STACK_FRAME_H_
#define MAI_LANG_STACK_FRAME_H_

#include "lang/mm.h"

namespace mai {

namespace lang {

class Closure;

struct StackFrame {
    enum Maker: int32_t {
        kTrampoline,
        kStub,
        kBytecode,
    };
    
    static constexpr int32_t kOffsetSavedBP = 0;
    static constexpr int32_t kOffsetReturnAddress = kOffsetSavedBP + sizeof(Address);
    static constexpr int32_t kOffsetMaker = kOffsetSavedBP - static_cast<int>(sizeof(Maker));
    
    static Maker GetMaker(Address frame_bp) {
        return *reinterpret_cast<Maker *>(frame_bp + kOffsetMaker);
    }
}; // struct StackFrame

struct TrampolineStackFrame : public StackFrame {
    static constexpr int32_t kMaker = kTrampoline;

    static constexpr int32_t kOffsetCaughtPoint = kOffsetMaker - 32;
    static constexpr int32_t kSize = RoundUp(-kOffsetCaughtPoint, kStackAligmentSize);
}; // struct TrampolineStackFrame

struct StubStackFrame : public StackFrame {
    static constexpr int32_t kMaker = kStub;
    
    static constexpr int32_t kSize = RoundUp(-kOffsetMaker, kStackAligmentSize);
}; // struct StubStackFrame


struct BytecodeStackFrame : public StackFrame {
    static constexpr int32_t kMaker = kBytecode;
    
    static constexpr int32_t kOffsetPC = kOffsetMaker - static_cast<int>(sizeof(uint32_t));
    static constexpr int32_t kOffsetTop = kOffsetPC - static_cast<int>(sizeof(uint32_t));
    static constexpr int32_t kOffsetCallee = kOffsetTop - static_cast<int>(kPointerSize);
    static constexpr int32_t kOffsetBytecodeArray = kOffsetCallee - static_cast<int>(kPointerSize);
    static constexpr int32_t kOffsetConstPool = kOffsetBytecodeArray - static_cast<int>(kPointerSize);
    static constexpr int32_t kOffsetCaughtPoint = kOffsetConstPool - 32;
    static constexpr int32_t kOffsetLocalVars = kOffsetCaughtPoint;
    static constexpr int32_t kOffsetHeaderSize = -kOffsetLocalVars;
    
    static Closure *GetCallee(Address frame_bp) {
        return *reinterpret_cast<Closure **>(frame_bp + kOffsetCallee);
    }
    
    static int32_t GetPC(Address frame_bp) {
        return *reinterpret_cast<int32_t *>(frame_bp + kOffsetPC);
    }
}; // struct BytecodeStackFrame


} // namespace lang

} // namespace mai

#endif // MAI_LANG_STACK_FRAME_H_
