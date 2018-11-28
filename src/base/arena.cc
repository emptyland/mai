#include "base/arena.h"

namespace mai {
    
namespace base {
    
/*static*/ const uint32_t Arena::kInitZag = 0xcccccccc;
/*static*/ const uint32_t Arena::kFreeZag = 0xfeedfeed;
    
/*static*/ Arena::PageHead *const Arena::kBusyFlag
    = reinterpret_cast<Arena::PageHead *>(0x1);
    
/*virtual*/ Arena::~Arena() {
    while (current_) {
        PageHead *x = current_;
        current_ = x->next;
#if defined(DEBUG) || defined(_DEBUG)
        Round32BytesFill(kFreeZag, x, kPageSize);
#endif
        low_level_alloc_->Free(x, kPageSize);
    }
    while (large_) {
        PageHead *x = large_;
        large_ = x->next;
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
    
void *Arena::NewNormal(size_t size, size_t alignment) {
    PageHead *page;
    while ((page = current_.load(std::memory_order_acquire)) == kBusyFlag) {
        std::this_thread::yield();
    }
    size_t alloc_size = RoundUp(size, alignment);
    const char *const limit = reinterpret_cast<const char *>(page) + kPageSize;
    char *result = page->u.free.fetch_add(alloc_size);
    if (result + alloc_size > limit) {
        current_.store(kBusyFlag, std::memory_order_release);
        PageHead *root = page;
        page = NewPage(kPageSize);
        if (!page) {
            return nullptr;
        }
        page->next = root;
        result = page->u.free.fetch_add(alloc_size);
        current_.store(page, std::memory_order_relaxed);
        root->u.free -= alloc_size;
    }
    return result;
}
    
void Arena::GetUsageStatistics(std::vector<Statistics> *normal,
                               std::vector<Statistics> *large) const {
    
    if (normal) {
        normal->clear();
        for (PageHead *pg = current_.load(); pg != nullptr; pg = pg->next) {
            Statistics s;
            s.addr = reinterpret_cast<const char *>(pg);
            s.bound_begin = s.addr + sizeof(PageHead);
            s.usage       = pg->u.free.load() - s.bound_begin;
            s.bound_end   = s.bound_begin + s.usage;
            s.used_rate   = static_cast<double>(s.usage) /
                            static_cast<double>(kPageSize - sizeof(PageHead));
            normal->push_back(s);
        }
    }
    if (large) {
        large->clear();
        for (PageHead *pg = large_.load(); pg != nullptr; pg = pg->next) {
            Statistics s;
            s.addr = reinterpret_cast<const char *>(pg);
            s.bound_begin = s.addr + sizeof(PageHead);
            s.usage       = pg->u.size;
            s.bound_end   = s.bound_begin + s.usage;
            s.used_rate   = 1.0f;
            large->push_back(s);
        }
    }
}
    
} // namespace base
    
} // namespace mai
