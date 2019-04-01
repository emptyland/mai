#include "nyaa/spaces.h"
#include "mai/allocator.h"
#include "mai-lang/isolate.h"

namespace mai {
    
namespace nyaa {
    
static void *PageAllocate(size_t size, size_t alignment, Allocator *lla) {
    auto page_size = lla->granularity();
    DCHECK_EQ(0, size % page_size);
    DCHECK_EQ(0, alignment % page_size);
    
    size_t request_size = RoundUp(size + (alignment - page_size), page_size);
    auto result = lla->Allocate(request_size);
    if (!result) {
        return nullptr;
    }
    
    auto base = static_cast<Address>(result);
    auto aligned_base = RoundUp(base, alignment);
    if (aligned_base != base) {
        DCHECK_LT(base, aligned_base);
        auto prefix_size = aligned_base - base;
        //CHECK(PageFree(base, prefix_size));
        lla->Free(base, prefix_size);
        request_size -= prefix_size;
    }
    
    if (size != request_size) {
        DCHECK_LT(size, request_size);
        size_t suffix_size = request_size - size;
        //CHECK(PageFree(aligned_base + size, suffix_size));
        lla->Free(aligned_base + size, suffix_size);
        request_size -= suffix_size;
    }
    
    DCHECK_EQ(request_size, size);
    return static_cast<void*>(aligned_base);
}
    
/*static*/ Page *Page::NewRegular(HeapSpace space, int scale, int access, Isolate *isolate) {
    Allocator *lla = isolate->env()->GetLowLevelAllocator();
    DCHECK_GE(kPageSize, lla->granularity());
    DCHECK_EQ(kPageSize % lla->granularity(), 0);
    DCHECK_GT(scale, 0);
    
    size_t requried_size = kPageSize * scale;
    
    void *chunk = nullptr;
    for (int i = 0; i < kAllocateRetries; ++i) {
        if ((chunk = PageAllocate(requried_size, kPageSize, lla)) != nullptr) {
            break;
        }
        DLOG(INFO) << "Retry page allocating: " << i;
    }
    if (!chunk) {
        PLOG(INFO) << "Allocation fail!";
        return nullptr; // Failed!
    }
    DCHECK_EQ(reinterpret_cast<uintptr_t>(chunk) % kPageSize, 0);

    //DbgFillInitZag(chunk, kPageSize);
    auto rs = lla->SetAccess(chunk, requried_size, access);
    if (!rs) {
        DLOG(ERROR) << rs.ToString();
        return nullptr; // Failed!
    }
    return new (chunk) Page(space, static_cast<uint32_t>(requried_size));
}
    
void Page::Dispose(Isolate *isolate) {
    isolate->env()->GetLowLevelAllocator()->Free(this, page_size_);
}
    
Error SemiSpace::Init(size_t size, Isolate *isolate) {
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

void *SemiSpace::AllocateRaw(size_t size) {
    Address align_free = RoundUp(free_, kAligmentSize);
    size_t  align_size = RoundUp(size, kAllocateAlignmentSize);
    if (align_free + align_size >= page_->area_limit_) {
        return nullptr;
    }
    page_->available_ -= align_free + align_size - free_;
    free_ = align_free + align_size;
    return align_free;
}
    
Error NewSpace::Init(size_t total_size, Isolate *isolate) {
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
    
/*virtual*/ OldSpace::~OldSpace() {
    while (dummy_->next_ != dummy_) {
        Page *x = dummy_->next_;
        Page::Remove(x);
        x->Dispose(isolate_);
    }
    delete dummy_;
}

/*virtual*/ size_t OldSpace::Available() const {
    return 0;
}

Error OldSpace::Init(size_t init_size, size_t limit_size) {
    DCHECK_GT(init_size, kPageSize);
    DCHECK_GT(limit_size, init_size);
    
    init_size_ = init_size;
    limit_size_ = limit_size;

    size_t n_pages = (init_size_ + kPageSize - 1) / kPageSize;
    for (size_t i = 0; i < n_pages; ++i) {
        Page *page = NewPage();
        if (!page) {
            return MAI_CORRUPTION("Not enough memory!");
        }
    }

    cache_ = dummy_->next_;
    return Error::OK();
}

void *OldSpace::AllocateRaw(size_t size, HeapColor init_color) {
    return nullptr;
}
    
Page *OldSpace::NewPage() {
    int access = Allocator::kRd | Allocator::kWr;
    if (executable()) {
        access |= Allocator::kEx;
    }
    
    Page *page = Page::NewRegular(kOldSpace, 1, access, isolate_);
    if (!page) {
        return nullptr;
    }
    Page::Insert(dummy_, page);
    pages_total_size_ += page->page_size();
    return page;
}
    
} // namespace nyaa
    
} // namespace mai
