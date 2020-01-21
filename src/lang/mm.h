#ifndef MAI_LANG_MM_H_
#define MAI_LANG_MM_H_

#include "base/slice.h"
#include "base/base.h"
#include <memory>

namespace mai {

namespace lang {

enum SpaceKind : int {
    kSemiSpace,
    kNewSpace,
    kOldSpace,
    kLargeSpace,
    kMetadataSpace,
};

enum HeapColor : int {
    kColorFreed  = 0,
    kColorWhite = 1,
    KColorGray  = 2,
    kColorBlack = 3,
};

static constexpr int kPageShift = 20;
static constexpr size_t kPageSize = 1u << kPageShift; // 1MB
static constexpr uintptr_t kPageMask = ~((1lu << kPageShift) - 1);
static constexpr uint32_t kPageMaker = 0x9a6e0102u;
static constexpr int kAllocateRetries = 16; // How many times to allocation retires?

static constexpr size_t kAligmentSize = 4; // Heap allocation aligment to 4 bytes

static constexpr uint32_t kFreeZag = 0xfeedfeed;
static constexpr uint32_t kInitZag = 0xcccccccc;

#if defined(_DEBUG) || defined(DEBUG)
static inline void *DbgFillInitZag(void *p, size_t n) {
    if (!p || n == 0) {
        return nullptr;
    }
    base::Round32BytesFill(kInitZag, p, n);
    return p;
}

static inline void *DbgFillFreeZag(void *p, size_t n) {
    if (!p || n == 0) {
        return nullptr;
    }
    base::Round32BytesFill(kFreeZag, p, n);
    return p;
}
#else
static inline void *DbgFillInitZag(void *p, size_t n) { return p; }
static inline void *DbgFillFreeZag(void *p, size_t n) { return p; }
#endif

} // namespace lang

} // namespace mai

#endif // MAI_LANG_MM_H_
