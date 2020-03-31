#include "lang/space.h"

namespace mai {

namespace lang {

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

AllocationResult OldSpace::DoAllocate(size_t size) {
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

    size_t fit = page->FindFitRegion(request_size);
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
    
    used_size_ += request_size;
    page->bitmap()->MarkAllocated(address - page->chunk(), request_size);
    return AllocationResult(AllocationResult::OK, address);
}

void OldSpace::Free(Address addr, bool merge) {
    Page *page = Page::FromAddress(addr);
    DCHECK_EQ(kind(), page->owner_space());
    //DCHECK_NE(kColorFreed, GetAddressColor(addr)) << "Free freed block: " << addr;

    const size_t size = page->bitmap()->AllocatedSize(addr - page->chunk());
    DbgFillFreeZag(addr, size);
    Page::Chunk *chunk = reinterpret_cast<Page::Chunk *>(addr);
    chunk->size = static_cast<uint32_t>(size);

    if (merge && addr + size < page->limit()) {
        bool allocated = page->HasAllocated(addr + chunk->size);
        if (!allocated) {
            Page::Chunk *slibing = reinterpret_cast<Page::Chunk *>(addr + size);
            chunk->size = static_cast<uint32_t>(size + slibing->size);
            page->RemoveFitRegion(slibing->size, slibing);
        }
    }
    page->InsertFitRegion(size, chunk);
    page->AddAvailable(size);
    used_size_ -= size;

    if (page->IsEmpty()) {
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
    } else {
        Page *head = Page::Cast(dummy_->next());
        if (page != head && page->available() > head->available()) {
            QUEUE_REMOVE(page);
            QUEUE_INSERT_HEAD(dummy_, page);
        }
        page->bitmap()->ClearAllocated(addr - page->chunk());
    }
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
    used_size_ += size;
    DCHECK_EQ(0, reinterpret_cast<uintptr_t>(page->chunk()) % kAligmentSize);
    return AllocationResult(AllocationResult::OK, page->chunk());
}

} // namespace lang

} // namespace mai
