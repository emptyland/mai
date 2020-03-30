#pragma once
#ifndef MAI_LANG_SPACE_H_
#define MAI_LANG_SPACE_H_

#include "lang/page.h"
#include "glog/logging.h"
#define DBG_MACRO_NO_WARNING
#include "dbg/dbg.h"
#include <atomic>

namespace mai {

namespace lang {

class NewSpace;
class OldSpace;
class LargeSpace;
class SemiSpace;

class Space {
public:
    DEF_VAL_GETTER(SpaceKind, kind);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Space);
protected:
    Space(SpaceKind kind, Allocator *lla)
        : kind_(kind)
        , lla_(lla) {}

    SpaceKind kind_;
    Allocator *lla_;
}; // class Space


class SemiSpaceIterator;

class SemiSpace final {
public:
    using Bitmap = RestrictAllocationBitmap;
    using Iterator = SemiSpaceIterator;

    static SemiSpace *New(size_t size, Allocator *lla) {
        size_t request_size = RoundUp(size, lla->granularity());
        void *chunk = lla->Allocate(request_size);
        if (!chunk) {
            return nullptr;
        }
        return new (chunk) SemiSpace(request_size);
    }
    
    DEF_VAL_GETTER(size_t, size);
    DEF_VAL_GETTER(Address, chunk);
    DEF_VAL_GETTER(Address, limit);

    Address surive_level() const { return free(); }
    
    friend class Heap;
    friend class NewSpace;
    friend class SemiSpaceIterator;
    DISALLOW_IMPLICIT_CONSTRUCTORS(SemiSpace);
private:
    SemiSpace(size_t size)
        : size_(size)
        , limit_(reinterpret_cast<Address>(this) + size)
        , chunk_(RoundUp(limit_ - GetChunkSize(), kAligmentSize))
        , free_(chunk_)
        , bitmap_length_(static_cast<uint32_t>(GetBitmapLength())) {
        ::memset(bitmap_, 0, sizeof(bitmap_[0]) * bitmap_length_);
        //free_.store(RoundUp(limit_ - GetChunkSize(), kAligmentSize));
    }
    
    void Dispose(Allocator *lla) { lla->Free(this, size_); }
    
    size_t Purge() {
        size_t remaining = free_.load(std::memory_order_relaxed) - chunk();
        DbgFillFreeZag(chunk(), remaining);
        free_.store(chunk(), std::memory_order_relaxed);
        return remaining;
    }
    
    Bitmap *bitmap() { return Bitmap::FromSizeAddress(&bitmap_length_); }
    
    Address free() const { return free_.load(std::memory_order_relaxed); }
    
    size_t GetChunkSize() const {
        return ((size_ - sizeof(SemiSpace)) * 256) / 260;
    }
    
    size_t GetBitmapLength() const { return GetChunkSize() / 256 - 1; }
    
    ptrdiff_t GetFreeSize() const { return limit_ - free_.load(); }
    
    bool Contains(Address addr) const { return addr >= chunk() && addr < limit(); }
    
    size_t AllocatedSize(Address addr) {
        DCHECK(Contains(addr));
        return bitmap()->AllocatedSize(addr - chunk());
    }
    
    Address AquireSpace(size_t size) {
        size_t request_size = GetMinAllocationSize(size);
        Address space = free_.fetch_add(request_size);
        if (space + request_size >= limit_) {
            return nullptr;
        }
        bitmap()->MarkAllocated(space - chunk_, request_size);
        return space;
    }
    
    const size_t size_;
    const Address limit_;
    const Address chunk_;
    std::atomic<Address> free_;
    uint32_t bitmap_length_;
    uint32_t bitmap_[0];
}; // class SemiSpace


class SemiSpaceIterator final {
public:
    SemiSpaceIterator(SemiSpace *owns) : owns_(owns) {}
    
    void SeekToFirst() {
        current_ = owns_->chunk();
        UpdateSize();
    }

    bool Valid() const { return current_ != nullptr && current_ < owns_->free() && current_size_ > 0; }

    void Next() {
        DCHECK(Valid());
        current_ += current_size_;
        UpdateSize();
    }
    
    Any *object() const {
        DCHECK(Valid());
        return reinterpret_cast<Any *>(address());
    }
    
    Address address() const {
        DCHECK(Valid());
        return current_;
    }

    size_t object_size() const {
        DCHECK(Valid());
        return current_size_;
    }
    
private:
    void UpdateSize() {
        current_size_ = owns_->bitmap()->AllocatedSize(current_ - owns_->chunk());
    }
    
    SemiSpace *const owns_;
    Address current_ = nullptr;
    size_t  current_size_ = 0;
}; // class SemiSpaceIterator


class NewSpace : public Space {
public:
    NewSpace(Allocator *lla) : Space(kNewSpace, lla) {}
    
    ~NewSpace() {
        if (survive_area_) { survive_area_->Dispose(lla_); }
        if (original_area_) { original_area_->Dispose(lla_); }
    }
    
    Error Initialize(size_t initial_size) {
        DCHECK(survive_area_ == nullptr && original_area_ == nullptr);
        survive_area_ = SemiSpace::New(initial_size, lla_);
        if (!survive_area_) {
            return MAI_CORRUPTION("Not enough memory!");
        }
        original_area_ = SemiSpace::New(initial_size, lla_);
        if (!original_area_) {
            return MAI_CORRUPTION("Not enough memory!");
        }
        return Error::OK();
    }
    
    DEF_PTR_GETTER(SemiSpace, survive_area);
    DEF_PTR_GETTER(SemiSpace, original_area);
    
    bool InSuriveArea(Address addr) const { return survive_area_->Contains(addr); }
    
    bool InOriginalArea(Address addr) const { return original_area_->Contains(addr); }
    
    // Thread safe:
    AllocationResult Allocate(size_t size) {
        DCHECK_GT(size, 0);
        Address space = DCHECK_NOTNULL(original_area_)->AquireSpace(size);
        if (!space) {
            return AllocationResult(AllocationResult::FAIL, nullptr);
        }
        return AllocationResult(AllocationResult::OK, space);
    }
    
    ALWAYS_INLINE bool Contains(Address addr) const {
        return addr >=original_chunk() && addr < original_limit();
    }
    
    size_t GetAllocatedSize(Address addr) {
        DCHECK_GE(addr, original_area_->chunk());
        DCHECK_LT(addr, original_area_->limit());
        return original_area_->bitmap()->AllocatedSize(addr - original_area_->chunk());
    }
    
    SemiSpace::Iterator GetOriginalIterator() const { return SemiSpace::Iterator(original_area_); }
    
    Address original_chunk() const { return original_area_->chunk(); }
    
    Address original_limit() const { return original_area_->limit(); }
    
    size_t GetRSS() const { return survive_area_->size() + original_area_->size(); }
    
    size_t GetUsedSize() const { return original_area_->free() - original_area_->chunk(); }
    
    size_t Available() const { return original_area_->limit() - original_area_->free(); }
    
    size_t Flip(bool reinit) {
        std::swap(survive_area_, original_area_);
        size_t remaining = original_area_->Purge();
        if (reinit) {
            survive_area_->Purge();
        }
        return remaining;
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NewSpace);
private:
    SemiSpace *survive_area_ = nullptr; // survive space
    SemiSpace *original_area_ = nullptr; // allocation space
}; // class NewSpace

class OldSpace final : public Space {
public:
    OldSpace(Allocator *lla, int max_freed_pages = 10)
        : Space(kOldSpace, lla)
        , dummy_(new PageHeader(kDummySpace))
        , free_(new PageHeader(kDummySpace))
        , max_freed_pages_(max_freed_pages) {
    }
    ~OldSpace();
    
    DEF_VAL_GETTER(int, allocated_pages);
    DEF_VAL_GETTER(int, freed_pages);
    DEF_VAL_GETTER(size_t, used_size);
    
    size_t GetRSS() const { return kPageSize << allocated_pages_; }

    AllocationResult Allocate(size_t size) {
        std::lock_guard<std::mutex> lock(mutex_);
        return DoAllocate(size);
    }

    // Free lock is not need.
    void Free(Address addr, bool merge);
    
    bool Contains(Address addr) {
        if (!addr) {
            return false;
        }
        PageHeader *page = PageHeader::FromAddress(addr);
        return page->maker() == kPageMaker && page->owner_space() == kind();
    }
    
    friend class Heap;
    DISALLOW_IMPLICIT_CONSTRUCTORS(OldSpace);
private:
    // Not thread safe:
    AllocationResult DoAllocate(size_t size);
    
    Page *GetOrNewFreePage() {
        Page *free_page = Page::Cast(free_->next());
        if (free_page == free_) { // free list is empty
            //dbg("Allocate new page", allocated_pages_);
            free_page = Page::New(kind(), Allocator::kRd|Allocator::kWr, lla_);
            allocated_pages_++;
        }
        QUEUE_REMOVE(free_page);
        return free_page;
    }
    
    void ReclaimFreePages() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!QUEUE_EMPTY(free_)) {
            auto x = Page::Cast(free_->next());
            QUEUE_REMOVE(x);
            x->Dispose(lla_);
        }
        allocated_pages_ -= freed_pages_;
        freed_pages_ = 0;
    }
    
    PageHeader *dummy_; // Page double-linked list dummy
    PageHeader *free_;  // Free page double-linked list dummy
    const int max_freed_pages_; // Limit of freed pages
    int allocated_pages_ = 0; // Number of allocated pages
    int freed_pages_ = 0; // Number of free pages
    size_t used_size_ = 0; // Allocated bytes size
    std::mutex mutex_; // Allocation mutex
}; // class OldSpace


// Large and pinned object space
class LargeSpace : public Space {
public:
    LargeSpace(Allocator *lla)
        : Space(kLargeSpace, lla)
        , dummy_(new PageHeader(kDummySpace)) {}

    ~LargeSpace() {
        while (!QUEUE_EMPTY(dummy_)) {
            auto x = dummy_->next();
            QUEUE_REMOVE(x);
            LargePage::Cast(x)->Dispose(lla_);
        }
        delete dummy_;
    }
    
    DEF_VAL_GETTER(size_t, rss_size);
    DEF_VAL_GETTER(size_t, used_size);
    
    bool Contains(Address addr) {
        if (!addr) {
            return false;
        }
        PageHeader *page = PageHeader::FromAddress(addr);
        return page->maker() == kPageMaker && page->owner_space() == kind();
    }

    AllocationResult Allocate(size_t size) {
        std::lock_guard<std::mutex> lock(mutex_);
        return DoAllocate(size);
    }

    // Free lock is not need.
    void Free(Address addr) {
        DCHECK(Contains(addr));
        LargePage *page = LargePage::FromAddress(addr);
        DbgFillFreeZag(page->chunk(), page->available());
        QUEUE_REMOVE(page);
        rss_size_ -= page->size();
        used_size_ -= page->available();
        page->Dispose(lla_);
    }

    friend class Heap;
    DISALLOW_IMPLICIT_CONSTRUCTORS(LargeSpace);
private:
    // Not thread safe:
    AllocationResult DoAllocate(size_t size);
    
    PageHeader *dummy_; // Page double-linked list dummy
    size_t rss_size_ = 0; // OS Allocated bytes size
    size_t used_size_ = 0; // Allocated bytes size
    std::mutex mutex_; // Allocation mutex
}; // class OldSpace

} // namespace lang

} // namespace mai

#endif // MAI_LANG_SPACE_H_
