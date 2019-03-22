#ifndef MAI_NYAA_MEMORY_H_
#define MAI_NYAA_MEMORY_H_

#include "base/slice.h"
#include "base/base.h"

namespace mai {
    
namespace nyaa {
    
enum HeapArea : int {
    SEMI_SPACE,
    NEW_SPACE,
    OLD_SPACE,
    CODE_SPACE,
    LARGE_SPACE,
};
    
static const size_t kAllocateAlignmentSize = kPointerSize;
    
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
    
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_MEMORY_H_
