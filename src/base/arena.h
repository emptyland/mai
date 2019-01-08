#ifndef MAI_BASE_ARENA_H_
#define MAI_BASE_ARENA_H_

#include "base/base.h"
#include "mai/allocator.h"
#include "glog/logging.h"
#include <atomic>
#include <thread>

namespace mai {
    
namespace base {
    
struct ArenaStatistics;
    
// All methods is thread safe and lock-free
class Arena final : public Allocator {
public:
    static const int kPageSize = 16 * base::kKB;
    static const int kAlignment = sizeof(void *);
    
    static const uint32_t kInitZag;
    static const uint32_t kFreeZag;
    
    Arena(Allocator *low_level)
        : low_level_alloc_(DCHECK_NOTNULL(low_level))
        , current_(NewPage(kPageSize))
        , large_(nullptr)
        , memory_usage_(kPageSize) // The initialize size is one page.
    {
    }
    
    virtual ~Arena() override;
    
    template<class T> T *New() { new (Allocate(sizeof(T))) T(); }
    
    template<class T> T *NewArray(size_t n) {
        return static_cast<T *>(Allocate(sizeof(T) * n));
    }
    
    virtual void *Allocate(size_t size, size_t alignment = sizeof(max_align_t)) override;
    
    virtual void Free(const void */*chunk*/, size_t /*size*/) override {}
    
    void Purge(bool reinit);
    
    void *NewLarge(size_t size, size_t alignment) {
        size_t alloc_size = RoundUp(sizeof(PageHead) + size, kPageSize);
        PageHead *page = NewPage(alloc_size);
        if (!page) {
            return nullptr;
        }
        page->u.size = alloc_size;
        Link(&large_, page);
        memory_usage_.fetch_add(alloc_size);
        return static_cast<void *>(page + 1);
    }
    
    void *NewNormal(size_t size, size_t alignment);

    using Statistics = ArenaStatistics;
    
    void GetUsageStatistics(std::vector<Statistics> *normal,
                            std::vector<Statistics> *large) const;
    
    Allocator *low_level_allocator() const { return low_level_alloc_; }
    
    size_t memory_usage() const { return memory_usage_.load(); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Arena);
private:
    struct PageHead {
        std::atomic<PageHead*> next;
        union {
            size_t size;
            std::atomic<char*> free;
        } u;
    };
    
    static PageHead *const kBusyFlag;
    
    void Link(std::atomic<PageHead*> *head, PageHead *page) {
        page->next.store(head->load(std::memory_order_acquire),
                         std::memory_order_relaxed);
        head->store(page, std::memory_order_release);
    }

    PageHead *NewPage(size_t page_size) {
        PageHead *page =
            static_cast<PageHead *>(low_level_alloc_->Allocate(page_size,
                                                               kAlignment));
        page->next = nullptr;
        page->u.free = reinterpret_cast<char *>(page + 1);
        return page;
    }
    
    bool TestFull(PageHead *page, size_t size) {
        char *limit = reinterpret_cast<char *>(page) + kPageSize;
        if (page->u.free + size > limit) {
            return true;
        } else {
            return false;
        }
    }
    
    Allocator *const low_level_alloc_;
    std::atomic<PageHead*> current_;
    std::atomic<PageHead*> large_;
    std::atomic<size_t> memory_usage_;
}; // class Arena
    
struct ArenaStatistics {
    const char *addr;
    const char *bound_begin;
    const char *bound_end;
    size_t      usage;
    double      used_rate;
}; // struct ArenaStatistics
    
} // namespace base
    
} // namespace mai


#endif // MAI_BASE_ARENA_H_
