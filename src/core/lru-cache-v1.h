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
    uint64_t id; // User defined id
    void *value;
    size_t key_size;
    uint32_t hash_val;
    std::atomic<uint32_t> flags;
    std::atomic<int> hits;
    char data[1];
}; // struct LRUHandle

class LRUCache final {
public:
    static const size_t kMinLRUTableSlots = 1237;
    
    LRUCache(Allocator *low_level_allocator, size_t capacity);
    ~LRUCache();
    
    DEF_VAL_GETTER(size_t, capacity);
    DEF_VAL_GETTER(size_t, size);
    
    Error GetOrLoad(std::string_view key, base::intrusive_ptr<LRUHandle> *result,
                    LRUHandle::Deleter *deleter,
                    LRUHandle::Loader *loader,
                    void *arg0, void *arg1 = nullptr);
    
    template<class Callable>
    inline Error GetOrLoad(std::string_view key,
                           base::intrusive_ptr<LRUHandle> *result,
                           LRUHandle::Deleter *deleter,
                           Callable &&loader);
    
    void Insert(std::string_view key, LRUHandle *handle,
                LRUHandle::Deleter *deleter);
    
    LRUHandle *Get(std::string_view key);
    
    void Remove(std::string_view key) {
        LRUHandle *x = Get(key);
        if (x && x->ref_count() == 1) {
            x->set_deletion(true);
        }
    }

    void PurgeIfNeeded(bool force);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(LRUCache);
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
    
    
template<class Callable>
inline Error LRUCache::GetOrLoad(std::string_view key,
                                 base::intrusive_ptr<LRUHandle> *result,
                                 LRUHandle::Deleter *deleter,
                                 Callable &&loader) {
    LRUHandle *handle = Get(key);
    if (handle) {
        result->reset(handle);
        return Error::OK();
    }
    Error rs = loader(key, &handle);
    if (!rs) {
        return rs;
    }
    handle->AddRef();
    Insert(key, handle, deleter);
    PurgeIfNeeded(false);
    result->reset(handle);
    return Error::OK();
}
    
} // inline namespace v1
    
} // namespace core
    
    
} // namespace mai


#endif // MAI_CORE_LRU_CACHE_V1_H_
