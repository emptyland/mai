#ifndef MAI_CORE_HASH_MAP_V2_H_
#define MAI_CORE_HASH_MAP_V2_H_

#include "base/arena.h"
#include "base/base.h"
#include <atomic>

namespace mai {
    
namespace core {
    
inline namespace v2 {
    
template<class Key, class Comparator>
class HashMap final {
public:
    class Iterator;
    
    HashMap(size_t initial_slots, Comparator cmp, base::Arena *arena)
        : cmp_(cmp)
        , arena_(DCHECK_NOTNULL(arena))
        , n_slots_(initial_slots)
        , n_entries_(0) {
        buckets_ =
            static_cast<Node *>(arena_->Allocate(sizeof(Node) * n_slots_, 4));
        ::memset(buckets_, 0, sizeof(Node) * n_slots_);
    }
    
    ~HashMap() {}
    
    DEF_VAL_GETTER(size_t, n_slots);
    size_t n_entries() const { return n_entries_.load(); }
    
    void Put(Key key) {
        uint32_t hash = cmp_(key);
        Node *slot = buckets_ + hash % n_slots_;
        Node *node = NewNode(key);

        Node *pp = slot;
        Node *p = slot->next();
        while (p) {
            if (cmp_(p->key_, key) >= 0) {
                break;
            }
            pp = p;
            p = p->next();
        }
        node->nobarrier_set_next(p);
        
        Node *old_val = pp->nobarrier_next();
        Node *e = nullptr;
        do {
            e = old_val;
        } while (pp->next_.compare_exchange_strong(e, node));
        //pp->set_next(node);
        n_entries_.fetch_add(1);
    }

private:
    struct Node;
    
    std::tuple<size_t, Node *> FindGreaterOrEqual(Key key) const {
        uint32_t hash = cmp_(key);
        size_t index = hash % n_slots_;
        Node *slot = buckets_ + index;

        Node *p = slot->next();
        while (p) {
            if (cmp_(p->key_, key) >= 0) {
                return std::make_tuple(index, p);
            }
            p = p->next();
        }
        return std::make_tuple(n_slots_, nullptr);
    }
    
    std::tuple<size_t, Node *> FindEqual(Key key) const {
        uint32_t hash = cmp_(key);
        size_t index = hash % n_slots_;
        Node *slot = buckets_ + index;
        
        Node *p = slot->next();
        while (p) {
            if (cmp_(p->key_, key) == 0) {
                return std::make_tuple(index, p);
            }
            p = p->next();
        }
        return std::make_tuple(n_slots_, nullptr);
    }
    
    Node *NewNode(Key key) {
        void *chunk = arena_->Allocate(sizeof(Node), 4);
        return new (chunk) Node{key};
    }
    
    Comparator const cmp_;
    base::Arena *const arena_;
    const size_t n_slots_;
    
    std::atomic<size_t> n_entries_;
    Node *buckets_;
}; // class HashMap
    
template<class Key, class Comparator>
struct HashMap<Key, Comparator>::Node {
    
    Node() {}
    
    Node(Key key) : next_(nullptr), key_(key) {}
    
    Node *next() {
        return next_.load(std::memory_order_acquire);
    }
    
    void set_next(Node *x) {
        next_.store(x, std::memory_order_release);
    }
    
    Node *nobarrier_next() {
        return next_.load(std::memory_order_relaxed);
    }
    
    void nobarrier_set_next(Node *x) {
        next_.store(x, std::memory_order_relaxed);
    }
    
    std::atomic<Node *> next_;
    Key key_;
}; // template<class Key, class Comparator> struct HashMap<Key, Comparator>::Node
    
template<class Key, class Comparator>
class HashMap<Key, Comparator>::Iterator {
public:
    Iterator(const HashMap<Key, Comparator> *owns)
        : owns_(owns)
        , slot_(owns->n_slots_) {}
    
    bool Valid() const {
        return slot_ < owns_->n_slots_ && node_ != nullptr;
    }
    
    Key key() const { return DCHECK_NOTNULL(node_)->key_; }
    
    void Next() {
        node_ = node_->next();
        if (!node_) {
            while (++slot_ < owns_->n_slots_) {
                if (owns_->buckets_[slot_].next()) {
                    node_ = owns_->buckets_[slot_].next();
                    break;
                }
            }
        }
    }
    
    void Seek(Key key) {
        std::tie(slot_, node_) = owns_->FindGreaterOrEqual(key);
    }
    
    void Seek2(Key key) {
        std::tie(slot_, node_) = owns_->FindEqual(key);
    }
    
    void SeekToFirst() {
        for (size_t i = 0; i < owns_->n_slots_; ++i) {
            if (owns_->buckets_[i].next()) {
                node_ = owns_->buckets_[i].next();
                slot_ = i;
                break;
            }
        }
    }
    
private:
    const HashMap<Key, Comparator> *owns_;
    size_t slot_;
    Node *node_ = nullptr;
}; // template<class Key, class Comparator> class HashMap<Key, Comparator>::Iterator
    
} // inline namespace v2
    
} // namespace core
    
} // namespace mai


#endif // MAI_CORE_HASH_MAP_V2_H_
