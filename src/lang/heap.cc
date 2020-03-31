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
    // For testing, if trap is setting, should return the fake error
    if (test_trap_ != AllocationResult::OK && test_trap_count_-- == 0) {
        // Reset trap setting
        AllocationResult::Result trap = test_trap_;
        test_trap_ = AllocationResult::OK;
        test_trap_count_ = 0;
        return AllocationResult(trap, nullptr);
    }
    
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
        AllocationResult result = old_space_->Allocate(size);
        if (result.ok()) {
            DbgFillInitZag(result.address(), size);
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

Any *Heap::MoveNewSpaceObject(Any *object, bool promote) {
    SemiSpace *original_area = new_space_->original_area();
    DCHECK(original_area->Contains(reinterpret_cast<Address>(object)));
    size_t placed_size = original_area->AllocatedSize(reinterpret_cast<Address>(object));
    
    Address dest = nullptr;
    if (promote) {
        auto rv = old_space_->Allocate(placed_size);
        if (!rv.ok()) {
            return nullptr;
        }
        dest = rv.address();
    } else {
        SemiSpace *survie_area = new_space_->survive_area();
        dest = survie_area->AquireSpace(placed_size);
        if (!dest) {
            return nullptr;
        }
    }
    ::memcpy(dest, object, placed_size);
    object->set_forward_address(dest);
    return reinterpret_cast<Any *>(dest);
}

} // namespace lang

} // namespace mai

