#include "base/arenas.h"
#include "base/slice.h"

namespace mai {
    
namespace base {
    
/*static*/ StandaloneArena::PageHead *const StandaloneArena::kBusyFlag
    = reinterpret_cast<StandaloneArena::PageHead *>(0x1);
    
/*virtual*/ StandaloneArena::~StandaloneArena() {
    Purge(false);
}

/*virtual*/ void *StandaloneArena::Allocate(size_t size, size_t alignment) {
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
    
void StandaloneArena::Purge(bool reinit) {
    while (current_) {
        PageHead *x = current_;
        current_ = x->next.load(std::memory_order_relaxed);
#if defined(DEBUG) || defined(_DEBUG)
        Round32BytesFill(kFreeZag, x, kPageSize);
#endif
        ::free(x);
    }
    while (large_) {
        PageHead *x = large_;
        large_ = x->next.load(std::memory_order_relaxed);
#if defined(DEBUG) || defined(_DEBUG)
        size_t page_size = x->u.size;
        Round32BytesFill(kFreeZag, x, page_size);
#endif
        ::free(x);
    }
    if (reinit) {
        current_.store(NewPage(kPageSize), std::memory_order_relaxed);
        memory_usage_.store(kPageSize, std::memory_order_relaxed);
    } else {
        memory_usage_.store(0, std::memory_order_relaxed);
    }
}
    
void *StandaloneArena::NewNormal(size_t size, size_t alignment) {
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
        
        memory_usage_.fetch_add(kPageSize);
    }
    return result;
}

void StandaloneArena::GetUsageStatistics(std::vector<Statistics> *normal,
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
