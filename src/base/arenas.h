#ifndef MAI_BASE_ARENAS_H_
#define MAI_BASE_ARENAS_H_

#include "base/arena.h"
#include "base/slice.h"
#include "glog/logging.h"
#include <atomic>
#include <thread>

namespace mai {
    
namespace base {
    
struct ArenaStatistics;
    
// All methods is thread safe and lock-free
class StandaloneArena final : public Arena {
public:
    static const int kPageSize = 16 * base::kKB;
    static const int kAlignment = sizeof(void *);
    
    StandaloneArena()
        : current_(NewPage(kPageSize))
        , large_(nullptr)
        , memory_usage_(kPageSize) /* The initialize size is one page.*/ {
    }
    
    virtual ~StandaloneArena() override;
    
    virtual void *Allocate(size_t size, size_t alignment = 4) override;

    virtual void Purge(bool reinit) override;

    virtual size_t memory_usage() const override {
        return memory_usage_.load();
    }
    
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
    
    void GetUsageStatistics(std::vector<Statistics> *normal, std::vector<Statistics> *large) const;
    
    //Allocator *ll_allocator() const { return ll_allocator_; }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(StandaloneArena);
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
        page->next.store(head->load(std::memory_order_acquire), std::memory_order_relaxed);
        head->store(page, std::memory_order_release);
    }

    PageHead *NewPage(size_t page_size) {
        PageHead *page = static_cast<PageHead *>(::malloc(page_size));
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
    
    //Allocator *const ll_allocator_;
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
    
    
class ScopedArena : public Arena {
public:
    ScopedArena(size_t limit = -1) : limit_(limit) {}
    virtual ~ScopedArena() override { Purge(false); }
    
    virtual void Purge(bool /*reinit*/) override;
    
    virtual size_t granularity() override { return 4; }
    
    virtual size_t memory_usage() const override { return usage_; }
    
    virtual void *Allocate(size_t size, size_t alignment = 4) override;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ScopedArena);
    
    FRIEND_UNITTEST_CASE(ArenaTest, ScopedLargeAllocation);
    FRIEND_UNITTEST_CASE(ArenaTest, ScopedTotalBufAllocation);
private:
    uint8_t buf_[64];
    size_t  usage_ = 0;
    size_t const limit_;
    std::vector<void *> chunks_;
}; // class ScopedArena
    

template<int N = 128>
class StaticArena : public Arena {
public:
    static constexpr const int kLimitSize = N;
    
    StaticArena() {
    #if defined(DEBUG) || defined(_DEBUG)
        Round32BytesFill(kInitZag, buf_, kLimitSize);
    #endif
    }

    virtual ~StaticArena() {
    #if defined(DEBUG) || defined(_DEBUG)
        Round32BytesFill(kFreeZag, buf_, kLimitSize);
    #endif
    }
    
    virtual void Purge(bool reinit = false) override {
        usage_ = 0;
    #if defined(DEBUG) || defined(_DEBUG)
        Round32BytesFill(kInitZag, buf_, kLimitSize);
    #endif
    }
    
    virtual size_t granularity() override { return 4; }
    
    virtual size_t memory_usage() const override { return usage_; }
    
    virtual void *Allocate(size_t size, size_t alignment = 4) override {
        uint8_t *p = RoundUp(buf_ + usage_, alignment);
        if (p + size > end_) {
            return nullptr;
        }
        usage_ = (p - buf_) + size;
        return p;
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(StaticArena);
private:
    uint8_t buf_[kLimitSize];
    const uint8_t *const end_ = buf_ + kLimitSize;
    size_t  usage_ = 0;
}; // class StaticArena
    
} // namespace base
    
} // namespace mai

#endif // MAI_BASE_ARENAS_H_
