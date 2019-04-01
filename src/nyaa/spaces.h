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
    
    bool Contains(Address addr) const { return addr >= area_base_ && addr < area_limit_; }
    
    // Only for regular page
    static inline Page *FromAddress(Address addr) {
        auto page = reinterpret_cast<Page *>(reinterpret_cast<uintptr_t>(addr) & kPageMask);
        DCHECK_EQ(kPageTag, page->tag_);
        return page;
    }
    
    // Only for regular page
    static Page *FromHeapObject(NyObject *ob) { return FromAddress(reinterpret_cast<Address>(ob)); }

    static Page *NewRegular(HeapSpace space, int scale, int access, Isolate *isolate);
    
    void Dispose(Isolate *isolate);
    
    struct Chunk {
        Chunk *next;
        size_t n;
    };
    
    struct Region {
        Chunk *tiny;    // [0, 64) bytes
        Chunk *small;   // [64, 512) bytes
        Chunk *medium;  // [512, 1024) bytes
        Chunk *normal;  // [1024, 4096) bytes
        Chunk *big;     // >= 4096 bytes
    };

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
        area_limit_ = area_base_ + page_size_;
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
class Space {
public:
    virtual ~Space() {}
    
    virtual size_t Available() const = 0;
    
    DEF_VAL_GETTER(HeapSpace, kind);
    DEF_VAL_GETTER(bool, executable);

    DISALLOW_IMPLICIT_CONSTRUCTORS(Space);
protected:
    Space(HeapSpace kind, bool executable = false)
        : kind_(kind)
        , executable_(executable) {}
    
    HeapSpace const kind_;
    bool const executable_;
}; // class Space

class SemiSpace : public Space {
public:
    SemiSpace() : Space(kSemiSpace) {}
    
    virtual ~SemiSpace() override {
        if (page_) { page_->Dispose(isolate_); }
    }
    
    virtual size_t Available() const override {
        DCHECK_EQ(page_->area_limit_ - free_, page_->available_);
        return page_->available_;
    }
    
    DEF_PTR_GETTER_NOTNULL(Page, page);
    
    Error Init(size_t size, Isolate *isolate);
    
    void *AllocateRaw(size_t size);

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
    
    bool Contains(Address addr) const {
        return from_area_->page()->Contains(addr) || to_area_->page()->Contains(addr);
    }
    
    Error Init(size_t total_size, Isolate *isolate);
    
    void *AllocateRaw(size_t size) { return to_area_->AllocateRaw(size); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NewSpace);
private:
    std::unique_ptr<SemiSpace> from_area_;
    std::unique_ptr<SemiSpace>   to_area_;
}; // class NewSpace

class OldSpace final : public Space {
public:
    OldSpace(bool executable, Isolate *isolate)
        : Space(kOldSpace, executable)
        , dummy_(new Page())
        , isolate_(DCHECK_NOTNULL(isolate)) {}
    virtual ~OldSpace() override;
    
    virtual size_t Available() const override;
    
    Error Init(size_t init_size, size_t limit_size);
    
    void *AllocateRaw(size_t size, HeapColor init_color);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(OldSpace);
private:
    Page *NewPage();
    
    Page *dummy_ = nullptr;
    Page *cache_ = nullptr;
    size_t init_size_ = 0;
    size_t limit_size_ = 0;
    size_t pages_total_size_ = 0;
    Isolate *isolate_ = nullptr;
}; // class OldSpace
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_SPACES_H_
