#ifndef MAI_CORE_HASH_MAP_H_
#define MAI_CORE_HASH_MAP_H_

#include "base/spin-locking.h"
#include "base/base.h"
#include "glog/logging.h"
#include <memory>
#include <tuple>

namespace mai {
    
namespace core {
    
template<class Key, class Comparator>
class HashMap final {
public:
    class Iterator;
    
    struct Node {
        Node *next;
        Key   key;
    };
    
    HashMap(size_t initial_slots, Comparator comparator)
        : comparator_(comparator)
        , n_slots_(initial_slots)
        , n_items_(0)
        , slots_(new Slot[initial_slots]) {
    }
    
    ~HashMap() {
        for (size_t i = 0; i < n_slots_; ++i) {
            if (slots_[i].head) {
                Node *p = slots_[i].head;
                slots_[i].head = p->next;
                delete p;
            }
        }
    }

    void Put(Key key) {
        int hash = comparator_(key);
        Slot *slot = (slots_.get() + hash % n_slots_);
        Node *node = new Node;
        node->key = key;
        base::WriterSpinLock socpe_lock(&slot->mutex);

        Node *pp = reinterpret_cast<Node *>(slot);
        Node *p = slot->head;
        while (p) {
            if (comparator_(p->key, key) >= 0) {
                break;
            }
            pp = p;
            p = p->next;
        }
        pp->next = node;
        node->next = p;
        n_items_.fetch_add(1);
    }
    
    size_t items_count() const { return n_items_.load(); }
    DEF_VAL_GETTER(size_t, n_slots);
private:
    struct Slot {
        Slot() : mutex(base::RWSpinLocking::kLockBais) {}
        
        Node *head = nullptr;
        base::SpinMutex mutex;
    };
    
    std::tuple<size_t, Node *> FindGreaterOrEqual(Key key) const {
        int hash = comparator_(key);
        int index = hash % n_slots_;
        Slot *slot = (slots_.get() + index);
        
        base::ReaderSpinLock socpe_lock(&slot->mutex);
        Node *p = slot->head;
        while (p) {
            if (comparator_(p->key, key) >= 0) {
                return std::make_tuple(index, p);
            }
            p = p->next;
        }
        return std::make_tuple(n_slots_, nullptr);
    }
    
    Comparator const comparator_;
    size_t n_slots_;
    std::atomic<size_t> n_items_;
    std::unique_ptr<Slot[]> slots_; // Slots is fixed!
}; // class HashMap
    
    
template<class Key, class Comparator>
class HashMap<Key, Comparator>::Iterator {
public:
    Iterator(const HashMap<Key, Comparator> *map)
        : map_(map)
        , slot_(map->n_slots_) {}
    
    bool Valid() const {
        return slot_ < map_->n_slots_ && node_ != nullptr;
    }
    
    Key key() const { return DCHECK_NOTNULL(node_)->key; }
    
    void Next() {
        base::ReaderSpinLock socpe_lock(&map_->slots_[slot_].mutex);
        node_ = node_->next;
        if (!node_) {
            while (++slot_ < map_->n_slots_) {
                base::ReaderSpinLock socpe_lock(&map_->slots_[slot_].mutex);
                if (map_->slots_[slot_].head) {
                    node_ = map_->slots_[slot_].head;
                    break;
                }
            }
        }
    }
    
    void Seek(Key key) {
        std::tie(slot_, node_) = map_->FindGreaterOrEqual(key);
    }
    
    void SeekToFirst() {
        for (size_t i = 0; i < map_->n_slots_; ++i) {
            base::ReaderSpinLock socpe_lock(&map_->slots_[i].mutex);
            if (map_->slots_[i].head) {
                node_ = map_->slots_[i].head;
                slot_ = i;
                break;
            }
        }
    }
    
private:
    const HashMap<Key, Comparator> *map_;
    size_t slot_;
    Node *node_ = nullptr;
}; // class HashMap<Key, Comparator>::Iterator
    
} // namespace core
    
} // namespace mai

#endif // MAI_CORE_HASH_MAP_H_
