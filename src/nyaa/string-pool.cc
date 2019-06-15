#include "nyaa/string-pool.h"
#include "nyaa/visitors.h"
#include "base/arenas.h"
#include "mai/env.h"

namespace mai {
    
namespace nyaa {
    
StringPool::StringPool(Allocator *lla, RandomGenerator *random)
    : lla_(DCHECK_NOTNULL(lla))
    , random_(random)
    , arena_(new base::StandaloneArena(lla_))
    , n_slots_(kInitSlots)
    , slots_(arena_->NewArray<Node>(n_slots_))
    , seed_(!random ? 0 : random_->NextU32()) {
    ::memset(slots_, 0, sizeof(Node) * n_slots_);
}

StringPool::~StringPool() { delete arena_; }
    
size_t StringPool::Iterate(ObjectVisitor *visitor) {
    size_t n = 0;
    for (size_t i = 0; i < n_slots_; ++i) {
        Node *prev = slots_ + i;
        Node *p = prev->next;
        while (p) {
            visitor->VisitPointer(this, reinterpret_cast<Object **>(&p->key));
            if (!p->key) { // p->key == nullptr means it should be sweep.
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
    
void StringPool::Rehash(size_t n_slots) {
    base::Arena *bak_arena = arena_;
    Node *bak_slots = slots_;
    size_t bak_n_slots = n_slots_;
    
    arena_   = new base::StandaloneArena(lla_);
    slots_   = arena_->NewArray<Node>(n_slots);
    n_slots_ = n_slots;
    ::memset(slots_, 0, sizeof(Node) * n_slots_);
    size_    = 0;
    pool_    = nullptr;
    seed_    = !random_ ? 0 : random_->NextU32();

    for (size_t i = 0; i < bak_n_slots; ++i) {
        Node *slot = bak_slots + i;
        for (Node *p = slot->next; p; p = p->next) {
            Add(p->key);
        }
    }
    delete bak_arena;
}
    
} // namespace nyaa

} // namespace mai
