#include "base/zone.h"
#include "base/arena.h"
#include "glog/logging.h"
#include <thread>
#include <mutex>

namespace mai {
    
namespace base {
    
struct Zone::Page {
    std::atomic<Page*> next;
    union {
        size_t size;
        std::atomic<char*> free;
    };
};
    
struct Zone::ShadowPage {
    ShadowPage *next;
    union {
        size_t size;
        char  *free;
    };
};

template<class T>
static inline T *Align(T *x, size_t alignment) {
    return reinterpret_cast<T *>(RoundUp(reinterpret_cast<uintptr_t>(x),
                                         alignment));
}

class Zone::Core final {
public:
    static const size_t kHeaderSize = sizeof(Page);
    
    Core(Zone *zone);
    ~Core();
    
    size_t page_size() const { return owns_->page_size_; }
    
    size_t cache_size() const {
        return cache_size_.load(std::memory_order_acquire);
    }
    
    DEF_PTR_GETTER_NOTNULL(ArenaImpl, dummy);
    
    ShadowPage *NewLargePage(size_t n) {
        n += kHeaderSize;
        ShadowPage *page = static_cast<ShadowPage *>(ll_allocator_->Allocate(n));
        if (page) {
            page->size = n;
            page->next = nullptr;
        }
        return page;
    }

    ShadowPage *NewSmallPage();
    
    void FreeLargePage(ShadowPage *page) {
        auto size = page->size;
    #if defined(DEBUG) || defined(_DEBUG)
        Round32BytesFill(Arena::kFreeZag, page, size);
    #endif
        ll_allocator_->Free(page, size);
    }

    void ReclaimSmallPage(ShadowPage *page);
    
    inline void InsertArena(ArenaImpl *x);
    inline void RemoveArena(ArenaImpl *x);
  
    static_assert(sizeof(Zone::Page) == sizeof(Zone::ShadowPage),
                  "Page and shadow not has same size.");
private:
    ShadowPage *AllocSmallPage() {
        auto page = static_cast<ShadowPage *>(ll_allocator_->Allocate(page_size()));
        if (!page) {
            return nullptr;
        }
        page->next = nullptr;
        page->free = reinterpret_cast<char *>(page + 1);
        return page;
    }
    
    Zone *const owns_;
    Allocator *const ll_allocator_;
    std::atomic<Page *> cache_;
    std::atomic<size_t> cache_size_;
    ArenaImpl *dummy_;
    std::mutex dummy_mutex_;
}; // class Zone::Core
    
class Zone::ArenaImpl : public Arena {
public:
    static const size_t kHeaderSize = sizeof(Page);
    
    ArenaImpl(Zone::Core *owns, size_t limit_size)
        : owns_(owns)
        , limit_size_(limit_size)
        , memory_usage_(0) {}
    
    virtual ~ArenaImpl() {
        Purge(false);
        if (this != owns_->dummy()) {
            owns_->RemoveArena(this);
        }
    }
    
    DEF_PTR_GETTER_NOTNULL(Core, owns);
    DEF_VAL_PROP_RW(size_t, limit_size);
    
    virtual void Purge(bool /*reinit*/) override;
    virtual void *Allocate(size_t size, size_t alignment) override;
    virtual size_t memory_usage() const override { return memory_usage_; }
    
    ArenaImpl *next_ = this;
    ArenaImpl *prev_ = this;
private:
    void *AllocateSmall(size_t size, size_t alignment);
    
    void *AllocateLarge(size_t size, size_t alignment) {
        ShadowPage *page = owns_->NewLargePage(size + alignment);
        page->next = large_;
        large_ = page;
        return Align(static_cast<void *>(page + 1), alignment);
    }
    
    Zone::Core *const owns_;
    size_t limit_size_;
    size_t memory_usage_ = 0;
    ShadowPage *current_ = nullptr;
    ShadowPage *large_ = nullptr;
}; // class Zone::ArenaImpl
    
////////////////////////////////////////////////////////////////////////////////
/// class Zone::Core
////////////////////////////////////////////////////////////////////////////////
Zone::Core::Core(Zone *zone)
    : owns_(zone)
    , ll_allocator_(zone->ll_allocator_)
    , cache_(nullptr)
    , cache_size_(0)
    , dummy_(new ArenaImpl(this, 0)) {
}
    
Zone::Core::~Core() {
    DCHECK(dummy_->next_ == dummy_ && dummy_->prev_ == dummy_);
    delete dummy_;
    
    auto x = cache_.load(std::memory_order_relaxed);
    while (x) {
        auto p = x;
        x = x->next.load(std::memory_order_relaxed);
        ll_allocator_->Free(p, page_size());
    }
}
    
inline void Zone::Core::InsertArena(ArenaImpl *x) {
    std::lock_guard<std::mutex> lock(dummy_mutex_);
    x->next_ = dummy_;
    ArenaImpl *prev = dummy_->prev_;
    x->prev_ = prev;
    prev->next_ = x;
    dummy_->prev_ = x;
}
    
inline void Zone::Core::RemoveArena(ArenaImpl *x) {
    std::lock_guard<std::mutex> lock(dummy_mutex_);
    x->prev_->next_ = x->next_;
    x->next_->prev_ = x->prev_;
#if defined(DEBUG) || defined(_DEBUG)
    x->next_ = nullptr;
    x->prev_ = nullptr;
#endif
}
    
Zone::ShadowPage *Zone::Core::NewSmallPage() {
    auto page = cache_.load(std::memory_order_acquire);
    if (!page) {
        return AllocSmallPage();
    }
    auto expected = page;
    if (cache_.compare_exchange_strong(expected, page->next)) {
        cache_size_.fetch_sub(page_size());
        return reinterpret_cast<ShadowPage *>(page);
    } else {
        return AllocSmallPage();
    }
}
    
void Zone::Core::ReclaimSmallPage(ShadowPage *page) {
    if (cache_size_.load() >= owns_->max_cache_size_) {
#if defined(DEBUG) || defined(_DEBUG)
        Round32BytesFill(Arena::kFreeZag, page, page_size());
#endif
        ll_allocator_->Free(page, page_size());
        return;
    }
    Page *shadow = reinterpret_cast<Page *>(page);
    Page *head;
    do {
        head = cache_.load(std::memory_order_relaxed);
        shadow->next = head;
    } while (!cache_.compare_exchange_weak(head, shadow));
    
    cache_size_.fetch_add(page_size());
}

////////////////////////////////////////////////////////////////////////////////
/// class Zone::ArenaImpl
////////////////////////////////////////////////////////////////////////////////

/*virtual*/ void Zone::ArenaImpl::Purge(bool /*reinit*/) {
    ShadowPage *x = large_;
    while (x) {
        auto p = x;
        x = x->next;
        owns_->FreeLargePage(p);
    }
    large_ = nullptr;
    
    x = current_;
    while (x) {
        auto p = x;
        x = x->next;
        owns_->ReclaimSmallPage(p);
    }
    current_ = nullptr;
    memory_usage_ = 0;
}

/*virtual*/ void *Zone::ArenaImpl::Allocate(size_t size, size_t alignment) {
    alignment = std::max(granularity(), alignment);
    //size_t alloc_size = RoundUp(size, alignment);
    
    void *result = nullptr;
    if (size + alignment < owns_->page_size() - kHeaderSize) {
        result = AllocateSmall(size, alignment);
    } else {
        result = AllocateLarge(size, alignment);
    }
#if defined(DEBUG) || defined(_DEBUG)
    Round32BytesFill(kInitZag, result, size);
#endif
    return result;
}
    
void *Zone::ArenaImpl::AllocateSmall(size_t size, size_t alignment) {
    bool need_new_page = false;
    if (!current_) {
        need_new_page = true;
    } else {
        auto end = reinterpret_cast<char *>(current_) + owns_->page_size();
        
        current_->free = Align(current_->free, alignment);
        if (current_->free + size > end) {
            need_new_page = true;
        }
    }
    
    if (need_new_page) {
        ShadowPage *page = owns_->NewSmallPage();
        page->free = Align(page->free, alignment);
        page->next = current_;
        current_ = page;
        memory_usage_ += owns_->page_size();
    }
    
    DCHECK_EQ(0, reinterpret_cast<uintptr_t>(current_->free) % alignment);
    void *result = static_cast<void *>(current_->free);
    current_->free += size;
    return result;
}
    
    
////////////////////////////////////////////////////////////////////////////////
/// class Zone
////////////////////////////////////////////////////////////////////////////////
    
Zone::Zone(Allocator *ll_allocator, size_t cache_size)
    : ll_allocator_(DCHECK_NOTNULL(ll_allocator))
    , page_size_(ll_allocator->granularity() * 4)
    , max_cache_size_(cache_size)
    , core_(new Core(this)) {
    DCHECK_GE(ll_allocator->granularity(), 4 * base::kKB);
    DCHECK_EQ(0, ll_allocator->granularity() % 4 * base::kKB);

    DCHECK_GE(max_cache_size_, page_size_);
}

/*virtual*/ Zone::~Zone() {
}

Arena *Zone::NewArena(size_t limit_size, Arena *old_arena) {
    if (old_arena) {
        auto impl = dynamic_cast<ArenaImpl *>(old_arena);
        DCHECK(impl->owns() == core_.get());
        impl->Purge(true);
        impl->set_limit_size(limit_size);
        return old_arena;
    }
    auto impl = new ArenaImpl(core_.get(), limit_size);
    core_->InsertArena(impl);
    return impl;
}
    
Arena *Zone::default_arena() const { return core_->dummy(); }
    
size_t Zone::GetCacheSize() const { return core_->cache_size(); }
    
size_t Zone::GetTotalMameoryUsage() const {
    size_t usage = 0;
    auto x = core_->dummy();
    while (x) {
        usage += x->memory_usage();
        x = x->next_;
    }
    return usage;
}

/*virtual*/ void *Zone::Allocate(size_t size, size_t alignment) {
    return core_->dummy()->Allocate(size, alignment);
}

/*virtual*/ void Zone::Free(const void */*chunk*/, size_t /*size*/) {
}
    
/*virtual*/ size_t Zone::granularity() { return core_->dummy()->granularity(); }

    
} // namespace base
    
} // namespace mai
