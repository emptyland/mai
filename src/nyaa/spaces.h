#ifndef MAI_NYAA_SPACES_H_
#define MAI_NYAA_SPACES_H_

#include "nyaa/memory.h"
#include "base/base.h"
#include "mai/allocator.h"

namespace mai {
    
namespace nyaa {
    
class NyaaCore;
class Isolate;
class Object;
class NyObject;
class RootVisitor;
    
class Space;
class SemiSpace;
class NewSpace;
class OldSpace;
class LargeSpace;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Page
////////////////////////////////////////////////////////////////////////////////////////////////////
    
class Page final {
public:
    static constexpr const uint16_t kPageTag = 0x9a6e;
    static constexpr const uintptr_t kPageMask = ~((1lu << kPageShift) - 1);
    
    HeapSpace owns_space() const { return static_cast<HeapSpace>(owns_space_); }
    size_t available() const { return available_; }
    size_t page_size() const { return page_size_; }
    DEF_VAL_GETTER(Address, area_base);
    DEF_VAL_GETTER(Address, area_limit);
    size_t usable_size() const { return area_limit_ - area_base_; }
    
    bool Contains(Address addr) const { return addr >= area_base_ && addr < area_limit_; }
    
    // Only for regular page
    static inline Page *FromAddress(Address addr) {
        auto page = reinterpret_cast<Page *>(reinterpret_cast<uintptr_t>(addr) & kPageMask);
        DCHECK_EQ(kPageTag, page->tag_);
        return page;
    }
    
    // Only for regular page
    static Page *FromHeapObject(NyObject *ob) { return FromAddress(reinterpret_cast<Address>(ob)); }

    static Page *NewRegular(HeapSpace space, int scale, int access, Isolate *isolate) {
        DCHECK_GT(scale, 0);
        return NewVariable(space, kPageSize * scale, access, isolate);
    }
    
    static Page *NewPaged(HeapSpace space, int access, Isolate *isolate) {
        return NewRegular(space, 1, access, isolate);
    }
    
    static Page *NewVariable(HeapSpace space, size_t size, int access, Isolate *isolate);
    
    void Dispose(Isolate *isolate);
    
    struct Chunk {
        Chunk *next;
        uint32_t size;
    };
    
    struct Region {
        Chunk *tiny   = nullptr;  // [0, 64) bytes
        Chunk *small  = nullptr;  // [64, 512) bytes
        Chunk *medium = nullptr;  // [512, 1024) bytes
        Chunk *normal = nullptr;  // [1024, 4096) bytes
        Chunk *big    = nullptr;  // >= 4096 bytes
    };
    
    static constexpr const size_t kMaxRegionChunks = 5;
    static const size_t kRegionLimitSize[kMaxRegionChunks];
    
    DEF_PTR_PROP_RW_NOTNULL2(Region, region);
    Chunk **region_array() const { return reinterpret_cast<Chunk **>(region()); }
    
    void SetUpRegion();
    
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

    friend class SemiSpace;
    friend class NewSpace;
    friend class OldSpace;
    friend class LargeSpace;

    DISALLOW_IMPLICIT_CONSTRUCTORS(Page);
private:
    Page(int16_t owns_space, uint32_t page_size)
        : tag_(kPageTag)
        , owns_space_(owns_space)
        , page_size_(page_size) {
        DCHECK_GE(page_size, kPageSize);
        area_base_ = reinterpret_cast<Address>(this + 1);
        area_limit_ = reinterpret_cast<Address>(this) + page_size;
        available_ = static_cast<uint32_t>(area_limit_ - area_base_);
    }
    
    Page()
        : tag_(0)
        , owns_space_(0)
        , page_size_(sizeof(Page))
        , available_(0)
        , area_base_(nullptr)
        , area_limit_(nullptr) {}
    
    static void Insert(Page *head, Page *x) {
        x->next_ = head;
        auto *prev = head->prev_;
        x->prev_ = prev;
        prev->next_ = x;
        head->prev_ = x;
    }

    static void Remove(Page *x) {
        x->prev_->next_ = x->next_;
        x->next_->prev_ = x->prev_;
#if defined(DEBUG) || defined(_DEBUG)
        x->next_ = nullptr;
        x->prev_ = nullptr;
#endif
    }
    
    uint16_t tag_;
    int16_t owns_space_; // owner space
    uint32_t available_;
    uint32_t page_size_;
    Region *region_ = nullptr;
    Address area_base_;
    Address area_limit_;
    Page *next_ = this; // linked page
    Page *prev_ = this;
}; // class Page
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// Spaces
////////////////////////////////////////////////////////////////////////////////////////////////////
    
class HeapVisitor {
public:
    HeapVisitor() {}
    virtual ~HeapVisitor() {}
    virtual void VisitObject(Space *owns, NyObject *ob, size_t placed_size) = 0;
}; // class HeapVisitor
    
class Space {
public:
    using Visitor = HeapVisitor;
    
    virtual ~Space() {}
    
    virtual size_t Available() const = 0;
    
    virtual Address AllocateRaw(size_t size) = 0;
    
    DEF_VAL_GETTER(HeapSpace, kind);
    
    HeapColor GetAddressColor(Address addr) const;
    
    void SetAddressColor(Address addr, HeapColor color);

    DISALLOW_IMPLICIT_CONSTRUCTORS(Space);
protected:
    Space(HeapSpace kind) : kind_(kind) {}
    
    HeapSpace const kind_;
}; // class Space

class SemiSpace final : public Space {
public:
    SemiSpace() : Space(kSemiSpace) {}
    
    virtual ~SemiSpace() override {
        if (page_) {
            page_->Dispose(isolate_);
        }
    }
    
    virtual size_t Available() const override {
        DCHECK_EQ(page_->area_limit() - free_, page_->available());
        return page_->available();
    }
    
    virtual Address AllocateRaw(size_t size) override;
    
    size_t UsageMemory() const { return free_ - page_->area_base(); }
    
    bool Contains(Address addr) const { return page_->Contains(addr); }
    
    void Iterate(Address begin, Address end, HeapVisitor *visitor);
    
    DEF_PTR_GETTER_NOTNULL(Page, page);
    DEF_VAL_GETTER(Address, free);
    
    Error Init(size_t size, Isolate *isolate);
    
    void Purge() {
        DbgFillFreeZag(page_->area_base(), free_ - page_->area_base());
        free_ = page_->area_base();
        page_->available_ = static_cast<uint32_t>(page_->usable_size());
    }
    
    friend class NewSpace;
    friend class Scavenger;
    DISALLOW_IMPLICIT_CONSTRUCTORS(SemiSpace);
private:
    Page *page_  = nullptr;
    Address free_ = nullptr;
    Isolate *isolate_ = nullptr;
}; // class SemiSpace

class NewSpace final : public Space {
public:
    NewSpace()
        : Space(kNewSpace)
        , from_area_(new SemiSpace())
        , to_area_(new SemiSpace()) {}
    virtual ~NewSpace() override {}
    virtual size_t Available() const override { return to_area_->Available(); }
    virtual Address AllocateRaw(size_t size) override { return to_area_->AllocateRaw(size); }
    
    SemiSpace *from_area() const { return from_area_.get(); }
    SemiSpace *to_area() const { return to_area_.get(); }
    
    Error Init(size_t total_size, Isolate *isolate);
    
    bool Contains(Address addr) const {
        return from_area_->Contains(addr) || to_area_->Contains(addr);
    }

    void Purge(bool reinit) {
        from_area_.swap(to_area_);
        from_area_->Purge();
        if (reinit) {
            to_area_->Purge();
        }
    }
    
    friend class Scavenger;
    DISALLOW_IMPLICIT_CONSTRUCTORS(NewSpace);
private:
    std::unique_ptr<SemiSpace> from_area_;
    std::unique_ptr<SemiSpace>   to_area_;
}; // class NewSpace

class OldSpace final : public Space {
public:
    OldSpace(HeapSpace kind, bool executable, Isolate *isolate)
        : Space(kind)
        , executable_(executable)
        , dummy_(new Page())
        , isolate_(DCHECK_NOTNULL(isolate)) {}
    virtual ~OldSpace() override;
    virtual size_t Available() const override;
    virtual Address AllocateRaw(size_t size) override;
    
    DEF_PTR_GETTER(Page, cache);
    DEF_VAL_GETTER(bool, executable);
    
    Error Init(size_t init_size, size_t limit_size);
    
    bool Contains(Address addr) const {
        Page *page = Page::FromAddress(addr);
        return page->owns_space() == kind();
    }
    
    void Free(Address addr, size_t size, bool merge_slibing = true);
    
    void MergeFreeChunks(Page *page);
    void MergeAllFreeChunks();
    
    void Iterate(HeapVisitor *visitor);
    
    FRIEND_UNITTEST_CASE(NyaaSpacesTest, OldSpaceInit);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(OldSpace);
private:
    Page *NewPage();
    
    Isolate *isolate_;
    bool executable_;
    Page *dummy_;
    Page *cache_ = nullptr;
    size_t init_size_ = 0;
    size_t limit_size_ = 0;
    size_t pages_total_size_ = 0;
}; // class OldSpace
    
class LargeSpace final : public Space {
public:
    LargeSpace(size_t limit_size, Isolate *isolate)
        : Space(kLargeSpace)
        , dummy_(new Page())
        , limit_size_(limit_size)
        , isolate_(isolate) {}
    virtual ~LargeSpace() override;
    virtual size_t Available() const override;
    virtual Address AllocateRaw(size_t size) override { return AllocateRaw(size, false); }

    bool Contains(Address addr) const {
        Page *page = Page::FromAddress(addr);
        return page->owns_space() == kind();
    }
    
    Address AllocateRaw(size_t size, bool executable);
    
    void Free(Address addr) {
        Page *page = Page::FromAddress(addr);
        DCHECK_EQ(kind(), page->owns_space());
        Page::Remove(page);
        pages_total_size_ -= page->page_size();
        page->Dispose(isolate_);
    }
    
    void Iterate(HeapVisitor *visitor);
    
private:
    Page *dummy_;
    Isolate *const isolate_;
    size_t limit_size_;
    size_t pages_total_size_ = 0;
}; // class LargeSpace
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_SPACES_H_
