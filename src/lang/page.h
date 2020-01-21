#ifndef MAI_LANG_PAGE_H_
#define MAI_LANG_PAGE_H_

#include "lang/mm.h"
#include "base/bit-ops.h"
#include "base/queue-macros.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {
class Allocator;
namespace lang {

class PageHeader {
public:
    DEF_VAL_GETTER(SpaceKind, owner_space);
    DEF_VAL_GETTER(uint32_t, available);
    
    static PageHeader *FromAddress(Address addr) {
        return reinterpret_cast<PageHeader *>(reinterpret_cast<uintptr_t>(addr) & kPageMask);
    }

    friend class OldSpace;
    friend class LargeSpace;
    DISALLOW_IMPLICIT_CONSTRUCTORS(PageHeader);
protected:
    PageHeader(SpaceKind space)
        : maker_(kPageMaker)
        , owner_space_(space)
        , available_(0)
        , next_(this)
        , prev_(this) {}
    
    static void *AllocatePage(size_t size, int access, Allocator *lla);

    uint32_t maker_; // maker for verification
    SpaceKind owner_space_; // owner space kind
    uint32_t available_;
    QUEUE_HEADER(PageHeader); // double-linked list pointer
}; // class PageHeader


template<size_t N>
class AllocationBitmap final {
public:
    static constexpr int kBucketShift = 5;
    static constexpr int kBucketBits = 1 << kBucketShift;
    static constexpr size_t kBucketMask = kBucketBits - 1;
    
    inline void MarkAllocated(ptrdiff_t offset, size_t size);
    
    inline void ClearAllocated(ptrdiff_t offset);
    
    inline bool HasAllocated(ptrdiff_t offset) {
        DCHECK_GE(offset, 0);
        DCHECK_EQ(0, offset % kPointerSize);
        return Test(offset >> kPointerShift);
    }
    
    inline size_t AllocatedSize(ptrdiff_t offset);

    static inline AllocationBitmap *FromBits(uint32_t *bits) {
        return reinterpret_cast<AllocationBitmap *>(bits);
    }
    
    static inline AllocationBitmap *FromArray(uint32_t bits[N]) {
        return reinterpret_cast<AllocationBitmap *>(bits);
    }
private:
    inline bool Test(size_t i) const {
        DCHECK_LT(i, N * 32);
        return bits_[i >> kBucketShift] & (1u << (i & kBucketMask));
    }
    
    inline void Set(size_t i) {
        DCHECK_LT(i, N * 32);
        bits_[i >> kBucketShift] |= 1u << (i & kBucketMask);
    }
    
    inline void Clear(size_t i) {
        DCHECK_LT(i, N * 32);
        bits_[i >> kBucketShift] &= ~(1u << (i & kBucketMask));
    }

    // begins of b
    inline size_t FindNextOne(size_t b);

    uint32_t bits_[N];
}; // template<size_t N> class Bitmap


class Page final : public PageHeader {
public:
    struct Chunk {
        Chunk   *next;
        uint32_t size;
    }; // struct Chunk
    
    struct Region {
        Chunk *tiny   = nullptr; // [0, 64) bytes
        Chunk *small  = nullptr; // [64, 512) bytes
        Chunk *medium = nullptr; // [512, 1024) bytes
        Chunk *normal = nullptr; // [1024, 4096) bytes
        Chunk *big    = nullptr; // >= 4096 bytes
    }; // struct Region
    
    static constexpr size_t kChunkSize = ((kPageSize - sizeof(PageHeader) - sizeof(Region)) * 256) / 260;
    static constexpr size_t kBitmapSize = kChunkSize / 256;
    
    using Bitmap = AllocationBitmap<kBitmapSize>;
    
    static constexpr size_t kMaxRegionChunks = 5;
    static const size_t kRegionLimitSize[kMaxRegionChunks];
    
    Address chunk() { return chunk_; }
    Address limit() { return reinterpret_cast<Address>(this) + kPageSize; }
    
    static Page *New(SpaceKind space, int access, Allocator *lla) {
        void *chunk = AllocatePage(kPageSize, access, lla);
        if (!chunk) {
            return nullptr;
        }
        return new (chunk) Page(space);
    }

    friend class OldSpace;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Page);
private:
    Page(SpaceKind space);
    
    size_t FindFitRegion(size_t size) const {
        for (size_t i = 0; i < kMaxRegionChunks; ++i) {
            if (size < kRegionLimitSize[i] && region_array()[i]) {
                return i;
            }
        }
        return kMaxRegionChunks;
    }
    
    static size_t FindWantedRegion(size_t size) {
        for (size_t i = 0; i < kMaxRegionChunks; ++i) {
            if (size < kRegionLimitSize[i]) {
                return i;
            }
        }
        return kMaxRegionChunks;
    }
    
    Chunk **region_array() const { return reinterpret_cast<Chunk **>(&region_); }
    
    Bitmap *bitmap() { return Bitmap::FromArray(bitmap_); }
    
    mutable Region region_; // free regions
    uint32_t bitmap_[kBitmapSize]; // usage bitmap
    uint8_t chunk_[0]; // chunk for allocation
}; // class Page


class LargePage final : public PageHeader {
public:
    Address chunk() { return chunk_; }
    Address limit() { return reinterpret_cast<Address>(this) + size_; }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(LargePage);
private:
    size_t size_; // large object size
    uint8_t chunk_[0]; // chunk for allocation
};

template<size_t N>
inline size_t AllocationBitmap<N>::AllocatedSize(ptrdiff_t offset) {
    DCHECK_GE(offset, 0);
    DCHECK_EQ(0, offset % kPointerSize);
    size_t i = offset >> kPointerShift;
    if (!Test(i)) {
        return 0;
    }
    size_t j = FindNextOne(i + 1);
    return (j - i + 1) << kPointerShift;
}

template<size_t N>
inline void AllocationBitmap<N>::MarkAllocated(ptrdiff_t offset, size_t size) {
    DCHECK_GE(size, 2 * kPointerSize);
    DCHECK_GE(offset, 0);
    DCHECK_EQ(0, offset % kPointerSize);
    size_t i = offset >> kPointerShift;
    DCHECK(!Test(i));
    size_t j = i + (size >> kPointerShift) - 1;
    DCHECK(!Test(j));
    Set(i);
    Set(j);
}

template<size_t N>
inline void AllocationBitmap<N>::ClearAllocated(ptrdiff_t offset) {
    DCHECK_GE(offset, 0);
    DCHECK_EQ(0, offset % kPointerSize);
    size_t i = offset >> kPointerShift;
    DCHECK(Test(i));
    size_t j = FindNextOne(i + 1);
    Clear(i);
    Clear(j);
}

template<size_t N> inline size_t AllocationBitmap<N>::FindNextOne(size_t b) {
    size_t x = b >> kBucketShift;
    if (uint32_t bits = bits_[x]; bits != 0) {
        for (size_t i = b & kBucketMask; i < 32; i++) {
            if (bits & (1u << i)) {
                return (x << kBucketShift) + i;
            }
        }
    }
    for (size_t i = x + 1; i < N; i++) {
        if (bits_[i]) {
            return (i << kBucketShift) + base::Bits::FindFirstOne32(bits_[i]);
        }
    }
    NOREACHED();
    return 0;
}

} // namespace lang

} // namespace mai

#endif // MAI_LANG_PAGE_H_
