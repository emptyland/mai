#include "base/scoped-arena.h"

namespace mai {

namespace base {
    
/*virtual*/ void ScopedArena::Purge(bool /*reinit*/) {
    for (auto chunk : chunks_) {
        ::free(chunk);
    }
    chunks_.clear();
    usage_ = 0;
}
    
/*virtual*/ void *ScopedArena::Allocate(size_t size, size_t /*alignment*/) {
    if (usage_ + size > limit_) {
        return nullptr;
    }
    void *rv;
    if (usage_ + size <= arraysize(buf_)) {
        rv = &buf_[usage_];
    } else {
        rv = ::malloc(size);
        chunks_.push_back(rv);
    }
    usage_ += size;
    return rv;
}
    
} // namespace base
    
} // namespace mai
