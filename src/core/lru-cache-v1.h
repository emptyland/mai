#ifndef MAI_CORE_LRU_CACHE_V1_H_
#define MAI_CORE_LRU_CACHE_V1_H_

#include "core/hash-map-v2.h"
#include "base/arena.h"
#include "base/reference-count.h"
#include "mai/allocator.h"
#include "mai/error.h"
#include "mai/comparator.h"
#include <atomic>
#include <mutex>

namespace mai {
    
namespace core {
    
inline namespace v1 {
    
struct LRUHandle;

struct LRUHandle {
    using Deleter = void (std::string_view, void *);
    using Loader = Error (std::string_view, LRUHandle **, void *, void *);
    
    std::string_view key() const { return std::string_view(data, key_size); }
    
    bool is_deletion() const { return flags.load() & 0x1; }
    void set_deletion(bool val) { set_flags(val, 0x1); }
    void set_flags(bool val, uint32_t bits);
    
    void AddRef() { refs.fetch_add(1); }
    bool ReleaseRef() { return refs.fetch_sub(1) == 1; }
    int ref_count() const { return refs.load(); }
    
    static LRUHandle *New(std::string_view key, size_t value_size,
                          uint64_t id = 0);
    static void Free(LRUHandle *handle);


    LRUHandle *next;
    LRUHandle *prev;
    Deleter *deleter;
    std::atomic<int> refs;
    uintptr_t id; // User defined id
    void *value;
    size_t key_size;
    uint32_t hash_val;
    std::atomic<uint32_t> flags;
    std::atomic<int> hits;
    char data[1];
}; // struct LRUHandle

class LRUCacheShard final {
public:
    static const size_t kMinLRUTableSlots = 1237;
    
    LRUCacheShard(Allocator *ll_allocator, size_t capacity);
    ~LRUCacheShard();
    
    DEF_VAL_GETTER(size_t, capacity);
    DEF_VAL_GETTER(size_t, size);
    
    Error GetOrLoad(std::string_view key, base::intrusive_ptr<LRUHandle> *result,
                    LRUHandle::Deleter *deleter,
                    LRUHandle::Loader *loader,
                    void *arg0, void *arg1 = nullptr);
    
    void Insert(std::string_view key, LRUHandle *handle,
                LRUHandle::Deleter *deleter);
    
    LRUHandle *Get(std::string_view key);
    
    void Remove(std::string_view key);

    void PurgeIfNeeded(bool force);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(LRUCacheShard);
private:
    struct KeyComparator {
        const Comparator *cmp;
        int operator () (LRUHandle *lhs, LRUHandle *rhs) const {
            return cmp->Compare(lhs->key(), rhs->key());
        }
        uint32_t operator () (LRUHandle *lhs) const {
            return lhs->hash_val;
        }
    }; // struct Comparator
    
    using LRUTable = HashMap<LRUHandle *, KeyComparator>;
    using Arena = ::mai::base::Arena;
    
    struct TableBoundle : public base::ReferenceCounted<TableBoundle> {
        TableBoundle(Allocator *low_level_allocator,
                     size_t slots, KeyComparator cmp)
            : arena(low_level_allocator)
            , table(slots, cmp, &arena) {}
        float conflict_factor() const {
            return static_cast<float>(table.n_entries()) /
                   static_cast<float>(table.n_slots());
        }
        Arena    arena;
        LRUTable table;
    }; // struct TableBoundle
    
    struct LookupHandle;
    
    void LRU_Remove(LRUHandle *handle) {
        LRUHandle *prev = handle->prev;
        LRUHandle *next = handle->next;
        prev->next = next;
        next->prev = prev;
#if defined(DEBUG) || defined(_DEBUG)
        handle->prev = nullptr;
        handle->next = nullptr;
#endif // defined(DEBUG) || defined(_DEBUG)
    }
    
    void LRU_Insert(LRUHandle *list, LRUHandle *handle) {
        // add linked list
        handle->next = list;
        LRUHandle *prev = list->prev;
        handle->prev = prev;
        prev->next = handle;
        list->prev = handle;
    }
    
    const Comparator *const cmp_;
    base::atomic_intrusive_ptr<TableBoundle> boundle_;
    size_t capacity_;
    std::atomic<int> in_ordered_;
    
    size_t size_ = 0;
    LRUHandle lru_dummy_;
    LRUHandle *lru_ = &lru_dummy_;
    LRUHandle in_use_dummy_;
    LRUHandle *in_use_ = &in_use_dummy_;
    std::mutex mutex_;
}; // class LRUCache
    


class LRUCache final {
public:
    LRUCache(size_t max_shards, Allocator *ll_allocator, size_t capacity);
    ~LRUCache();
    
    DEF_VAL_GETTER(size_t, max_shards);
    
    Error GetOrLoad(std::string_view key, base::intrusive_ptr<LRUHandle> *result,
                    LRUHandle::Deleter *deleter,
                    LRUHandle::Loader *loader,
                    void *arg0, void *arg1 = nullptr) {
        return GetShard(HashKey(key))->GetOrLoad(key, result, deleter, loader,
                                                 arg0, arg1);
    }
    
    void Insert(std::string_view key, LRUHandle *handle,
                LRUHandle::Deleter *deleter) {
        GetShard(HashKey(key))->Insert(key, handle, deleter);
    }
    
    LRUHandle *Get(std::string_view key) {
        return GetShard(HashKey(key))->Get(key);
    }
    
    void Remove(std::string_view key) {
        return GetShard(HashKey(key))->Remove(key);
    }
    
    // Get or new a cache shard
    LRUCacheShard *GetShard(size_t idx);
    
    // Just get a cache shard
    const LRUCacheShard *GetShard(size_t idx) const;
    
    // Delete a cache shard by shard index
    bool Delete(size_t shard);

    void Purge(size_t idx);
    
    size_t HashNumber(uint64_t n) const { return n % max_shards_; }
    size_t HashKey(std::string_view key) const;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(LRUCache);

private:
    static const uintptr_t kPendingMask = 1u;
    static const uintptr_t kCreatedMask = ~kPendingMask;
    
    static LRUCacheShard *Get(const std::atomic<uintptr_t> *shard) {
        auto val = shard->load();
        return reinterpret_cast<LRUCacheShard *>(val);
    }
    
    static bool NeedInit(std::atomic<uintptr_t> *shard) {
        uintptr_t exp = 0;
        if (shard->compare_exchange_strong(exp, kPendingMask)) {
            return true;
        }
        while (shard->load(std::memory_order_acquire) == kPendingMask) {
            std::this_thread::yield();
        }
        return false;
    }
    
    void Install(std::atomic<uintptr_t> *shard) {
        auto inst = new LRUCacheShard(ll_allocator_, capacity_);
        auto val = reinterpret_cast<uintptr_t>(inst);
        shard->store(val, std::memory_order_release);
        
    }

    const size_t max_shards_;
    Allocator *const ll_allocator_;
    const size_t capacity_;
    std::atomic<uintptr_t> *shards_;
    
}; // class LRUCache
    
} // inline namespace v1
    
} // namespace core
    
    
} // namespace mai


#endif // MAI_CORE_LRU_CACHE_V1_H_
