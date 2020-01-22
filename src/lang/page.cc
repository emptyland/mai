#include "lang/page.h"
#include "mai/allocator.h"
#include "glog/logging.h"

namespace mai {

namespace lang {

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
        lla->Free(base, prefix_size);
        request_size -= prefix_size;
    }
    
    if (size != request_size) {
        DCHECK_LT(size, request_size);
        size_t suffix_size = request_size - size;
        lla->Free(aligned_base + size, suffix_size);
        request_size -= suffix_size;
    }
    
    DCHECK_EQ(request_size, size);
    return static_cast<void*>(aligned_base);
}

/*static*/ void *PageHeader::AllocatePage(size_t size, int access, Allocator *lla) {
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
    return chunk;
}

const size_t Page::kRegionLimitSize[kMaxRegionChunks] = {
    64,
    512,
    1024,
    4096,
    SIZE_T_MAX,
};

Page::Page(SpaceKind space)
    : PageHeader(space) {
    ::memset(bitmap_, 0, sizeof(bitmap_[0]) * kBitmapSize);
    DbgFillInitZag(chunk_, kChunkSize);
        
    Address align_addr = RoundUp(chunk(), kAligmentSize);
    DCHECK_EQ(align_addr, chunk());

    Chunk *chunk = reinterpret_cast<Chunk *>(align_addr);
    chunk->next = nullptr;
    chunk->size = static_cast<uint32_t>(limit() - align_addr);
    available_ = chunk->size;
    
    size_t i = FindWantedRegion(chunk->size);
    DCHECK_LT(i, kMaxRegionChunks);
    region_array()[i] = chunk;
}

LinearPage::LinearPage(SpaceKind space)
    : PageHeader(space) {
    Address base = reinterpret_cast<Address>(this);
    limit_ = base + kPageSize;
    free_ = base + sizeof(LinearPage);
    DCHECK_EQ(0, reinterpret_cast<uintptr_t>(free_) % kAligmentSize);
    available_ = kPageSize - sizeof(LinearPage);
    DbgFillInitZag(free_, available_);
}

} // namespace lang

} // namespace mai
