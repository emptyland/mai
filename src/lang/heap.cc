#include "lang/heap.h"
#include "lang/isolate-inl.h"

namespace mai {

namespace lang {

Heap::Heap(Allocator *lla)
    : new_space_(new NewSpace(lla))
    , old_space_(new OldSpace(lla))
    , large_space_(new LargeSpace(lla)) {
}

Heap::~Heap() {
    
}

AllocationResult Heap::Allocate(size_t size, uint32_t tags) {
    TODO();
    return AllocationResult(AllocationResult::NOTHING, nullptr);
}

} // namespace lang

} // namespace mai

