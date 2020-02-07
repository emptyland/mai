#include "lang/machine.h"
#include "lang/scheduler.h"
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

void Machine::PostRunnable(Coroutine *co, bool now) {
    DCHECK_EQ(Coroutine::kRunnable, co->state());
    std::lock_guard<std::mutex> lock(runnable_mutex_);
    {
        if (now) {
            QUEUE_INSERT_HEAD(runnable_dummy_, co);
        } else {
            QUEUE_INSERT_TAIL(runnable_dummy_, co);
        }
        n_runnable_++;
    }

    if (Machine::This() != this) {
        if (state_.load(std::memory_order_acquire) == kIdle) {
            cond_var_.notify_one();
        }
    }
}

void Machine::Start() {
    DCHECK(!thread_.joinable()); // Not running
    DCHECK_EQ(kDead, state_.load());
    thread_ = std::thread([this] () {
        MachineScope machine_scope(this);
        Entry();
    });
}

void Machine::Entry() {
    state_.store(kRunning, std::memory_order_relaxed);
    
    int span_shift = 1;
    while (owner_->shutting_down() <= 0) {
        Coroutine *co = nullptr;
        {
            std::lock_guard<std::mutex> lock(runnable_mutex_);
            if (n_runnable_ > 0) {
                DCHECK(!QUEUE_EMPTY(runnable_dummy_));
                co = runnable_dummy_->next();
                QUEUE_REMOVE(co);
                n_runnable_--;
            } else {
                if (id_ == 0) {
                    break;
                }
            }
        }

        if (co) {
            CallStub<intptr_t(Coroutine *)> trampoline(STATE->metadata_space()->trampoline_code());
            
            DCHECK_EQ(Coroutine::kRunnable, co->state());
            running_ = co;
            TLS_STORAGE->coroutine = co;
            trampoline.entry()(co);
            
            if (co->state() == Coroutine::kDead || co->state() == Coroutine::kPanic) {
                owner_->Schedule();
            } else if (co->state() == Coroutine::kInterrupted) {
                // Yield coroutine to re-schedule
                owner_->Schedule();
            } else {
                NOREACHED();
            }
        }

        int span_count = 0;
        while (span_count++ < span_shift) {
            std::this_thread::yield();
        }
        span_shift <<= 1;

        if (span_shift >= 1024) {
            span_shift = 1;
            state_.store(kIdle, std::memory_order_release);
            std::unique_lock<std::mutex> lock(mutex_);
            cond_var_.wait(lock);
        }
    }

    state_.store(kDead, std::memory_order_relaxed);
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

static bool NeedInit(std::atomic<AbstractValue *> *address) {
    static AbstractValue *const kPendingValue = reinterpret_cast<AbstractValue *>(NumberValueSlot::kPendingMask);
    AbstractValue *e = nullptr;
    if (address->compare_exchange_weak(e, kPendingValue)) {
        return true;
    }
    while (address->load(std::memory_order_acquire) == kPendingValue) {
        std::this_thread::yield();
    }
    return false;
}

template<class T>
static AbstractValue *GetOrInstallNumber(BuiltinType primitive_type, const void *value, size_t n) {
    int64_t index = *static_cast<const T *>(value);
    if (index < 0 || index >= Isolate::kNumberOfCachedNumberValues) {
        return Machine::This()->NewNumber(primitive_type, value, n, 0);
    }
    auto address = DCHECK_NOTNULL(STATE->cached_number_value(primitive_type, index));
    if (!(reinterpret_cast<uintptr_t>(address->load(std::memory_order_acquire)) & NumberValueSlot::kCreatedMask) &&
        NeedInit(address)) {
        AbstractValue *instance = Machine::This()->NewNumber(primitive_type, value, n, Heap::kOld);
        address->store(instance, std::memory_order_release);
    }
    return address->load();
}

AbstractValue *Machine::ValueOfNumber(BuiltinType primitive_type, const void *value, size_t n) {
    switch (primitive_type) {
        case kType_bool: {
            int index = *static_cast<const bool *>(value);
            return STATE->cached_number_value(primitive_type, index)->load(std::memory_order_relaxed);
        }
        case kType_f32:
        case kType_f64:
            return NewNumber(primitive_type, value, n, 0);

        case kType_i8:
            return GetOrInstallNumber<int8_t>(primitive_type, value, n);
        case kType_u8:
            return GetOrInstallNumber<uint8_t>(primitive_type, value, n);
        case kType_i16:
            return GetOrInstallNumber<int16_t>(primitive_type, value, n);
        case kType_u16:
            return GetOrInstallNumber<uint16_t>(primitive_type, value, n);
        case kType_i32:
            return GetOrInstallNumber<int32_t>(primitive_type, value, n);
        case kType_u32:
            return GetOrInstallNumber<uint32_t>(primitive_type, value, n);
        case kType_int:
            return GetOrInstallNumber<int32_t>(primitive_type, value, n);
        case kType_uint:
            return GetOrInstallNumber<uint32_t>(primitive_type, value, n);
        case kType_i64:
            return GetOrInstallNumber<int64_t>(primitive_type, value, n);
        case kType_u64:
            return GetOrInstallNumber<uint64_t>(primitive_type, value, n);
        default:
            NOREACHED();
            break;
    }
    return nullptr;
}

AbstractValue *Machine::NewNumber(BuiltinType primitive_type, const void *value, size_t n,
                                  uint32_t flags) {
    AllocationResult result = STATE->heap()->Allocate(sizeof(AbstractValue), flags);
    if (!result.ok()) {
        return nullptr;
    }
    const Class *clazz = STATE->metadata_space()->GetNumberClassOrNull(primitive_type);
    DCHECK(clazz != nullptr) << "Bad type: " << primitive_type;
    return new (result.ptr()) AbstractValue(DCHECK_NOTNULL(clazz), value, n, 0);
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
