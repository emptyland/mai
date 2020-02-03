#pragma once
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


template<class Base>
class AllocationBitmap {
public:
    static constexpr int kBucketShift = 5;
    static constexpr int kBucketBits = 1 << kBucketShift;
    static constexpr size_t kBucketMask = kBucketBits - 1;
    
    inline void MarkAllocated(ptrdiff_t offset, size_t size);
    
    inline void ClearAllocated(ptrdiff_t offset);
    
    inline bool HasAllocated(ptrdiff_t offset) const {
        DCHECK_GE(offset, 0);
        DCHECK_EQ(0, offset % kPointerSize);
        return base()->Test(offset >> kPointerShift);
    }
    
    inline size_t AllocatedSize(ptrdiff_t offset) const;
    
    using ParentType = Base;
private:
    inline ParentType *base() { return reinterpret_cast<ParentType *>(this); }
    inline const ParentType *base() const { return reinterpret_cast<const ParentType *>(this); }
}; // class AllocationBitmap


template<size_t N>
class FixedAllocationBitmap final : public AllocationBitmap<FixedAllocationBitmap<N>> {
public:
    static inline FixedAllocationBitmap *FromArray(uint32_t bits[N]) {
        return reinterpret_cast<FixedAllocationBitmap *>(bits);
    }

    friend class AllocationBitmap<FixedAllocationBitmap<N>>;
private:
    using AllocationBitmap<FixedAllocationBitmap<N>>::kBucketShift;
    using AllocationBitmap<FixedAllocationBitmap<N>>::kBucketMask;
    
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
    inline size_t FindNextOne(size_t b) const;

    uint32_t bits_[N];
}; // template<size_t N> class Bitmap


class RestrictAllocationBitmap final : public AllocationBitmap<RestrictAllocationBitmap> {
public:
    static inline RestrictAllocationBitmap *FromSizeAddress(uint32_t *address) {
        return reinterpret_cast<RestrictAllocationBitmap *>(address);
    }
    
    friend class AllocationBitmap<RestrictAllocationBitmap>;
private:
    RestrictAllocationBitmap() : length_(0) {}
    
    using AllocationBitmap<RestrictAllocationBitmap>::kBucketShift;
    using AllocationBitmap<RestrictAllocationBitmap>::kBucketMask;
    
    inline bool Test(size_t i) const {
        DCHECK_LT(i, length_ * 32);
        DCHECK(bits_[i >> kBucketShift].is_lock_free());
        //return bits_[i >> kBucketShift] & (1u << (i & kBucketMask));
        return bits_[i >> kBucketShift].load() & (1u << (i & kBucketMask));
    }

    inline void Set(size_t i) {
        DCHECK_LT(i, length_ * 32);
        DCHECK(bits_[i >> kBucketShift].is_lock_free());
        //bits_[i >> kBucketShift] |= 1u << (i & kBucketMask);
        bits_[i >> kBucketShift].fetch_or(1u << (i & kBucketMask));
    }

    inline void Clear(size_t i) {
        DCHECK_LT(i, length_ * 32);
        DCHECK(bits_[i >> kBucketShift].is_lock_free());
        //bits_[i >> kBucketShift] &= ~(1u << (i & kBucketMask));
        bits_[i >> kBucketShift].fetch_and(~(1u << (i & kBucketMask)));
    }
    
    inline size_t FindNextOne(size_t b) const;

    const uint32_t length_;
    std::atomic<uint32_t> bits_[0];
    static_assert(sizeof(bits_[0]) == sizeof(uint32_t), "Bad type size!");
}; // class RestrictAllocationBitmap


class PageHeader {
public:
    DEF_VAL_GETTER(SpaceKind, owner_space);
    DEF_VAL_GETTER(uint32_t, available);
    
    static PageHeader *FromAddress(Address addr) {
        return reinterpret_cast<PageHeader *>(reinterpret_cast<uintptr_t>(addr) & kPageMask);
    }

    friend class OldSpace;
    friend class LargeSpace;
    friend class MetadataSpace;
    DISALLOW_IMPLICIT_CONSTRUCTORS(PageHeader);
protected:
    PageHeader(SpaceKind space)
        : maker_(kPageMaker)
        , owner_space_(space)
        , available_(0)
        , next_(this)
        , prev_(this) {}
    
    static void *AllocatePage(size_t size, int access, Allocator *lla);
    
    DEF_VAL_GETTER(uint32_t, maker);
    DEF_PTR_GETTER(PageHeader, next);
    DEF_PTR_GETTER(PageHeader, prev);

    uint32_t maker_; // Maker for verification
    SpaceKind owner_space_; // Owner space kind
    uint32_t available_; // Available bytes size
    QUEUE_HEADER(PageHeader); // Double-linked list pointer
}; // class PageHeader


// Normal objects page
class Page final : public PageHeader {
public:
    struct Chunk {
        Chunk   *next;
        uint32_t size;
    }; // struct Chunk
    
    struct Region {
        Chunk *tiny;   // [0, 64) bytes
        Chunk *small;  // [64, 512) bytes
        Chunk *medium; // [512, 1024) bytes
        Chunk *normal; // [1024, 4096) bytes
        Chunk *big;    // >= 4096 bytes
    }; // struct Region
    
    static constexpr size_t kChunkSize = ((kPageSize - sizeof(PageHeader) - sizeof(Region)) * 256) / 260;
    static constexpr size_t kBitmapSize = kChunkSize / 256;
    
    using Bitmap = FixedAllocationBitmap<kBitmapSize>;

    static constexpr size_t kMaxRegionChunks = 5;
    static const size_t kRegionLimitSize[kMaxRegionChunks];
    
    Address chunk() { return chunk_; }
    Address limit() { return reinterpret_cast<Address>(this) + kPageSize; }
    
    bool IsEmpty() { return limit() - chunk() == available_; }
    
    static Page *FromAddress(Address addr) {
        return !addr ? nullptr : Cast(PageHeader::FromAddress(addr));
    }
    
    static Page *Cast(PageHeader *h) { return static_cast<Page *>(h); }
    
    static bool IsLarge(size_t n) {
        return n > ((kPageSize - sizeof(Page)) >> 1);
    }

    friend class OldSpace;
    // For unit-tests:
    FRIEND_UNITTEST_CASE(PageTest, AllocateNormalPage);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Page);
private:
    Page(SpaceKind space): PageHeader(space) { Reinitialize(); }
    
    void Reinitialize();
    
    void Dispose(Allocator *lla) { lla->Free(this, kPageSize); }
    
    static Page *New(SpaceKind space, int access, Allocator *lla) {
        void *chunk = AllocatePage(kPageSize, access, lla);
        if (!chunk) {
            return nullptr;
        }
        return new (chunk) Page(space);
    }
    
    void InsertFitRegion(size_t size, Chunk *chunk) {
        size_t i = FindWantedRegion(size);
        DCHECK_NE(kMaxRegionChunks, i);
        chunk->next = region(i);
        set_region(i, chunk);
    }
    
    size_t FindFitRegion(size_t size) const {
        for (size_t i = 0; i < kMaxRegionChunks; ++i) {
            if (size < kRegionLimitSize[i] && region(i)) {
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
    
    void AddAvailable(size_t n) { available_ += n; }
    
    void SubAvailable(size_t n) { available_ -= n; }
    
    Chunk **regions() const { return reinterpret_cast<Chunk **>(&region_); }
    
    Chunk *region(size_t i) const {
        DCHECK_LT(i, kMaxRegionChunks);
        return regions()[i];
    }
    
    void set_region(size_t i, Chunk *chunk) {
        DCHECK_LT(i, kMaxRegionChunks);
        regions()[i] = DCHECK_NOTNULL(chunk);
    }
    
    Bitmap *bitmap() { return Bitmap::FromArray(bitmap_); }
    
    mutable Region region_; // free regions

    uint32_t bitmap_[kBitmapSize]; // usage bitmap
    static_assert(sizeof(Bitmap) == sizeof(bitmap_), "Fixed bitmap size.");
    
    uint8_t chunk_[0]; // chunk for allocation
}; // class Page


// Large object page
class LargePage final : public PageHeader {
public:
    DEF_VAL_GETTER(size_t, size);
    Address chunk() { return chunk_; }
    Address limit() { return reinterpret_cast<Address>(this) + size_; }
    
    static LargePage *FromAddress(Address addr) {
        return !addr ? nullptr : Cast(PageHeader::FromAddress(addr));
    }
    
    static LargePage *Cast(PageHeader *h) { return static_cast<LargePage *>(h); }
    
    friend class LargeSpace;
    friend class MetadataSpace;
    // For unit-tests:
    FRIEND_UNITTEST_CASE(PageTest, AllocateLargePage);

    DISALLOW_IMPLICIT_CONSTRUCTORS(LargePage);
private:
    LargePage(SpaceKind space, size_t size)
        : PageHeader(space)
        , size_(size) {
        available_ = static_cast<uint32_t>(size - sizeof(LargePage));
    }

    void Dispose(Allocator *lla) { lla->Free(this, size_); }
    
    static LargePage *New(SpaceKind space, int access, size_t size, Allocator *lla) {
        size_t request_size = RoundUp(sizeof(LargePage) + size, lla->granularity());
        void *chunk = AllocatePage(request_size, access, lla);
        if (!chunk) {
            return nullptr;
        }
        return new (chunk) LargePage(space, request_size);
    }

    size_t size_; // large object size
    uint8_t chunk_[0]; // chunk for allocation
}; // class LargePage


// Linear page for metadata
class LinearPage final : public PageHeader {
public:
    DEF_VAL_GETTER(Address, limit);
    
    size_t GetFreeSize() const { return limit_ - free_; }
    
    static bool IsLarge(size_t n) {
        return n > ((kPageSize - sizeof(LinearPage)) >> 1);
    }
    
    static LinearPage *Cast(PageHeader *h) { return static_cast<LinearPage *>(h); }
    
    friend class MetadataSpace;
private:
    LinearPage(SpaceKind space);
    
    void Dispose(Allocator *lla) { lla->Free(this, kPageSize); }
    
    static LinearPage *New(SpaceKind space, int access, Allocator *lla) {
        void *chunk = lla->Allocate(kPageSize);
        if (!chunk) {
            return nullptr;
        }
        lla->SetAccess(chunk, kPageSize, access);
        return new (chunk) LinearPage(space);
    }
    
    Address Advance(size_t size) {
        Address next = RoundUp(free_ + size, kAligmentSize);
        if (next >= limit()) {
            return nullptr;
        }
        Address chunk = free_;
        free_ = next;
        return chunk;
    }

    Address limit_;
    Address free_;
    uint8_t chunk_[0];
}; // class LinearPage

template<class Base>
inline void AllocationBitmap<Base>::MarkAllocated(ptrdiff_t offset, size_t size) {
    DCHECK_GE(size, 2 * kPointerSize);
    DCHECK_GE(offset, 0);
    DCHECK_EQ(0, offset % kPointerSize);
    size_t i = offset >> kPointerShift;
    DCHECK(!base()->Test(i));
    size_t j = i + (size >> kPointerShift) - 1;
    DCHECK(!base()->Test(j));
    base()->Set(i);
    base()->Set(j);
}

template<class Base>
inline void AllocationBitmap<Base>::ClearAllocated(ptrdiff_t offset) {
    DCHECK_GE(offset, 0);
    DCHECK_EQ(0, offset % kPointerSize);
    size_t i = offset >> kPointerShift;
    DCHECK(base()->Test(i));
    size_t j = base()->FindNextOne(i + 1);
    base()->Clear(i);
    base()->Clear(j);
}

template<class Base>
inline size_t AllocationBitmap<Base>::AllocatedSize(ptrdiff_t offset) const {
    DCHECK_GE(offset, 0);
    DCHECK_EQ(0, offset % kPointerSize);
    size_t i = offset >> kPointerShift;
    if (!base()->Test(i)) {
        return 0;
    }
    size_t j = base()->FindNextOne(i + 1);
    return (j - i + 1) << kPointerShift;
}


template<size_t N> inline size_t FixedAllocationBitmap<N>::FindNextOne(size_t b) const {
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

inline size_t RestrictAllocationBitmap::FindNextOne(size_t b) const {
    size_t x = b >> kBucketShift;
    if (uint32_t bits = bits_[x]; bits != 0) {
        for (size_t i = b & kBucketMask; i < 32; i++) {
            if (bits & (1u << i)) {
                return (x << kBucketShift) + i;
            }
        }
    }
    for (size_t i = x + 1; i < length_; i++) {
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
