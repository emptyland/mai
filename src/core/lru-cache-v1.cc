#include "core/lru-cache-v1.h"

namespace mai {
    
namespace core {
    
inline namespace v1 {
    
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
    LRUHandle *h = static_cast<LRUHandle *>(::malloc(total_size));
    if (!h) {
        return h;
    }
    ::memset(h, 0, total_size);
    ::memcpy(h->data, key.data(), key.size());
    h->id       = id;
    h->key_size = key.size();
    h->value    = h->data + key.size();
    return h;
}

/*static*/ void LRUHandle::Free(LRUHandle *handle) {
    ::free(handle);
}
    
LRUCache::LRUCache(Allocator *low_level_allocator, size_t capacity)
    : cmp_(Comparator::Bytewise())
    , boundle_(new TableBoundle(low_level_allocator, kMinLRUTableSlots,
                                KeyComparator{cmp_}))
    , capacity_(capacity)
    , in_ordered_(17) {
#if defined(DEBUG) || defined(_DEBUG)
    ::memset(&lru_dummy_, 0, sizeof(lru_dummy_));
    ::memset(&in_use_dummy_, 0, sizeof(in_use_dummy_));
#endif // defined(DEBUG) || defined(_DEBUG)
    lru_->next = lru_;
    lru_->prev = lru_;
    in_use_->next = in_use_;
    in_use_->prev = in_use_;
}
    
LRUCache::~LRUCache() {
    while (in_use_->next != in_use_) {
        LRUHandle *x = in_use_->next;
        DCHECK_EQ(1, x->refs.load());
        
        if (x->deleter) { x->deleter(x->key(), x->value); }
        LRU_Remove(x);
        LRUHandle::Free(x);
    }

    while (lru_->next != lru_) {
        LRUHandle *x = lru_->next;
        DCHECK_EQ(1, x->refs.load());

        if (x->deleter) { x->deleter(x->key(), x->value); }
        LRU_Remove(x);
        LRUHandle::Free(x);
    }
}
    
Error LRUCache::GetOrLoad(std::string_view key, base::intrusive_ptr<LRUHandle> *result,
                    LRUHandle::Deleter *deleter, LRUHandle::Loader *loader,
                    void *arg0, void *arg1) {
    LRUHandle *handle = Get(key);
    if (handle) {
        result->reset(handle);
        return Error::OK();
    }
    Error rs = loader(key, &handle, arg0, arg1);
    if (!rs) {
        return rs;
    }
    Insert(key, handle, deleter);
    PurgeIfNeeded(false);
    result->reset(handle);
    return Error::OK();
}
    
void LRUCache::Insert(std::string_view key, LRUHandle *handle,
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

LRUHandle *LRUCache::Get(std::string_view key) {
    std::unique_ptr<char[]> buf(new char[sizeof(LRUHandle) + key.size()]);
    LRUHandle *lookup_key = reinterpret_cast<LRUHandle *>(buf.get());
    memcpy(lookup_key->data, key.data(), key.size());
    lookup_key->key_size = key.size();
    lookup_key->hash_val = cmp_->Hash(key);
    
    base::intrusive_ptr<TableBoundle> boundle(boundle_.get());
    
    LRUTable::Iterator iter(&boundle->table);
    iter.Seek2(lookup_key);
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
    
void LRUCache::PurgeIfNeeded(bool force) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (size_ <= capacity_ && !force) {
        return;
    }
    
    LRUHandle *inv = lru_->next;
    int refs = inv->ref_count();
    if (refs == 1) {
        LRU_Remove(inv);
        inv->set_deletion(true);
        if (inv->deleter) { inv->deleter(inv->key(), inv->value); }
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
            LRU_Remove(inv);
            inv->set_deletion(true);
            if (inv->deleter) { inv->deleter(inv->key(), inv->value); }
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
        
        TableBoundle *new_boundle =
            new TableBoundle(boundle->arena.low_level_allocator(), new_slots,
                             KeyComparator{cmp_});

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
    
} // inline namespace v1
    
} // namespace core
    
} // namespace mai
