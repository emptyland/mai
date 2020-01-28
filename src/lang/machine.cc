#include "lang/machine.h"
#include "lang/value-inl.h"
#include "lang/heap.h"
#include "mai/allocator.h"
#include <memory>

namespace mai {

namespace lang {

Machine::Machine(int id, Scheduler *owner)
    : id_(id)
    , owner_(owner) {
    top_slot_ = new HandleScopeSlot;
    top_slot_->scope = nullptr;
    top_slot_->prev  = nullptr;
    top_slot_->base  = nullptr;
    top_slot_->end   = top_slot_->base;
    top_slot_->limit = top_slot_->end;
}

Machine::~Machine() {
    // TODO:
}

void Machine::ExitHandleScope() {
    auto lla = __isolate->env_->GetLowLevelAllocator();
    auto slot = top_slot_;
    DCHECK(slot->prev || slot->prev->end == slot->base);
    top_slot_ = slot->prev;
    if (slot->prev->limit != slot->limit &&
        (slot->limit - slot->base) % lla->granularity() == 0) {
        lla->Free(slot->base, slot->limit - slot->base);
    }
    delete slot;
}

Address Machine::AdvanceHandleSlots(int n_slots) {
    auto lla = __isolate->env_->GetLowLevelAllocator();
    auto slot = top_slot_;
    DCHECK_GE(n_slots, 0);
    if (!n_slots) {
        return DCHECK_NOTNULL(slot)->end;
    }
    
    auto size = n_slots * sizeof(Any **);
    if (slot->end + size >= slot->limit) {
        auto backup = slot->base;
        auto growed_size = RoundUp((slot->limit - slot->base), lla->granularity())
            + lla->granularity();
        slot->base = static_cast<Address>(lla->Allocate(growed_size, lla->granularity()));
        slot->limit = slot->base + growed_size;
        ::memcpy(slot->base, backup, slot->end - backup);
        slot->end = slot->base + (slot->end - backup);
    }
    
    DCHECK_LT(slot->end, slot->limit);
    auto slot_addr = slot->end;
    slot->end += size;
    return slot_addr;
}

// Factory for create a UTF-8 string
String *Machine::NewUtf8String(const char *utf8_string, size_t n, uint32_t flags) {
    size_t request_size = String::RequiredSize(n + 1);
    AllocationResult result = __isolate->heap()->Allocate(request_size, flags);
    if (!result.ok()) {
        return nullptr;
    }
    String *obj = new (result.ptr()) String(__isolate->builtin_type(kType_string),
                                            static_cast<uint32_t>(n + 1),
                                            utf8_string,
                                            static_cast<uint32_t>(n));
    return obj;
}

} // namespace lang

} // namespace mai
