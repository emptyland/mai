#include "base/arena.h"

namespace mai {
    
namespace base {
    
static uint32_t kInitZag = 0xcccccccc;
static uint32_t kFreeZag = 0xfeedfeed;
    
/*virtual*/ Arena::~Arena() {
    while (current_) {
        PageHead *x = current_;
        current_ = x->next.load();
#if defined(DEBUG) || defined(_DEBUG)
        Round32BytesFill(kFreeZag, x, kPageSize);
#endif
        low_level_alloc_->Free(x, kPageSize);
    }
    while (large_) {
        PageHead *x = large_;
        large_ = x->next.load();
        size_t page_size = x->u.size;
#if defined(DEBUG) || defined(_DEBUG)
        Round32BytesFill(kFreeZag, x, page_size);
#endif
        low_level_alloc_->Free(x, page_size);
    }
}

/*virtual*/ void *Arena::Allocate(size_t size, size_t alignment) {
    void *chunk;
    if (size >  kPageSize - sizeof(PageHead)) {
        chunk = NewLarge(size, alignment);
    } else {
        chunk = NewNormal(size, alignment);
    }
#if defined(DEBUG) || defined(_DEBUG)
    Round32BytesFill(kInitZag, chunk, size);
#endif
    return chunk;
}
    
} // namespace base
    
} // namespace mai
