#ifndef MAI_NYAA_MEMORY_H_
#define MAI_NYAA_MEMORY_H_

#include "base/slice.h"
#include "base/base.h"

namespace mai {
    
namespace nyaa {
    
class NyaaCore;
class Nyaa;
class Heap;
class NyUDO;

enum HeapSpace : int {
    kSemiSpace,
    kNewSpace,
    kOldSpace,
    kCodeSpace,
    kLargeSpace,
};

enum HeapColor : int {
    kColorFreed  = 0,
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

static const size_t kLargeStringLength = 128;

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
#define NYAA_USE_POINTER_TYPE  1
#endif
    
    
using UDOFinalizer = void (*)(NyUDO *udo, NyaaCore *);

enum GarbageCollectionMethod {
    kMajorGC,
    kMinorGC,
    kFullGC,
};

struct GarbageCollectionHistogram {
    size_t collected_bytes = 0;
    size_t collected_objs  = 0;
    double time_cost       = 0;
}; // struct GarbageCollectionHistogram

class GarbageCollectionPolicy {
public:
    GarbageCollectionPolicy(NyaaCore *core, Heap *heap)
        : core_(core)
        , heap_(heap) {}
    virtual ~GarbageCollectionPolicy() {}
    
    DEF_PTR_GETTER(NyaaCore, core);
    DEF_PTR_GETTER(Heap, heap);
    DEF_VAL_GETTER(size_t, collected_bytes);
    DEF_VAL_GETTER(size_t, collected_objs);
    DEF_VAL_GETTER(double, time_cost);
    
    virtual void Run() = 0;

    virtual void Reset() {
        collected_bytes_ = 0;
        collected_objs_ = 0;
        time_cost_ = 0;
    }

    DISALLOW_IMPLICIT_CONSTRUCTORS(GarbageCollectionPolicy);
protected:
    NyaaCore *const core_;
    Heap *const heap_;
    size_t collected_bytes_ = 0;
    size_t collected_objs_ = 0;
    double time_cost_ = 0;

}; // class GarbageCollectionPolicy
    
} // namespace nyaa

} // namespace mai


#endif // MAI_NYAA_MEMORY_H_
