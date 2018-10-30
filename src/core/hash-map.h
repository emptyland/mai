#ifndef MAI_CORE_HASH_MAP_H_
#define MAI_CORE_HASH_MAP_H_

#include "base/spin-locking.h"
#include "base/base.h"
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

    void Put(Key key) {
        int hash = comparator_.Hash(key);
        Slot *slot = (slots_.get() + hash % n_slots_);
        Node *node = new Node;
        node->key = key;
        base::WriterSpinLock socpe_lock(&slot->mutex);

        Node *pp = reinterpret_cast<Node *>(slot);
        Node *p = slot->head_;
        while (p) {
            if (comparator_.Equals(key, p->key)) {
                break;
            }
            pp = p;
            p = p->next;
        }
        pp->next = node;
        n_items_.fetch_add(1);
    }
    
    std::tuple<int, Node *> Seek(Key key) const {
        int hash = comparator_.Hash(key);
        int index = hash % n_slots_;
        Slot *slot = (slots_.get() + index);
     
        base::ReaderSpinLock socpe_lock(&slot->mutex);
        Node *p = slot->head_;
        while (p) {
            if (comparator_.Equals(key, p->key)) {
                return std::make_tuple(index, p);
            }
            p = p->next;
        }
        return std::make_tuple(-1, nullptr);
    }
    
private:
    Comparator const comparator_;
    
    struct Slot {
        Slot() : mutex(base::RWSpinLocking::kLockBais) {}
        
        Node *head_ = nullptr;
        base::SpinMutex mutex;
    };
    
    size_t n_slots_;
    std::atomic<size_t> n_items_;
    std::unique_ptr<Slot[]> slots_;
}; // class HashMap
    
} // namespace core
    
} // namespace mai

#endif // MAI_CORE_HASH_MAP_H_
