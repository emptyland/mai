#include "nyaa/spaces.h"
#include "nyaa/nyaa-values.h"
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
    
const size_t Page::kRegionLimitSize[kMaxRegionChunks] = {
    64,
    512,
    1024,
    4096,
    SIZE_T_MAX,
};
    
///*static*/ Page *Page::NewRegular(HeapSpace space, int scale, int access, Isolate *isolate) {
//    Allocator *lla = isolate->env()->GetLowLevelAllocator();
//    DCHECK_GE(kPageSize, lla->granularity());
//    DCHECK_EQ(kPageSize % lla->granularity(), 0);
//    DCHECK_GT(scale, 0);
//    
//    size_t requried_size = kPageSize * scale;
//    
//    void *chunk = nullptr;
//    for (int i = 0; i < kAllocateRetries; ++i) {
//        if ((chunk = PageAllocate(requried_size, kPageSize, lla)) != nullptr) {
//            break;
//        }
//        DLOG(INFO) << "Retry page allocating: " << i;
//    }
//    if (!chunk) {
//        PLOG(INFO) << "Allocation fail!";
//        return nullptr; // Failed!
//    }
//    DCHECK_EQ(reinterpret_cast<uintptr_t>(chunk) % kPageSize, 0);
//
//    auto rs = lla->SetAccess(chunk, requried_size, access);
//    if (!rs) {
//        DLOG(ERROR) << rs.ToString();
//        return nullptr; // Failed!
//    }
//    return new (chunk) Page(space, static_cast<uint32_t>(requried_size));
//}
    
void Page::SetUpRegion() {
    DCHECK(region_ == nullptr);
    
    Address align_addr = RoundUp(area_base_, kAligmentSize);
    Chunk *chunk = reinterpret_cast<Page::Chunk *>(align_addr);
    chunk->next = nullptr;
    chunk->size = static_cast<uint32_t>(area_limit_ - align_addr);
    DCHECK_EQ(chunk->size, available_);
    
    region_ = new Region{};
    size_t i = Page::FindWantedRegion(chunk->size);
    DCHECK_LT(i, Page::kMaxRegionChunks);
    region_array()[i] = chunk;
}
    
/*static*/ Page *Page::NewVariable(HeapSpace space, size_t size, int access, Isolate *isolate) {
    Allocator *lla = isolate->env()->GetLowLevelAllocator();
    DCHECK_GE(kPageSize, lla->granularity());
    DCHECK_EQ(kPageSize % lla->granularity(), 0);
    
    size_t requried_size = RoundUp(size, kPageSize);
    
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
    
    auto rs = lla->SetAccess(chunk, requried_size, access);
    if (!rs) {
        DLOG(ERROR) << rs.ToString();
        return nullptr; // Failed!
    }
    return new (chunk) Page(space, static_cast<uint32_t>(requried_size));
}
    
void Page::Dispose(Isolate *isolate) {
    delete region_;
    region_ = nullptr;
    isolate->env()->GetLowLevelAllocator()->Free(this, page_size_);
}
    
HeapColor Space::GetAddressColor(Address addr) const {
    Page *page = Page::FromAddress(addr);
    DCHECK_EQ(kind(), page->owns_space());
    
#if defined(NYAA_USE_POINTER_COLOR)
    void *hp = *reinterpret_cast<void **>(addr);
    uintptr_t header = reinterpret_cast<uintptr_t>(hp);
    return static_cast<HeapColor>((header & NyObject::kColorMask) >> NyObject::kMaskBitsOrder);
#else // !defined(NYAA_USE_POINTER_COLOR)
    // TODO:
#endif // defined(NYAA_USE_POINTER_COLOR)
}

void Space::SetAddressColor(Address addr, HeapColor color) {
    Page *page = Page::FromAddress(addr);
    DCHECK_EQ(kind(), page->owns_space());
    
#if defined(NYAA_USE_POINTER_COLOR)
    void *hp = *reinterpret_cast<void **>(addr);
    uintptr_t header = reinterpret_cast<uintptr_t>(hp);
    header &= ~NyObject::kColorMask;
    header |= ((static_cast<uintptr_t>(color) & 0xfull) << NyObject::kMaskBitsOrder);
    *reinterpret_cast<void **>(addr) = reinterpret_cast<void *>(header);
#else // !defined(NYAA_USE_POINTER_COLOR)
    // TODO:
#endif // defined(NYAA_USE_POINTER_COLOR)
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

/*virtual*/ Address SemiSpace::AllocateRaw(size_t size) {
    Address align_free = RoundUp(free_, kAligmentSize);
    size_t  align_size = RoundUp(size, kAllocateAlignmentSize);
    if (align_free + align_size >= page_->area_limit_) {
        return nullptr;
    }
    page_->available_ -= align_free + align_size - free_;
    free_ = align_free + align_size;
    return align_free;
}
    
void SemiSpace::Iterate(Address begin, Address end, HeapVisitor *visitor) {
    DCHECK_GE(begin, page_->area_base());
    DCHECK_LE(end, page_->area_limit());
    DCHECK_LE(begin, end);
    Address addr = begin;
    while (addr < end) {
        NyObject *ob = reinterpret_cast<NyObject *>(addr);
        size_t size = ob->PlacedSize();
        DCHECK_EQ(size % kAllocateAlignmentSize, 0);
        
        visitor->VisitObject(this, ob, size);
        addr += size;
    }
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
    size_t size = 0;
    for (Page *i = dummy_->next_; i != dummy_; i = i->next_) {
        size += i->available();
    }
    return size;
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

/*virtual*/ Address OldSpace::AllocateRaw(size_t size) {
    DCHECK_NOTNULL(cache_);
    size = RoundUp(size, kAllocateAlignmentSize);
    
    Page *hit = cache_;
    if (size > cache_->available()) {
        for (hit = dummy_->next_; hit != dummy_; hit = hit->next_) {
            if (size <= hit->available()) {
                break;
            }
        }
        if (hit == dummy_) {
            if (pages_total_size_ + kPageSize > limit_size_) {
                return nullptr;
            }
            hit = NewPage();
        }
        cache_ = hit;
    }

    size_t idx = hit->FindFitRegion(size);
    DCHECK_LT(idx, Page::kMaxRegionChunks);
    Page::Chunk *chunk = hit->region_array()[idx];
    DCHECK_NOTNULL(chunk);
    
    Address raw = reinterpret_cast<Address>(chunk);
    DCHECK_GE(chunk->size, size);
    if (chunk->size - size > sizeof(Page::Chunk)) {
        Page::Chunk *next = chunk->next;
        size_t n = chunk->size;
        chunk = reinterpret_cast<Page::Chunk *>(raw + size);
        chunk->next = next;
        chunk->size = static_cast<uint32_t>(n - size);
        
        size_t wanted = Page::FindWantedRegion(chunk->size);
        if (idx != wanted) {
            hit->region_array()[idx] = next;    
            chunk->next = hit->region_array()[wanted];
        }
        hit->available_ -= size;
        hit->region_array()[wanted] = chunk;
    } else{
        hit->available_ -= chunk->size;
        hit->region_array()[idx] = chunk->next;
    }

    //SetAddressColor(raw, kColorWhite);
    return raw;
}
    
void OldSpace::Free(Address addr, size_t size, bool merge_slibing) {
    Page *page = Page::FromAddress(addr);
    DCHECK_EQ(kind(), page->owns_space());
    DCHECK_NE(kColorFreed, GetAddressColor(addr)) << "Free freed block: " << addr;
    
    DbgFillFreeZag(addr, size);
    Page::Chunk *chunk = reinterpret_cast<Page::Chunk *>(addr);
    chunk->size = static_cast<uint32_t>(size);

    if (merge_slibing && addr + size < page->area_limit_) {
        HeapColor slibing_color = GetAddressColor(addr + size);
        if (slibing_color == kColorFreed) {
            Page::Chunk *slibing = reinterpret_cast<Page::Chunk *>(addr + size);
            chunk->size = static_cast<uint32_t>(size + slibing->size);
            
            size_t i = page->FindFitRegion(slibing->size);
            DCHECK_LT(i, Page::kMaxRegionChunks);
            
            Page::Chunk dummy;
            dummy.next = page->region_array()[i];
            Page::Chunk *p = dummy.next, *prev = &dummy;
            DCHECK(p != nullptr);
            while (p) {
                if (p == slibing) {
                    prev->next = slibing->next;
                    break;
                }
                prev = p;
                p = p->next;
            }
            page->region_array()[i] = dummy.next;
        }
    }
    
    size_t wanted = Page::FindWantedRegion(chunk->size);
    DCHECK_LT(wanted, Page::kMaxRegionChunks);

    chunk->next = page->region_array()[wanted];
    page->region_array()[wanted] = chunk;

    page->available_ += size;
    if (page != cache_ && page->available() > cache_->available()) {
        cache_ = page;
    }
}
    
void OldSpace::MergeFreeChunks(Page *page) {
    // TODO:
}

void OldSpace::MergeAllFreeChunks() {
    Page *p = dummy_->next_;
    while (p != dummy_) {
        Page *next = p->next_;
        MergeFreeChunks(p);
        p = next;
    }
}
    
void OldSpace::Iterate(HeapVisitor *visitor) {
    for (Page *page = dummy_->next_; page != dummy_; page = page->next_) {
        Address p = RoundUp(page->area_base(), kAligmentSize);
        while (p < page->area_limit()) {
            HeapColor color = GetAddressColor(p);
            if (color == kColorFreed) {
                Page::Chunk *chunk = reinterpret_cast<Page::Chunk *>(p);
                p += chunk->size;
            } else {
                DCHECK(color == KColorGray || color == kColorWhite || color == kColorBlack);
                NyObject *ob = reinterpret_cast<NyObject *>(p);
                //printf("heap: %p(%d)\n", ob, color);
                size_t placed_size = ob->PlacedSize();
                visitor->VisitObject(this, ob, placed_size);
                p += placed_size;
            }
        }
    }
}
    
Page *OldSpace::NewPage() {
    int access = Allocator::kRd | Allocator::kWr;
    if (executable()) {
        access |= Allocator::kEx;
    }
    
    Page *page = Page::NewPaged(kind(), access, isolate_);
    if (!page) {
        return nullptr;
    }
    page->SetUpRegion();
//    Address align_addr = RoundUp(page->area_base(), kAligmentSize);
//    Page::Chunk *chunk = reinterpret_cast<Page::Chunk *>(align_addr);
//    chunk->next = nullptr;
//    chunk->size = static_cast<uint32_t>(page->area_limit_ - align_addr);
//    DCHECK_EQ(chunk->size, page->available());
//
//    page->region_ = new Page::Region{};
//    size_t i = Page::FindWantedRegion(chunk->size);
//    DCHECK_LT(i, Page::kMaxRegionChunks);
//    page->region_array()[i] = chunk;

    Page::Insert(dummy_, page);
    pages_total_size_ += page->page_size();
    return page;
}
    
/*virtual*/ LargeSpace::~LargeSpace() {
    while (dummy_->next_ != dummy_) {
        Page *x = dummy_->next_;
        Page::Remove(x);
        x->Dispose(isolate_);
    }
    delete dummy_;
}
    
/*virtual*/ size_t LargeSpace::Available() const {
    size_t size = 0;
    for (Page *i = dummy_->next_; i != dummy_; i = i->next_) {
        size += i->page_size();
    }
    return size;
}

Address LargeSpace::AllocateRaw(size_t size, bool executable) {
    size_t required_size = size + sizeof(Page) + kAligmentSize;
    if (pages_total_size_ + RoundUp(required_size, kPageSize) > limit_size_) {
        return nullptr;
    }

    int access = Allocator::kRd | Allocator::kWr;
    if (executable) {
        access |= Allocator::kEx;
    }
    Page *page = Page::NewVariable(kind(), required_size, access, isolate_);
    if (!page) {
        return nullptr;
    }
    Page::Insert(dummy_, page);
    pages_total_size_ += page->page_size();
    
    Address result = RoundUp(page->area_base(), kAligmentSize);
    //SetAddressColor(result, kColorWhite);
    return result;
}
    
void LargeSpace::Iterate(HeapVisitor *visitor) {
    Page *p = dummy_->next_;
    while (p != dummy_) {
        Page *next = p->next_;
        Address addr = RoundUp(p->area_base(), kAligmentSize);
        NyObject *ob = reinterpret_cast<NyObject *>(addr);
        visitor->VisitObject(this, ob, ob->PlacedSize());
        p = next;
    }
}
    
} // namespace nyaa
    
} // namespace mai
