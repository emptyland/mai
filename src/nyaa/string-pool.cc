#include "nyaa/string-pool.h"
#include "base/arenas.h"

namespace mai {
    
namespace nyaa {
    
StringPool::StringPool(Allocator *lla)
    : lla_(DCHECK_NOTNULL(lla))
    , arena_(new base::StandaloneArena(lla_))
    , n_slots_(kInitSlots)
    , slots_(arena_->NewArray<Node>(n_slots_)) {
    ::memset(slots_, 0, sizeof(Node) * n_slots_);
}

StringPool::~StringPool() { delete arena_; }
    
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
