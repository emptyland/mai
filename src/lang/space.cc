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

} // namespace lang

} // namespace mai
