#ifndef MAI_LANG_MM_H_
#define MAI_LANG_MM_H_

#include "base/slice.h"
#include "base/base.h"
#include <memory>

namespace mai {

namespace lang {

class Any;

enum SpaceKind : int {
    kDummySpace,
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
static constexpr size_t kStackAligmentSize = 16; // Stack alignment size
static constexpr size_t kMinAllocationSize = kPointerSize << 1; // Two-Pointers

static constexpr size_t kMaxStackPoolRSS = 400 * base::kMB;
static constexpr size_t kC0StackSize = 80 * base::kMB;
static constexpr size_t kDefaultStackSize = 20 * base::kMB;

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

union SpanPart64 {
    uint64_t u64;
    int64_t  i64;
    double   f64;
};

union SpanPart32 {
    uint32_t u32;
    int32_t  i32;
    float    f32;
};

union SpanPartPtr {
    void *pv;
    uint8_t *addr;
    Any *any;
};

#define DECLARE_SPAN_PARTS(n) \
    SpanPartPtr ptr[n]; \
    SpanPart64 v64[n]; \
    SpanPart32 v32[n*2]; \
    uint16_t u16[n*4]; \
    int16_t i16[n*4]; \
    int8_t i8[n*8]; \
    uint8_t u8[n*8]
    
union Span16 {
    DECLARE_SPAN_PARTS(2);
};

union Span32 {
    DECLARE_SPAN_PARTS(4);
};

union Span64 {
    DECLARE_SPAN_PARTS(8);
};

#undef DECLARE_SPAN_PARTS

static_assert(sizeof(Span16) == 16, "Fixed span16 size");
static_assert(sizeof(Span32) == 32, "Fixed span32 size");
static_assert(sizeof(Span64) == 64, "Fixed span64 size");

using BytecodeInstruction = uint32_t;

// Remember set record for old-generation -> new-generation
struct RememberRecord {
    uint64_t seuqnce_number; // Sequence number
    Any *host; // Host object pointer
    Any **address; // Write to address
}; //struct RememberRecord

inline size_t GetMinAllocationSize(size_t n) {
    return n < kMinAllocationSize ? kMinAllocationSize : RoundUp(n, kPointerSize);
}

class AllocationResult final {
public:
    enum Result {
        OK, // Allocation is ok, chunk is result memory block
        FAIL, // Just fail
        LIMIT, // On soft capacity limit
        NOTHING, // Did not allocation, address is nullptr
        OOM, // Low-level-allocator return no memory, chunk is nullptr
    };

    DEF_VAL_GETTER(Result, result);
    DEF_VAL_GETTER(Address, address);
    
    bool ok() const { return result_ == OK; }

    void *ptr() const { return static_cast<void *>(address_); }
    
    Any *object() const { return static_cast<Any *>(ptr()); }
    
    friend class Heap;
    friend class NewSpace;
    friend class OldSpace;
    friend class LargeSpace;
    friend class MetadataSpace;
private:
    AllocationResult(): AllocationResult(NOTHING, nullptr) {}

    AllocationResult(Result result, Address chunk)
        : result_(result)
        , address_(chunk) {}
    
    Result result_;
    Address address_;
};

} // namespace lang

} // namespace mai

#endif // MAI_LANG_MM_H_
