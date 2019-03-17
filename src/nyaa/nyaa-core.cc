#include "nyaa/nyaa-core.h"
#include "nyaa/nyaa-values.h"
#include "mai-lang/nyaa.h"
#include "mai/allocator.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {
    
NyaaCore::NyaaCore(Nyaa *stub)
    : stub_(DCHECK_NOTNULL(stub))
    , page_alloc_(stub->isolate()->env()->GetLowLevelAllocator()) {
    top_slot_ = new HandleScopeSlot;
    top_slot_->scope = nullptr;
    top_slot_->prev  = nullptr;
    top_slot_->base  = nullptr;
    top_slot_->end   = top_slot_->base;
    top_slot_->limit = top_slot_->end;
}

NyaaCore::~NyaaCore() {
    
}
    
/*static*/ NyaaCore *NyaaCore::Current() {
    return Isolate::Current()->GetNyaa()->core();
}

void NyaaCore::EnterHandleScope(HandleScope *handle_scope) {
    auto prev_slot = top_slot_;
    auto slot = new HandleScopeSlot{DCHECK_NOTNULL(handle_scope), prev_slot};
    slot->base  = prev_slot->end;
    slot->end   = slot->base;
    slot->limit = prev_slot->limit;
    top_slot_ = slot;
}

void NyaaCore::ExitHandleScope() {
    auto slot = top_slot_;
    top_slot_ = slot->prev;
    if (slot->prev->limit != slot->limit &&
        (slot->limit - slot->base) % page_alloc_->granularity() == 0) {
        page_alloc_->Free(slot->base, slot->limit - slot->base);
    }
    delete slot;
}

Address NyaaCore::AdvanceHandleSlots(int n_slots) {
    auto slot = top_slot_;
    DCHECK_GE(n_slots, 0);
    if (!n_slots) {
        return DCHECK_NOTNULL(slot)->end;
    }
    
    auto size = n_slots * sizeof(NyObject **);
    if (slot->end + size >= slot->limit) {
        auto backup = slot->base;
        auto growed_size = RoundUp((slot->limit - slot->base), page_alloc_->granularity())
                + page_alloc_->granularity();
        slot->base = static_cast<Address>(page_alloc_->Allocate(growed_size,
                                                                page_alloc_->granularity()));
        slot->limit = slot->base + growed_size;
        ::memcpy(slot->base, backup, slot->end - backup);
        slot->end = slot->base + (slot->end - backup);
    }
    
    DCHECK_LT(slot->end, slot->limit);
    auto slot_addr = slot->end;
    slot->end += size;
    return slot_addr;
}
    
} // namespace nyaa
    
} // namespace mai
