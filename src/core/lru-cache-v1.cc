#include "core/lru-cache-v1.h"
#include "base/allocators.h"
#include "base/hash.h"
#include "base/lock-group.h"

namespace mai {
    
namespace core {
    
inline namespace v1 {
    
////////////////////////////////////////////////////////////////////////////////
/// struct LRUHandle
////////////////////////////////////////////////////////////////////////////////
    
void LRUHandle::set_flags(bool val, uint32_t bits) {
    if (val) {
        flags.fetch_or(bits);
    } else {
        flags.fetch_and(~bits);
    }
}

/*static*/ LRUHandle *LRUHandle::New(std::string_view key, size_t value_size,
                                     uint64_t id) {
    size_t total_size = sizeof(LRUHandle) + key.size() + value_size;
    total_size = RoundUp(total_size, sizeof(max_align_t));

    LRUHandle *h = static_cast<LRUHandle *>(::malloc(total_size));
    if (!h) {
        return h;
    }
    ::memset(h, 0, total_size);
    ::memcpy(h->data, key.data(), key.size());
    h->id       = id;
    h->key_size = key.size();
    h->value    = h->data + h->key_size; // RoundUp(key.size(), sizeof(max_align_t));
    return h;
}

/*static*/ void LRUHandle::Free(LRUHandle *handle) {
    ::free(handle);
}
    
////////////////////////////////////////////////////////////////////////////////
/// class LRUCache
////////////////////////////////////////////////////////////////////////////////
    
struct LRUCacheShard::LookupHandle final {
    LookupHandle(std::string_view key, const Comparator *cmp) {
        handle = static_cast<LRUHandle *>(memory.Allocate(sizeof(LRUHandle) + key.size()));
        handle->key_size = key.size();
        handle->hash_val = cmp->Hash(key);
        ::memcpy(handle->data, key.data(), key.size());
    }
    
    LRUHandle *handle;
    base::ScopedMemory memory;
}; // struct LRUCache::LookupHandle
    
LRUCacheShard::LRUCacheShard(Allocator *low_level_allocator, size_t capacity,
                             base::LockGroup *locks)
    : cmp_(Comparator::Bytewise())
    , boundle_(new TableBoundle(kMinLRUTableSlots, KeyComparator{cmp_}))
    , capacity_(capacity)
    , locks_(!locks ? new base::LockGroup(16) : locks)
    , locks_ownership_(locks == nullptr) {
#if defined(DEBUG) || defined(_DEBUG)
    ::memset(&lru_dummy_, 0, sizeof(lru_dummy_));
    ::memset(&in_use_dummy_, 0, sizeof(in_use_dummy_));
#endif // defined(DEBUG) || defined(_DEBUG)
    lru_->next = lru_;
    lru_->prev = lru_;
    in_use_->next = in_use_;
    in_use_->prev = in_use_;
}
    
LRUCacheShard::~LRUCacheShard() {
    LRUTable::Iterator iter(&boundle_->table);
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        LRUHandle *x = iter.key();
        if (x->is_deletion()) {
            LRUHandle::Free(x);
        }
    }
    
    while (in_use_->next != in_use_) {
        LRUHandle *x = in_use_->next;
        DCHECK_EQ(1, x->ref_count());
        
        if (!x->is_deletion() && x->deleter) {
            x->deleter(x->key(), x->value);
        }
        LRU_Remove(x);
        LRUHandle::Free(x);
    }

    while (lru_->next != lru_) {
        LRUHandle *x = lru_->next;
        DCHECK_EQ(1, x->ref_count());

        if (!x->is_deletion() && x->deleter) {
            x->deleter(x->key(), x->value);
        }
        LRU_Remove(x);
        LRUHandle::Free(x);
    }
    
    if (locks_ownership_) {
        delete locks_;
    }
}
    
Error LRUCacheShard::GetOrLoad(std::string_view key,
                          base::intrusive_ptr<LRUHandle> *result,
                          LRUHandle::Deleter *deleter, LRUHandle::Loader *loader,
                          void *arg0, void *arg1) {
    auto stripe = DCHECK_NOTNULL(locks_->GetByKey(key));

    LRUHandle *handle = nullptr;
    int retry_factor = 1;
    while (true) {
        handle = Get(key);
        if (handle) {
            result->reset(handle);
            return Error::OK();
        }
        if (stripe->TryLock()) {
            Error rs = loader(key, &handle, arg0, arg1);
            if (!rs) {
                return rs;
            }
            break;
        }
        for (int i = 0; i < retry_factor; ++i) {
            std::this_thread::yield();
        }
        retry_factor = (retry_factor << 1) % (1024 * 1024);
    }
    
    handle->AddRef();
    Insert(key, handle, deleter);
    PurgeIfNeeded(false);
    result->reset(handle);

    stripe->Unlock();
    return Error::OK();
#if 0
    LRUHandle *handle = Get(key);
    if (handle) {
        result->reset(handle);
        return Error::OK();
    }
    Error rs = loader(key, &handle, arg0, arg1);
    if (!rs) {
        return rs;
    }

    handle->AddRef();
    Insert(key, handle, deleter);
    PurgeIfNeeded(false);
    result->reset(handle);
    return Error::OK();
#endif
}
    
void LRUCacheShard::Insert(std::string_view key, LRUHandle *handle,
                      LRUHandle::Deleter *deleter) {
    handle->hash_val = cmp_->Hash(key);
    handle->deleter  = deleter;
    handle->hits.store(10, std::memory_order_relaxed);
    handle->refs.store(1, std::memory_order_relaxed);
    handle->flags.store(0, std::memory_order_relaxed);
    
    base::intrusive_ptr<TableBoundle> boundle(boundle_.get());
    
    mutex_.lock();
    boundle->table.Put(handle);
    LRU_Insert(lru_, handle);
    ++size_;
    mutex_.unlock();
    
    PurgeIfNeeded(false);
}

LRUHandle *LRUCacheShard::Get(std::string_view key) {
    LookupHandle lookup(key, cmp_);
    
    base::intrusive_ptr<TableBoundle> boundle(boundle_.get());
    
    LRUTable::Iterator iter(&boundle->table);
    iter.Seek2(lookup.handle);
    if (!iter.Valid() || iter.key()->is_deletion()) {
        return nullptr;
    }
    
    bool most_hit = (iter.key()->hits.fetch_add(1) >= 17);
    if (most_hit) {
        iter.key()->hits.store(1);
        
        std::unique_lock<std::mutex> lock(mutex_);
        if (iter.key()->is_deletion()) { // Try test deletion again.
            return nullptr;
        }
        LRU_Remove(iter.key());
        int refs = iter.key()->ref_count();
        if (refs == 1) {
            LRU_Insert(lru_, iter.key());
        } else if (refs > 1) {
            LRU_Insert(in_use_, iter.key());
        } else {
            DCHECK_GE(refs, 1);
        }
    }
    return iter.key();
}
    
void LRUCacheShard::Remove(std::string_view key) {
    LRUHandle *x = Get(key);
    if (x && x->ref_count() == 1) {
        x->set_deletion(true);
        if (x->deleter) {
            x->deleter(x->key(), x->value);
        }
        LRU_Remove(x);
    }
}
    
void LRUCacheShard::PurgeIfNeeded(bool force) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (size_ <= capacity_ && !force) {
        return;
    }
    
    LRUHandle *inv = lru_->next;
    int refs = inv->ref_count();
    if (refs == 1) {
        inv->set_deletion(true);
        if (inv->deleter) {
            inv->deleter(inv->key(), inv->value);
        }
        LRU_Remove(inv);
        --size_;
    } else if (refs >= 2) {
        LRU_Remove(inv);
        LRU_Insert(in_use_, inv);
    }
    
    inv = in_use_->next;
    while (inv != in_use_) {
        int refs = inv->ref_count();
        if (refs == 1) {
            LRUHandle *next = inv->next;
            inv->set_deletion(true);
            if (inv->deleter) {
                inv->deleter(inv->key(), inv->value);
            }
            LRU_Remove(inv);
            --size_;
            inv = next;
        } else {
            inv = inv->next;
        }
    }

    base::intrusive_ptr<TableBoundle> boundle(boundle_.get());

    float conflict_factor = boundle->conflict_factor();
    if (conflict_factor > 3.0 || boundle->arena.memory_usage() > base::kMB) {
        int new_slots = boundle->table.n_slots() * conflict_factor + 7;
        if (new_slots < kMinLRUTableSlots) {
            new_slots = kMinLRUTableSlots;
        }
        
        TableBoundle *new_boundle = new TableBoundle(new_slots, KeyComparator{cmp_});

        LRUTable::Iterator iter(&boundle->table);
        for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
            if (iter.key()->is_deletion()) {
                LRUHandle::Free(iter.key());
            } else {
                new_boundle->table.Put(iter.key());
            }
        }
        boundle_.reset(new_boundle);
    }
}
    
////////////////////////////////////////////////////////////////////////////////
/// class LRUCache
////////////////////////////////////////////////////////////////////////////////
    
LRUCache::LRUCache(size_t max_shards, Allocator *ll_allocator, size_t capacity)
    : max_shards_(max_shards)
    , ll_allocator_(DCHECK_NOTNULL(ll_allocator))
    , capacity_(capacity)
    , shards_(new std::atomic<uintptr_t>[max_shards])
    , locks_(new base::LockGroup(max_shards * 8, false)) {
    for (int i = 0; i < max_shards_; ++i) {
        shards_[i].store(0, std::memory_order_relaxed);
    }
}

LRUCache::~LRUCache() {
    if (shards_) {
        for (int i = 0; i < max_shards_; ++i) {
            auto val = shards_[i].load();
            DCHECK_NE(kPendingMask, val);
            delete reinterpret_cast<const LRUCacheShard *>(val);
        }
        delete[] shards_;
    }
}
    
LRUCacheShard *LRUCache::GetShard(size_t idx) {
    DCHECK_GE(idx, 0);
    DCHECK_LT(idx, max_shards_);
    
    auto shard = shards_ + idx;
    if (!(shard->load(std::memory_order_acquire) & kCreatedMask) &&
        NeedInit(shard)) {
        Install(shard);
    }
    return Get(shard);
}

const LRUCacheShard *LRUCache::GetShard(size_t idx) const {
    DCHECK_GE(idx, 0);
    DCHECK_LT(idx, max_shards_);
    
    uintptr_t val = 0;
    while ((val = shards_[idx].load(std::memory_order_acquire))
           == kPendingMask) {
        std::this_thread::yield();
    }
    return reinterpret_cast<const LRUCacheShard *>(val);
}
    
bool LRUCache::Delete(size_t idx) {
    DCHECK_GE(idx, 0);
    DCHECK_LT(idx, max_shards_);
    
    auto shard = shards_ + idx;
    
    uintptr_t val = 0;
    while ((val = shard->load(std::memory_order_acquire))
           == kPendingMask) {
        std::this_thread::yield();
    }
    auto exp = val;
    if (shard->compare_exchange_strong(exp, 0)) {
        auto inst = reinterpret_cast<LRUCacheShard *>(val);
        delete inst;
        return true;
    }
    return false;
}
    
void LRUCache::Purge(size_t idx) {
    uintptr_t val = 0;
    while ((val = shards_[idx].load(std::memory_order_acquire))
           == kPendingMask) {
        std::this_thread::yield();
    }
    auto inst = reinterpret_cast<LRUCacheShard *>(val);
    if (inst) {
        inst->PurgeIfNeeded(true);
    }
}
    
size_t LRUCache::HashKey(std::string_view key) const {
    return static_cast<size_t>(base::Hash::Sdbm(key.data(), key.size())
                               % max_shards_);
}
    
} // inline namespace v1
    
} // namespace core
    
} // namespace mai
