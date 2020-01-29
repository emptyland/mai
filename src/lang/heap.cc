#include "lang/heap.h"
#include "lang/isolate-inl.h"

namespace mai {

namespace lang {

Heap::Heap(Allocator *lla)
    : new_space_(new NewSpace(lla))
    , old_space_(new OldSpace(lla))
    , large_space_(new LargeSpace(lla)) {
    // TODO:
}

Heap::~Heap() {
    // TODO:
}

Error Heap::Initialize(size_t new_space_initial_size) {
    if (auto err = new_space_->Initialize(new_space_initial_size); err.fail()) {
        return err;
    }
    
    AllocationResult result = old_space_->Allocate(1);
    if (!result.ok()) {
        return MAI_CORRUPTION("Old-Space allocation fail!");
    }
    old_space_->Free(result.address(), true); // For old-space allocation test!

    result = large_space_->Allocate(1);
    if (!result.ok()) {
        return MAI_CORRUPTION("Large-Space allocation fail!");
    }
    large_space_->Free(result.address()); // For large-space allocation test.
    return Error::OK();
}

AllocationResult Heap::Allocate(size_t size, uint32_t flags) {
    if (size == 0) {
        return AllocationResult(AllocationResult::NOTHING, nullptr);
    }
    
    size_t request_size = GetMinAllocationSize(size);
    if (Page::IsLarge(request_size)) {
        flags |= kLarge;
    }
    
    if (flags & kLarge) {
        auto result = large_space_->Allocate(size);
        if (result.ok()) {
            DbgFillInitZag(result.address(), size);
        }
        return result;
    }

    if (flags & (kOld | kMetadata)) {
        AllocationResult result;
        int retries = 2;
        while (retries--) {
            result = old_space_->Allocate(size);
            if (result.ok()) {
                DbgFillInitZag(result.address(), size);
                break;
            }
            if (result.result() == AllocationResult::OOM) {
                old_space_->ReclaimFreePages();
            } else {
                break;
            }
        }
        return result;
    }

    auto result = new_space_->Allocate(size);
    if (result.ok()) {
        DbgFillInitZag(result.address(), size);
    }
    return result;
}

Heap::TotalMemoryUsage Heap::ApproximateMemoryUsage() const {
    TotalMemoryUsage usage{0, 0};
    usage.rss = new_space_->GetRSS();
    usage.used = new_space_->GetUsedSize();
    {
        std::lock_guard<std::mutex> lock(old_space_->mutex_);
        usage.rss += old_space_->GetRSS();
        usage.used += old_space_->used_size();
    }
    {
        std::lock_guard<std::mutex> lock(large_space_->mutex_);
        usage.rss += large_space_->rss_size();
        usage.used += large_space_->used_size();
    }
    DCHECK_GT(usage.rss, usage.used);
    return usage;
}

} // namespace lang

} // namespace mai

