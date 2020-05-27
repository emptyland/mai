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
class SemiSpaceIterator;
class OldSpaceIterator;
class LargeSpaceIterator;

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


class SemiSpace final {
public:
    using Bitmap = RestrictAllocationBitmap;
    using Iterator = SemiSpaceIterator;

    static SemiSpace *New(size_t size, Allocator *lla);
    
    DEF_VAL_GETTER(size_t, size);
    DEF_VAL_GETTER(Address, chunk);
    DEF_VAL_GETTER(Address, limit);

    Address surive_level() const { return free(); }
    
    friend class Heap;
    friend class NewSpace;
    friend class SemiSpaceIterator;
    DISALLOW_IMPLICIT_CONSTRUCTORS(SemiSpace);
private:
    SemiSpace(size_t size);
    
    void Dispose(Allocator *lla) { lla->Free(this, size_); }
    
    inline size_t Purge();
    
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
    
    ALWAYS_INLINE Address AquireSpace(size_t size);

    const size_t size_;
    const Address limit_;
    const Address chunk_;
    std::atomic<Address> free_;
    uint32_t bitmap_length_;
    uint32_t bitmap_[0];
}; // class SemiSpace


class NewSpace : public Space {
public:
    NewSpace(Allocator *lla) : Space(kNewSpace, lla) {}
    
    ~NewSpace() {
        if (survive_area_) { survive_area_->Dispose(lla_); }
        if (original_area_) { original_area_->Dispose(lla_); }
    }
    
    Error Initialize(size_t initial_size);
    
    DEF_PTR_GETTER(SemiSpace, survive_area);
    DEF_PTR_GETTER(SemiSpace, original_area);
    
    bool InSuriveArea(Address addr) const { return survive_area_->Contains(addr); }
    
    bool InOriginalArea(Address addr) const { return original_area_->Contains(addr); }
    
    // Thread safe:
    inline AllocationResult Allocate(size_t size);
    
    ALWAYS_INLINE bool Contains(Address addr) const {
        return addr >=original_chunk() && addr < original_limit();
    }
    
    size_t GetAllocatedSize(Address addr) {
        DCHECK_GE(addr, original_area_->chunk());
        DCHECK_LT(addr, original_area_->limit());
        return original_area_->bitmap()->AllocatedSize(addr - original_area_->chunk());
    }
    
    Address original_chunk() const { return original_area_->chunk(); }
    
    Address original_limit() const { return original_area_->limit(); }
    
    size_t GetRSS() const { return survive_area_->size() + original_area_->size(); }
    
    size_t GetUsedSize() const { return original_area_->free() - original_area_->chunk(); }
    
    size_t Available() const { return original_area_->limit() - original_area_->free(); }
    
    inline size_t Flip(bool reinit);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NewSpace);
private:
    SemiSpace *survive_area_ = nullptr; // survive space
    SemiSpace *original_area_ = nullptr; // allocation space
}; // class NewSpace


class OldSpace final : public Space {
public:
    using Iterator = OldSpaceIterator;
    
    OldSpace(Allocator *lla, int max_freed_pages = 10);
    ~OldSpace();
    
    DEF_VAL_GETTER(int, allocated_pages);
    DEF_VAL_GETTER(int, freed_pages);
    DEF_VAL_GETTER(size_t, used_size);
    DEF_PTR_GETTER(PageHeader, dummy);
    
    size_t GetRSS() const { return kPageSize << allocated_pages_; }

    AllocationResult Allocate(size_t size) {
        std::lock_guard<std::mutex> lock(mutex_);
        return AllocateLockless(size);
    }

    // Free lock is not need.
    void Free(Address addr, bool merge);
    
    inline void PurgeIfNeeded();
    
    ALWAYS_INLINE bool Contains(Address addr);
    
    inline Address MoveObject(Address addr, size_t n, Page *dest);
    
    inline Page *GetOrNewFreePage();

    inline void FreePage(Page *page);
    
    inline void Compact(const std::vector<Page *> &pages, size_t used_size);

    friend class Heap;
    friend class OldSpaceIterator;
    DISALLOW_IMPLICIT_CONSTRUCTORS(OldSpace);
private:
    // Not thread safe:
    AllocationResult AllocateLockless(size_t size);
    
    static Address PageAllocate(Page *page, size_t request_size);
    
    inline void ReclaimFreePages();
    
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
    using Iterator = LargeSpaceIterator;
    
    LargeSpace(Allocator *lla)
        : Space(kLargeSpace, lla)
        , dummy_(new PageHeader(kDummySpace)) {}

    inline ~LargeSpace();
    
    DEF_VAL_GETTER(size_t, rss_size);
    DEF_VAL_GETTER(size_t, used_size);
    
    ALWAYS_INLINE bool Contains(Address addr);

    AllocationResult Allocate(size_t size) {
        std::lock_guard<std::mutex> lock(mutex_);
        return DoAllocate(size);
    }

    // Free lock is not need.
    inline void Free(Address addr);

    friend class Heap;
    friend class LargeSpaceIterator;
    DISALLOW_IMPLICIT_CONSTRUCTORS(LargeSpace);
private:
    // Not thread safe:
    AllocationResult DoAllocate(size_t size);
    
    PageHeader *dummy_; // Page double-linked list dummy
    size_t rss_size_ = 0; // OS Allocated bytes size
    size_t used_size_ = 0; // Allocated bytes size
    std::mutex mutex_; // Allocation mutex
}; // class LargeSpace


class PageSpaceIterator final {
public:
    PageSpaceIterator(Page *page): page_(page) {}

    void SeekToFirst() { SeekToFirst(page_); }

    inline void SeekToFirst(Page *page);

    bool Valid() const {
        return current_ != nullptr && current_ < page_->guard() && current_size_ > 0;
    }

    void Next() {
        current_ = page_->FindNextChunk(current_ + current_size_);
        if (current_ < page_->guard()) {
            UpdateSize();
        }
    }
    
    Address address() const { return current_; }

    Any *object() const { return reinterpret_cast<Any *>(current_); }

    size_t object_size() const { return current_size_; }

protected:
    void UpdateSize() { current_size_ = page_->AllocatedSize(current_); }
    
private:
    Page   *page_;
    Address current_ = nullptr;
    size_t  current_size_ = 0;
}; // class PageSpaceIterator


class PageIterator {
public:
    PageIterator(PageHeader *dummy): dummy_(DCHECK_NOTNULL(dummy)) {}
    
    void SeekToFirst() { page_ = dummy_->next(); }

    bool Valid() const { return page_ != dummy_; }

    void Next() { page_ = page_->next(); }

    template<class T>
    T *page() const { return static_cast<T *>(page_); }
private:
    PageHeader *const dummy_;
    PageHeader *page_ = nullptr;
}; // class PageIterator


class SemiSpaceIterator final {
public:
    SemiSpaceIterator(SemiSpace *owns) : owns_(owns) {}
    
    void SeekToFirst() {
        current_ = owns_->chunk();
        UpdateSize();
    }

    bool Valid() const {
        return current_ != nullptr && current_ < owns_->free() && current_size_ > 0;
    }

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

class OldSpaceIterator final {
public:
    OldSpaceIterator(OldSpace *owns)
        : page_(owns->dummy_)
        , iter_(nullptr) {}

    void SeekToFirst() {
        page_.SeekToFirst();
        MoveToValid();
    }
    
    bool Valid() const { return page_.Valid() && iter_.Valid(); }
    
    inline void Next();
    
    Address address() const { return iter_.address(); }

    Any *object() const { return iter_.object(); }

    size_t object_size() const { return iter_.object_size(); }
private:
    inline void MoveToValid();

    PageIterator page_;
    PageSpaceIterator iter_;
}; // class OldSpaceIterator

class LargeSpaceIterator final : public PageIterator {
public:
    LargeSpaceIterator(LargeSpace *owns): PageIterator(owns->dummy_) {}
    
    Any *object() const { return reinterpret_cast<Any *>(address()); }

    Address address() const { return page<LargePage>()->chunk(); }
    
    size_t object_size() const { return page<LargePage>()->size(); }
}; // class LargeSpaceIterator


inline size_t SemiSpace::Purge() {
    size_t remaining = free_.load(std::memory_order_relaxed) - chunk();
    DbgFillFreeZag(chunk(), remaining);
    free_.store(chunk(), std::memory_order_relaxed);
    ::memset(bitmap_, 0, sizeof(bitmap_[0]) * bitmap_length_);
    return remaining;
}

ALWAYS_INLINE Address SemiSpace::AquireSpace(size_t size) {
    size_t request_size = GetMinAllocationSize(size);
    Address space = free_.fetch_add(request_size);
    if (space + request_size >= limit_) {
        return nullptr;
    }
    bitmap()->MarkAllocated(space - chunk_, request_size);
    return space;
}

inline AllocationResult NewSpace::Allocate(size_t size) {
    DCHECK_GT(size, 0);
    Address space = DCHECK_NOTNULL(original_area_)->AquireSpace(size);
    if (!space) {
        return AllocationResult(AllocationResult::FAIL, nullptr);
    }
    return AllocationResult(AllocationResult::OK, space);
}

inline size_t NewSpace::Flip(bool reinit) {
    size_t remaining = original_area_->Purge();
    std::swap(survive_area_, original_area_);
    if (reinit) {
        original_area_->Purge();
    }
    return remaining;
}

inline void OldSpace::PurgeIfNeeded() {
    Page *prev = Page::Cast(dummy_);
    Page *page = Page::Cast(dummy_->next());
    while (page != dummy_) {
        if (page->IsEmpty()) {
            FreePage(page);
            page = Page::Cast(prev->next());
        } else {
            prev = page;
            page = Page::Cast(page->next());
        }
    }
}

inline void OldSpace::Compact(const std::vector<Page *> &pages, size_t used_size) {
    while (!QUEUE_EMPTY(dummy_)) {
        FreePage(Page::Cast(dummy_->next())); // Free all
    }
    for (auto page : pages) {
        QUEUE_INSERT_HEAD(dummy_, page);
    }
    used_size_ = used_size;
    allocated_pages_ = static_cast<int>(freed_pages_ + pages.size());
}

ALWAYS_INLINE bool OldSpace::Contains(Address addr) {
    if (!addr) {
        return false;
    }
    PageHeader *page = PageHeader::FromAddress(addr);
    return page->maker() == kPageMaker && page->owner_space() == kind();
}

inline Page *OldSpace::GetOrNewFreePage() {
    Page *free_page = Page::Cast(free_->next());
    if (free_page == free_) { // free list is empty
        free_page = Page::New(kind(), Allocator::kRd|Allocator::kWr, lla_);
        allocated_pages_++;
    }
    QUEUE_REMOVE(free_page);
    return free_page;
}

inline void OldSpace::FreePage(Page *page) {
    QUEUE_REMOVE(page);
    QUEUE_INSERT_HEAD(free_, page);
    if (freed_pages_ >= max_freed_pages_) {
        Page *tail = Page::Cast(free_->prev());
        QUEUE_REMOVE(tail);
        tail->Dispose(lla_);
    } else {
        page->Reinitialize();
        freed_pages_++;
    }
}

inline void OldSpace::ReclaimFreePages() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!QUEUE_EMPTY(free_)) {
        auto x = Page::Cast(free_->next());
        QUEUE_REMOVE(x);
        x->Dispose(lla_);
    }
    allocated_pages_ -= freed_pages_;
    freed_pages_ = 0;
}

inline LargeSpace::~LargeSpace() {
    while (!QUEUE_EMPTY(dummy_)) {
        auto x = dummy_->next();
        QUEUE_REMOVE(x);
        LargePage::Cast(x)->Dispose(lla_);
    }
    delete dummy_;
}

ALWAYS_INLINE bool LargeSpace::Contains(Address addr) {
    if (!addr) {
        return false;
    }
    PageHeader *page = PageHeader::FromAddress(addr);
    return page->maker() == kPageMaker && page->owner_space() == kind();
}

inline void LargeSpace::Free(Address addr) {
    DCHECK(Contains(addr));
    LargePage *page = LargePage::FromAddress(addr);
    DbgFillFreeZag(page->chunk(), page->available());
    QUEUE_REMOVE(page);
    rss_size_ -= page->size();
    used_size_ -= page->size();
    page->Dispose(lla_);
}


inline void PageSpaceIterator::SeekToFirst(Page *page) {
    page_ = page;
    current_ = page_->FindFirstChunk();
    if (current_ < page_->guard()) {
        UpdateSize();
    }
}

inline void OldSpaceIterator::Next() {
    DCHECK(Valid());
    iter_.Next();
    if (!iter_.Valid()) {
        page_.Next();
        MoveToValid();
    }
}

inline void OldSpaceIterator::MoveToValid() {
    while (page_.Valid()) {
        iter_.SeekToFirst(page_.page<Page>());
        if (iter_.Valid()) {
            break;
        }
        page_.Next();
    }
}

} // namespace lang

} // namespace mai

#endif // MAI_LANG_SPACE_H_
