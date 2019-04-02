#ifndef MAI_NYAA_MEMORY_H_
#define MAI_NYAA_MEMORY_H_

#include "base/slice.h"
#include "base/base.h"

namespace mai {
    
namespace nyaa {
    
enum HeapSpace : int {
    kSemiSpace,
    kNewSpace,
    kOldSpace,
    kCodeSpace,
    kLargeSpace,
};
    
enum HeapColor : int {
    kColorNone  = 0,
    kColorWhite = 1,
    KColorGray  = 2,
    kColorBlack = 3,
};
    
static constexpr const int kPageShift = 20;
static constexpr const size_t kAligmentSize = 4;
static constexpr const size_t kAllocateAlignmentSize = kPointerSize;
static constexpr const size_t kPageSize = 1u << kPageShift;
static constexpr const size_t kLargePageThreshold = kPageSize >> 1;
    
static constexpr const int kAllocateRetries = 16;
    
static const size_t kLargeStringLength = 1024;
    
static const uint32_t kFreeZag = 0xfeedfeed;
static const uint32_t kInitZag = 0xcccccccc;
    
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
    
#if defined(MAI_OS_POSIX)
#define NYAA_USE_POINTER_COLOR 1
#endif
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_MEMORY_H_
