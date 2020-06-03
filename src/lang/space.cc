#include "lang/space.h"

namespace mai {

namespace lang {

SemiSpace::SemiSpace(size_t size)
    : size_(size)
    , limit_(reinterpret_cast<Address>(this) + size)
    , chunk_(RoundUp(limit_ - GetChunkSize(), kAligmentSize))
    , free_(chunk_)
    , bitmap_length_(static_cast<uint32_t>(GetBitmapLength())) {
    ::memset(bitmap_, 0, sizeof(bitmap_[0]) * bitmap_length_);
}

/*static*/ SemiSpace *SemiSpace::New(size_t size, Allocator *lla) {
    size_t request_size = RoundUp(size, lla->granularity());
    void *chunk = lla->Allocate(request_size);
    if (!chunk) {
        return nullptr;
    }
    return new (chunk) SemiSpace(request_size);
}

Error NewSpace::Initialize(size_t initial_size) {
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

OldSpace::OldSpace(Allocator *lla, int max_freed_pages)
    : Space(kOldSpace, lla)
    , dummy_(new PageHeader(kDummySpace))
    , free_(new PageHeader(kDummySpace))
    , max_freed_pages_(max_freed_pages) {
}

OldSpace::~OldSpace() {
    while (!QUEUE_EMPTY(free_)) {
        auto x = Page::Cast(free_->next_);
        QUEUE_REMOVE(x);
        x->Dispose(lla_);
    }
    delete free_;
    
    while (!QUEUE_EMPTY(dummy_)) {
        auto x = Page::Cast(dummy_->next_);
        QUEUE_REMOVE(x);
        x->Dispose(lla_);
    }
    delete dummy_;
}

AllocationResult OldSpace::AllocateLockless(size_t size) {
    Page *page = Page::Cast(dummy_->next());
    if (page == dummy_) { // empty;
        page = GetOrNewFreePage();
        if (!page) {
            return AllocationResult(AllocationResult::OOM, nullptr);
        }
        QUEUE_INSERT_HEAD(dummy_, page);
    }

    size_t request_size = GetMinAllocationSize(size);
    if (request_size > page->available()) {
        page = GetOrNewFreePage();
        if (!page) {
            return AllocationResult(AllocationResult::OOM, nullptr);
        }
        QUEUE_INSERT_HEAD(dummy_, page);
    }
    used_size_ += request_size;
    Address chunk = PageAllocate(page, request_size);
    while (!chunk) {
        page = GetOrNewFreePage();
        if (!page) {
            return AllocationResult(AllocationResult::OOM, nullptr);
        }
        QUEUE_INSERT_HEAD(dummy_, page);
        chunk = PageAllocate(page, request_size);
    }
    return AllocationResult(AllocationResult::OK, chunk);
}

/*static*/ Address OldSpace::PageAllocate(Page *page, size_t request_size) {
    size_t fit = page->FindFitRegion(request_size, true/*complete*/);
    if (fit >= Page::kMaxRegionChunks) {
        return nullptr;
    }
    Page::Chunk *chunk = DCHECK_NOTNULL(page->region(fit));
    Address address = reinterpret_cast<Address>(chunk);
    DCHECK_GE(chunk->size, request_size);
    if (chunk->size - request_size > sizeof(Page::Chunk)) {
        Page::Chunk *next = chunk->next;
        size_t n = chunk->size;
        chunk = reinterpret_cast<Page::Chunk *>(address + request_size);
        chunk->next = next;
        chunk->size = static_cast<uint32_t>(n - request_size);
        
        size_t wanted = Page::FindWantedRegion(chunk->size);
        if (fit != wanted) {
            page->set_region(fit, next);
            chunk->next = page->region(wanted);
        }
        page->SubAvailable(request_size);
        page->set_region(wanted, chunk);
    } else{
        page->SubAvailable(chunk->size);
        page->set_region(fit, chunk->next);
    }
    
    page->bitmap()->MarkAllocated(address - page->chunk(), request_size);
    return address;
}

void OldSpace::Free(Address addr, bool merge) {
    Page *page = Page::FromAddress(addr);
    DCHECK_EQ(kind(), page->owner_space());

    const size_t size = page->bitmap()->AllocatedSize(addr - page->chunk());
    DbgFillFreeZag(addr, size);
    Page::Chunk *chunk = reinterpret_cast<Page::Chunk *>(addr);
    chunk->size = static_cast<uint32_t>(size);

    if (merge && addr + size < page->guard()) {
        if (!page->HasAllocated(addr + chunk->size)) {
            Page::Chunk *slibing = reinterpret_cast<Page::Chunk *>(addr + size);
            if (slibing->size > 0) {
                chunk->size += slibing->size;
                page->RemoveFitRegion(slibing->size, slibing);
            }
        }
    }
    page->InsertFitRegion(chunk->size, chunk);
    page->AddAvailable(size);
    used_size_ -= size;

    page->bitmap()->ClearAllocated(addr - page->chunk());
}

AllocationResult LargeSpace::DoAllocate(size_t size) {
    if (size == 0) {
        return AllocationResult(AllocationResult::NOTHING, nullptr);
    }
    LargePage *page = LargePage::New(kind(), Allocator::kRdWr, size, lla_);
    if (!page) {
        return AllocationResult(AllocationResult::OOM, nullptr);
    }
    QUEUE_INSERT_TAIL(dummy_, page);
    rss_size_ += page->size();
    used_size_ += page->size();
    DCHECK_EQ(0, reinterpret_cast<uintptr_t>(page->chunk()) % kAligmentSize);
    return AllocationResult(AllocationResult::OK, page->chunk());
}

} // namespace lang

} // namespace mai
