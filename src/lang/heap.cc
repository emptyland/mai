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
    old_space_->Free(result.address(), true); // For allocation test!
    
    // TODO:
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
    
    if (flags & (kLarge | kPinned)) {
        // Use large_space_
        TODO();
        return AllocationResult(AllocationResult::NOTHING, nullptr);
    }
    
    if (flags & kOld) {
        auto result = old_space_->Allocate(size);
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

} // namespace lang

} // namespace mai

