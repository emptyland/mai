#include "lang/machine.h"
#include "lang/coroutine.h"
#include "lang/value-inl.h"
#include "lang/heap.h"
#include "mai/allocator.h"
#include <memory>

namespace mai {

namespace lang {

Machine::Machine(int id, Scheduler *owner)
    : id_(id)
    , owner_(owner)
    , free_dummy_(Coroutine::NewDummy())
    , runnable_dummy_(Coroutine::NewDummy()) {
    top_slot_ = new HandleScopeSlot;
    top_slot_->scope = nullptr;
    top_slot_->prev  = nullptr;
    top_slot_->base  = nullptr;
    top_slot_->end   = top_slot_->base;
    top_slot_->limit = top_slot_->end;
}

Machine::~Machine() {
    // TODO:
    while (!QUEUE_EMPTY(runnable_dummy_)) {
        auto x = runnable_dummy_->next();
        QUEUE_REMOVE(x);
        delete x;
    }
    Coroutine::DeleteDummy(runnable_dummy_);
    
    while (!QUEUE_EMPTY(free_dummy_)) {
        auto x = free_dummy_->next();
        QUEUE_REMOVE(x);
        delete x;
    }
    Coroutine::DeleteDummy(free_dummy_);
}

void Machine::ExitHandleScope() {
    auto lla = STATE->env_->GetLowLevelAllocator();
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
    auto lla = STATE->env_->GetLowLevelAllocator();
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
    AllocationResult result = STATE->heap()->Allocate(request_size, flags);
    if (!result.ok()) {
        return nullptr;
    }
    String *obj = new (result.ptr()) String(STATE->builtin_type(kType_string),
                                            static_cast<uint32_t>(n + 1),
                                            utf8_string,
                                            static_cast<uint32_t>(n));
    return obj;
}

AbstractValue *Machine::NewNumber(BuiltinType primitive_type, const void *value, size_t n,
                                  uint32_t flags) {
    AllocationResult result = STATE->heap()->Allocate(sizeof(AbstractValue), flags);
    if (!result.ok()) {
        return nullptr;
    }
    const Class *clazz = STATE->metadata_space()->GetNumberClassOrNull(primitive_type);
    DCHECK(clazz != nullptr) << "Bad type: " << primitive_type;
    return new (result.ptr()) AbstractValue(DCHECK_NOTNULL(clazz), value, n);
}

AbstractArray *Machine::NewArraySlow(BuiltinType type, size_t length, uint32_t flags) {
    const Class *clazz = DCHECK_NOTNULL(STATE->builtin_type(type));
    if (::strstr(clazz->name(), "array") != clazz->name()){
        NOREACHED() << "class: " << clazz->name() << " is not array!";
        return nullptr;
    }

    const Field *elems_field =
        DCHECK_NOTNULL(STATE->metadata_space()->FindClassFieldOrNull(clazz, "elems"));

    size_t request_size = sizeof(AbstractArray) + elems_field->type()->ref_size() * length;
    AllocationResult result = STATE->heap()->Allocate(request_size, flags);
    if (!result.ok()) {
        return nullptr;
    }
    AbstractArray *obj = new (result.ptr()) AbstractArray(clazz,
                                                          static_cast<uint32_t>(length),
                                                          static_cast<uint32_t>(length));
    // Zeroize all elements
    ::memset(obj + 1, 0, elems_field->type()->ref_size() * length);
    return obj;
}

AbstractArray *Machine::NewArrayCopiedSlow(const AbstractArray *origin, size_t increment,
                                           uint32_t flags) {
    const Class *clazz = origin->clazz();
    if (::strstr(clazz->name(), "array") != clazz->name()){
        NOREACHED() << "class: " << clazz->name() << " is not array!";
        return nullptr;
    }
    const Field *elems_field =
        DCHECK_NOTNULL(STATE->metadata_space()->FindClassFieldOrNull(clazz, "elems"));

    uint32_t length = static_cast<uint32_t>(origin->length() + increment);
    size_t request_size = sizeof(AbstractArray) + elems_field->type()->ref_size() * length;
    AllocationResult result = STATE->heap()->Allocate(request_size, flags);
    if (!result.ok()) {
        return nullptr;
    }
    AbstractArray *obj = new (result.ptr()) AbstractArray(clazz, length, length);
    Address dest = reinterpret_cast<Address>(obj) + elems_field->offset();
    const Byte *sour = reinterpret_cast<const Byte *>(origin) + elems_field->offset();

    // Copy data
    ::memcpy(dest, sour, origin->length() * elems_field->type()->ref_size());
    if (elems_field->type()->is_reference()) {
        // Write-Barrier:
        UpdateRememberRecords(obj, reinterpret_cast<Any **>(dest), origin->length());
    }

    if (increment > 0) {
        dest += elems_field->type()->ref_size() * origin->length();
        // Zeroize increment space
        ::memset(dest, 0, elems_field->type()->ref_size() * increment);
    }
    return obj;
}

Closure *Machine::NewClosure(Code *code, size_t captured_var_size, uint32_t flags) {
    DCHECK_EQ(0, captured_var_size % kPointerSize);
    size_t request_size = sizeof(Closure) + captured_var_size * sizeof(Closure::CapturedVar);
    AllocationResult result = STATE->heap()->Allocate(request_size, flags);
    if (!result.ok()) {
        return nullptr;
    }
    Closure *obj = new (result.ptr()) Closure(STATE->builtin_type(kType_closure),
                                              DCHECK_NOTNULL(code),
                                              static_cast<uint32_t>(captured_var_size), 0/*TODO: color*/);
    return obj;
}

Closure *Machine::NewClosure(Function *func, size_t captured_var_size, uint32_t flags) {
    DCHECK_EQ(0, captured_var_size % kPointerSize);
    size_t request_size = sizeof(Closure) + captured_var_size * sizeof(Closure::CapturedVar);
    AllocationResult result = STATE->heap()->Allocate(request_size, flags);
    if (!result.ok()) {
        return nullptr;
    }
    Closure *obj = new (result.ptr()) Closure(STATE->builtin_type(kType_closure),
                                              DCHECK_NOTNULL(func),
                                              static_cast<uint32_t>(captured_var_size), 0/*TODO: color*/);
    return obj;
}

void Machine::InsertFreeCoroutine(Coroutine *co) {
    QUEUE_INSERT_HEAD(free_dummy_, co);
    n_free_++;
}

Coroutine *Machine::TakeFreeCoroutine() {
    Coroutine *co = free_dummy_->next();
    if (co != free_dummy_) {
        n_free_--;
        QUEUE_REMOVE(co);
        return co;
    }
    return nullptr;
}

} // namespace lang

} // namespace mai
