#ifndef MAI_NYAA_STRING_POOL_H_
#define MAI_NYAA_STRING_POOL_H_

#include "nyaa/nyaa-values.h"
#include "base/arena.h"
#include "base/hash.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {

namespace nyaa {
    
// Weak table for string
class StringPool final {
public:
    static constexpr const size_t kInitSlots = 127;
    static constexpr const float kExtendConflictFactor = 1.75;
    static constexpr const float kShrinkConflictFactor = 0.25;
    
    StringPool(Allocator *lla);
    ~StringPool();
    
    DEF_VAL_GETTER(size_t, n_slots);
    DEF_VAL_GETTER(size_t, size);
    
    float GetConflictFactor() const {
        return static_cast<float>(size_) / static_cast<float>(n_slots_);
    }
    
    size_t ApproximateMemoryUsage() const { return arena_->memory_usage(); }
    
    size_t GetNodePoolSize() const {
        size_t n = 0;
        for (Node *p = pool_; p; p = p->next) {
            ++n;
        }
        return n;
    }
    
    NyString *GetOrNull(const char *z) const { return GetOrNull(z, !z ? 0 : ::strlen(z)); }
    
    NyString *GetOrNull(const char *z, size_t n) const {
        uint32_t hash_val = base::Hash::Js(z, n);
        Node *slot = slots_ + (hash_val % n_slots_);
        for (Node *p = slot->next; p; p = p->next) {
            if (p->key->Compare(z, n) == 0) {
                return p->key;
            }
        }
        return nullptr;
    }
    
    void Add(NyString *s) {
        DCHECK(GetOrNull(s->bytes(), s->size()) == nullptr);
        Node *slot = slots_ + (s->hash_val() % n_slots_);
        Node *p = AllocNode();
        p->next = slot->next;
        p->key  = s;
        slot->next = p;
        size_++;
        RehashIfNeeded(true);
    }
    
    template<class Cond> inline size_t Sweep(Cond cond) {
        size_t n = 0;
        for (size_t i = 0; i < n_slots_; ++i) {
            Node *prev = slots_ + i;
            Node *p = prev->next;
            while (p) {
                if (cond(p->key)) {
                    prev->next = p->next;
                    FreeNode(p);
                    p = prev->next;
                    ++n;
                } else {
                    prev = p;
                    p = p->next;
                }
            }
        }
        size_ -= n;
        RehashIfNeeded(false);
        return n;
    }

private:
    struct Node {
        Node *next = nullptr;
        NyString *key = nullptr;
    };
    
    void FreeNode(Node *x) {
        x->next = pool_;
        pool_ = x;
    }
    
    Node *AllocNode() {
        if (pool_) {
            Node *x = pool_;
            pool_ = x->next;
            return x;
        }
        return arena_->New<Node>();
    }
    
    void RehashIfNeeded(bool extend) {
        float factor = GetConflictFactor();
        
        if (extend) {
            if (factor > kExtendConflictFactor) {
                Rehash((n_slots_ << 1) + 1);
            }
        } else {
            if (n_slots_ <= kInitSlots) {
                return;
            }
            if (factor < kShrinkConflictFactor) {
                Rehash((n_slots_ >> 1) + 1);
            }
        }
    }
    
    void Rehash(size_t n_slots);
    
    Allocator *const lla_;
    base::Arena *arena_;
    size_t n_slots_;
    Node *slots_;
    size_t size_ = 0;
    Node *pool_ = nullptr; // pool of free nodes.
    uint32_t seed_ = 0;
}; // class StringPool
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_STRING_POOL_H_
