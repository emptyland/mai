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
    
    uint16_t tag_;
    int16_t owns_space_; // owner space
    uint32_t available_;
    uint32_t page_size_;
    Address area_base_;
    Address area_limit_;
    //Address free_;
    //Bitmap *bitmap_ = nullptr;
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
    Space(HeapSpace kind) : kind_(kind) {}
    
    HeapSpace const kind_;
    bool executable_ = false;
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
    
    Error Init(size_t size, Isolate *isolate) {
        isolate_ = DCHECK_NOTNULL(isolate);
        const size_t required_size = RoundUp(size, kPageSize);
        page_ = Page::NewRegular(kind(), static_cast<int>(required_size >> kPageShift),
                                 Allocator::kRd | Allocator::kWr, isolate_);
        if (!page_) {
            return MAI_CORRUPTION("Not enough memory.");
        } else {
            free_ = page_->area_base_;
            return Error::OK();
        }
    }
    
    void *AllocateRaw(size_t size) {
        Address align_free = RoundUp(free_, kAligmentSize);
        size_t  align_size = RoundUp(size, kAllocateAlignmentSize);
        if (align_free + align_size >= page_->area_limit_) {
            return nullptr;
        }
        page_->available_ -= align_free + align_size - free_;
        free_ = align_free + align_size;
        return align_free;
    }
private:
    Page *page_  = nullptr;
    Address free_ = nullptr;
    Isolate *isolate_ = nullptr;
}; // class SemiSpace

class NewSpace : public Space {
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
    
    Error Init(size_t total_size, Isolate *isolate) {
        auto rs = from_area_->Init(total_size >> 1, isolate);
        if (!rs) {
            return rs;
        }
        rs = to_area_->Init(total_size >> 1, isolate);
        if (!rs) {
            return rs;
        }
        // TODO:;
        return Error::OK();
    }
    
    void *AllocateRaw(size_t size) { return to_area_->AllocateRaw(size); }
    
private:
    std::unique_ptr<SemiSpace> from_area_;
    std::unique_ptr<SemiSpace>   to_area_;
    
}; // class NewSpace
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_SPACES_H_
