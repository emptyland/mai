#include "lang/space.h"

namespace mai {

namespace lang {

/*static*/ SemiSpace *SemiSpace::New(size_t size, Allocator *lla) {
    size = RoundUp(size, lla->granularity());
    void *chunk = lla->Allocate(size);
    if (!chunk) {
        return nullptr;
    }
    return new (chunk) SemiSpace(size);
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
        page->available_ -= request_size;
        page->set_region(wanted, chunk);
    } else{
        page->available_ -= chunk->size;
        page->set_region(fit, chunk->next);
    }
    
    used_size_ += request_size;
    page->bitmap()->MarkAllocated(address - page->chunk(), request_size);
    return AllocationResult(AllocationResult::OK, address);
}

void OldSpace::DoFree(Address addr, bool merge) {
    TODO();
}

} // namespace lang

} // namespace mai
