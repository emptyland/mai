#include "lang/machine.h"
#include "lang/scheduler.h"
#include "lang/coroutine.h"
#include "lang/channel.h"
#include "lang/value-inl.h"
#include "lang/stack-frame.h"
#include "mai/allocator.h"
#include <memory>

namespace mai {

namespace lang {

inline uint32_t color_tags() {
    uint32_t tags = STATE->heap()->initialize_color();
    DCHECK(tags & Any::kColorMask);
    return tags;
}

Machine::Machine(int id, Scheduler *owner)
    : id_(id)
    , owner_(owner)
    , free_dummy_(Coroutine::NewDummy())
    , runnable_dummy_(Coroutine::NewDummy())
    , waitting_dummy_(Coroutine::NewDummy()) {
    top_slot_ = new HandleScopeSlot;
    top_slot_->scope = nullptr;
    top_slot_->prev  = nullptr;
    top_slot_->base  = nullptr;
    top_slot_->end   = top_slot_->base;
    top_slot_->limit = top_slot_->end;
}

Machine::~Machine() {
    // TODO:
    while (!QUEUE_EMPTY(waitting_dummy_)) {
        auto x = waitting_dummy_->next();
        QUEUE_REMOVE(x);
        delete x;
    }
    Coroutine::DeleteDummy(waitting_dummy_);
    
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
    {
        std::lock_guard<std::mutex> lock(runnable_mutex_);
        if (now) {
            QUEUE_INSERT_HEAD(runnable_dummy_, co);
        } else {
            QUEUE_INSERT_TAIL(runnable_dummy_, co);
        }
        n_runnable_++;
    }
    co->set_owner(this);

    if (Machine::This() != this) {
        if (state_.load(std::memory_order_acquire) == kIdle) {
            cond_var_.notify_one();
        }
    }
}

// Post coroutine to waitting list
void Machine::PostWaitting(Coroutine *co) {
    std::lock_guard<std::mutex> lock(waitting_mutex_);
    DCHECK_EQ(Coroutine::kWaitting, co->state());
    QUEUE_INSERT_TAIL(waitting_dummy_, co);
    n_waitting_++;
    co->set_owner(this);
}

void Machine::TakeWaittingCoroutine(Coroutine *co) {
    std::lock_guard<std::mutex> lock(waitting_mutex_);
    DCHECK_EQ(this, co->owner());
    DCHECK_EQ(Coroutine::kWaitting, co->state());
    QUEUE_REMOVE(co);
    n_waitting_--;
    co->set_owner(nullptr);
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
        bool should_exit = (id_ == 0);
        {
            std::lock_guard<std::mutex> lock(runnable_mutex_);
            if (n_runnable_ > 0) {
                DCHECK(!QUEUE_EMPTY(runnable_dummy_));
                co = runnable_dummy_->next();
                QUEUE_REMOVE(co);
                n_runnable_--;
                should_exit = false;
            }
        } {
            std::lock_guard<std::mutex> lock(waitting_mutex_);
            if (n_waitting_ > 0) {
                should_exit = false;
            }
        }
        if (should_exit) {
            break;
        }

        if (co) {
            CallStub<intptr_t(Coroutine *)> trampoline(STATE->metadata_space()->trampoline_code());
            
            DCHECK_EQ(Coroutine::kRunnable, co->state());
            running_ = co;
            TLS_STORAGE->coroutine = co;

            uint64_t delta = STATE->env()->CurrentTimeMicros();
            trampoline.entry()(co);
            user_time_ += STATE->env()->CurrentTimeMicros() - delta;

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

            state_.store(kIdle, std::memory_order_relaxed);
            owner_->MarkIdle(this);
            
            std::unique_lock<std::mutex> lock(mutex_);
            cond_var_.wait(lock);
            
            state_.store(kRunning, std::memory_order_relaxed);
            owner_->ClearIdle(this);
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

Any *Machine::NewObject(const Class *clazz, uint32_t flags) {
    DCHECK(!clazz->is_builtin());
    DCHECK(!clazz->is_primitive());
    DCHECK_GE(clazz->instrance_size(), sizeof(Any));

    AllocationResult result = STATE->heap()->Allocate(clazz->instrance_size(), flags);
    if (!result.ok()) {
        AllocationPanic(result);
        return nullptr;
    }
    
    Any *any = new (result.ptr()) Any(clazz, color_tags());
    
    if (clazz->n_fields() > 0) {
        Address base = reinterpret_cast<Address>(any) + clazz->field(0)->offset();
        ::memset(base, 0, clazz->instrance_size() - sizeof(Any)); // Zeroize object fields
    }
    return any;
}

// Factory for create a UTF-8 string
String *Machine::NewUtf8String(const char *utf8_string, size_t n, uint32_t flags) {
    if (running_ && (!utf8_string || n == 0)) {
        return STATE->factory()->empty_string();
    }
    size_t request_size = String::RequiredSize(n + 1);
    AllocationResult result = STATE->heap()->Allocate(request_size, flags);
    if (!result.ok()) {
        AllocationPanic(result);
        return nullptr;
    }
    String *obj = new (result.ptr()) String(STATE->builtin_type(kType_string),
                                            static_cast<uint32_t>(n + 1),
                                            utf8_string,
                                            static_cast<uint32_t>(n),
                                            color_tags());
    return obj;
}

static bool NeedInit(std::atomic<AbstractValue *> *address) {
    static AbstractValue *const kPendingValue =
        reinterpret_cast<AbstractValue *>(NumberValueSlot::kPendingMask);
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
    if (index < 0 || index >= Factory::kNumberOfCachedNumberValues) {
        return Machine::This()->NewNumber(primitive_type, value, n, 0);
    }
    auto address = DCHECK_NOTNULL(STATE->cached_number_value(primitive_type, index));
    if (!(reinterpret_cast<uintptr_t>(address->load(std::memory_order_acquire)) &
          NumberValueSlot::kCreatedMask) && NeedInit(address)) {
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
        AllocationPanic(result);
        return nullptr;
    }
    const Class *clazz = STATE->metadata_space()->GetNumberClassOrNull(primitive_type);
    DCHECK(clazz != nullptr) << "Bad type: " << primitive_type;
    return new (result.ptr()) AbstractValue(DCHECK_NOTNULL(clazz), value, n, 0);
}

CapturedValue *Machine::NewCapturedValue(const Class *clazz, const void *value, size_t n,
                                         uint32_t flags) {
    DCHECK_LE(clazz->reference_size(), n);
    AllocationResult result = STATE->heap()->Allocate(sizeof(CapturedValue), flags);
    if (!result.ok()) {
        AllocationPanic(result);
        return nullptr;
    }
    if (clazz->is_reference()) {
        return new (result.ptr()) CapturedValue(STATE->builtin_type(kType_captured_value),
                                                *static_cast<Any *const *>(value), color_tags());
    } else {
        return new (result.ptr()) CapturedValue(STATE->builtin_type(kType_captured_value),
                                                value, n, color_tags());
    }
}

AbstractArray *Machine::NewArray(BuiltinType type, size_t length, size_t capacity, uint32_t flags) {
    const Class *clazz = DCHECK_NOTNULL(STATE->builtin_type(type));
    if (::strstr(clazz->name(), "array") != clazz->name() &&
        ::strstr(clazz->name(), "mutable_array") != clazz->name()) {
        NOREACHED() << "class: " << clazz->name() << " is not array!";
        return nullptr;
    }

    const Field *elems_field =
        DCHECK_NOTNULL(STATE->metadata_space()->FindClassFieldOrNull(clazz, "elems"));

    DCHECK_LE(length, capacity);
    size_t request_size = sizeof(AbstractArray) + elems_field->type()->reference_size() * capacity;
    AllocationResult result = STATE->heap()->Allocate(request_size, flags);
    if (!result.ok()) {
        AllocationPanic(result);
        return nullptr;
    }
    AbstractArray *obj = new (result.ptr()) AbstractArray(clazz,
                                                          static_cast<uint32_t>(capacity),
                                                          static_cast<uint32_t>(length),
                                                          color_tags());
    // Zeroize all elements
    ::memset(reinterpret_cast<Address>(obj) + elems_field->offset(), 0,
             elems_field->type()->reference_size() * length);
    return obj;
}

AbstractArray *Machine::NewArrayCopied(const AbstractArray *origin, size_t increment,
                                           uint32_t flags) {
    const Class *clazz = origin->clazz();
    if (::strstr(clazz->name(), "array") != clazz->name()){
        NOREACHED() << "class: " << clazz->name() << " is not array!";
        return nullptr;
    }
    const Field *elems_field =
        DCHECK_NOTNULL(STATE->metadata_space()->FindClassFieldOrNull(clazz, "elems"));

    uint32_t length = static_cast<uint32_t>(origin->length() + increment);
    size_t request_size = sizeof(AbstractArray) + elems_field->type()->reference_size() * length;
    AllocationResult result = STATE->heap()->Allocate(request_size, flags);
    if (!result.ok()) {
        AllocationPanic(result);
        return nullptr;
    }
    AbstractArray *obj = new (result.ptr()) AbstractArray(clazz, length, length, color_tags());
    Address dest = reinterpret_cast<Address>(obj) + elems_field->offset();
    const Byte *sour = reinterpret_cast<const Byte *>(origin) + elems_field->offset();

    // Copy data
    ::memcpy(dest, sour, origin->length() * elems_field->type()->reference_size());
    if (elems_field->type()->is_reference()) {
        // Write-Barrier:
        UpdateRememberRecords(obj, reinterpret_cast<Any **>(dest), origin->length());
    }

    if (increment > 0) {
        dest += elems_field->type()->reference_size() * origin->length();
        // Zeroize increment space
        ::memset(dest, 0, elems_field->type()->reference_size() * increment);
    }
    return obj;
}

AbstractArray *Machine::NewMutableArray8(const void *init_data, size_t length, size_t capacity,
                                         uint32_t flags) {
    size_t request_size = sizeof(Array<uint8_t>) + 1 * capacity;
    AllocationResult result = STATE->heap()->Allocate(request_size, flags);
    if (!result.ok()) {
        AllocationPanic(result);
        return nullptr;
    }
    
    DCHECK_GE(capacity, length);
    Array<uint8_t> *obj = new (result.ptr()) Array<uint8_t>(STATE->builtin_type(kType_mutable_array8),
                                                            static_cast<uint32_t>(capacity),
                                                            static_cast<uint32_t>(length),
                                                            color_tags());
    ::memcpy(obj->elems_, init_data, length);
    return obj;
}

AbstractArray *Machine::ResizeMutableArray(AbstractArray *origin, size_t new_size) {
    if (!origin || origin->capacity() == new_size) {
        return origin;
    }
    
    const Class *clazz = origin->clazz();
    AbstractArray *copied = NewArray(static_cast<BuiltinType>(clazz->id()), origin->length(),
                                     new_size, 0/*flags*/);
    if (!copied) {
        return nullptr;
    }

    const Field *elems_field =
        DCHECK_NOTNULL(STATE->metadata_space()->FindClassFieldOrNull(clazz, "elems"));
    
    Address dest = reinterpret_cast<Address>(copied) + elems_field->offset();
    Address sour = reinterpret_cast<Address>(origin) + elems_field->offset();

    // Copy data
    ::memcpy(dest, sour, origin->length() * elems_field->type()->reference_size());
    if (elems_field->type()->is_reference()) {
        // Write-Barrier:
        UpdateRememberRecords(copied, reinterpret_cast<Any **>(dest), origin->length());
    }
    
    return copied;
}

String *Machine::Array8ToString(AbstractArray *from) {
    DCHECK(from->clazz() == STATE->builtin_type(kType_mutable_array8) ||
           from->clazz() == STATE->builtin_type(kType_array8));
    DCHECK_GT(from->capacity(), from->length());

    // Fill term zero
    static_cast<Array<char> *>(from)->elems_[from->length()] = 0;

    from->set_clazz(STATE->builtin_type(kType_string));
    return reinterpret_cast<String *>(from);
}

Closure *Machine::NewClosure(Code *code, size_t captured_var_size, uint32_t flags) {
    DCHECK_EQ(0, captured_var_size % kPointerSize);
    size_t request_size = sizeof(Closure) + captured_var_size * sizeof(Closure::CapturedVar);
    AllocationResult result = STATE->heap()->Allocate(request_size, flags);
    if (!result.ok()) {
        AllocationPanic(result);
        return nullptr;
    }
    Closure *obj = new (result.ptr()) Closure(STATE->builtin_type(kType_closure),
                                              DCHECK_NOTNULL(code),
                                              static_cast<uint32_t>(captured_var_size),
                                              Closure::kCxxFunction|color_tags());
    return obj;
}

Closure *Machine::CloseFunction(Function *func, uint32_t flags) {
    Address frame_bp = GetPrevStackFrame(running_->bp1(), running_->stack()->stack_hi());
    StackFrame::Maker maker = StackFrame::GetMaker(DCHECK_NOTNULL(frame_bp));

    Closure *closure = NewClosure(func, func->captured_var_size(), flags);
    for (uint32_t i = 0; i < func->captured_var_size(); i++) {
        auto desc = func->captured_var(i);
        switch (desc->kind) {
            case Function::IN_CAPTURED:
                if (maker == StackFrame::kBytecode) {
                    Closure *callee = BytecodeStackFrame::GetCallee(frame_bp);
                    closure->captured_var_[i].value = callee->captured_var_[desc->index].value;
                    closure->WriteBarrier(reinterpret_cast<Any **>(&closure->captured_var_[i].value));
                } else {
                    NOREACHED();
                }
                break;
            case Function::IN_STACK: {
                Address addr = frame_bp - (desc->index * 2);
                closure->captured_var_[i].value = NewCapturedValue(desc->type, addr,
                                                                   desc->type->reference_size(),
                                                                   flags);
                closure->WriteBarrier(reinterpret_cast<Any **>(&closure->captured_var_[i].value));
            } break;
            default:
                NOREACHED();
                break;
        }
    }
    return closure;
}

Closure *Machine::NewClosure(Function *func, size_t captured_var_size, uint32_t flags) {
    DCHECK_EQ(0, captured_var_size % kPointerSize);
    size_t request_size = sizeof(Closure) + captured_var_size * sizeof(Closure::CapturedVar);
    AllocationResult result = STATE->heap()->Allocate(request_size, flags);
    if (!result.ok()) {
        AllocationPanic(result);
        return nullptr;
    }
    Closure *obj = new (result.ptr()) Closure(STATE->builtin_type(kType_closure),
                                              DCHECK_NOTNULL(func),
                                              static_cast<uint32_t>(captured_var_size),
                                              Closure::kMaiFunction|color_tags());
    return obj;
}

Throwable *Machine::NewPanic(Panic::Level code, String *message, uint32_t flags) {
    AllocationResult result = STATE->heap()->Allocate(sizeof(Panic), flags);
    if (!result.ok()) {
        AllocationPanic(result);
        return nullptr;
    }
    Array<String *> *stacktrace = Throwable::MakeStacktrace(!running_ ? nullptr : running_->bp1());
    if (!stacktrace) {
        AllocationPanic(AllocationResult::OOM);
        return nullptr;
    }
    return new (result.ptr()) Panic(STATE->builtin_type(kType_Panic), stacktrace, code, message,
                                    color_tags());
}

Exception *Machine::NewException(uint32_t type, String *message, Exception *cause, uint32_t flags) {
    AllocationResult result = STATE->heap()->Allocate(sizeof(Exception), flags);
    if (!result.ok()) {
        AllocationPanic(result);
        return nullptr;
    }
    Array<String *> *stacktrace = Throwable::MakeStacktrace(!running_ ? nullptr : running_->bp1());
    if (!stacktrace) {
        AllocationPanic(AllocationResult::OOM);
        return nullptr;
    }
    
    return new (result.ptr()) Exception(STATE->builtin_type(kType_Exception), stacktrace, cause,
                                        message, color_tags());
}

Channel *Machine::NewChannel(uint32_t data_typeid, size_t capacity, uint32_t flags) {
    const Class *data_type = STATE->metadata_space()->type(data_typeid);
    AllocationResult result =
        STATE->heap()->Allocate(Channel::RequiredSize(data_type, static_cast<uint32_t>(capacity)),
                                flags);
    if (!result.ok()) {
        AllocationPanic(result);
        return nullptr;
    }
    
    return new (result.ptr()) Channel(data_type, data_type,
                                      static_cast<uint32_t>(capacity),
                                      color_tags());
}

void Machine::ThrowPanic(Panic::Level level, String *message) {
    Throwable *panic = NewPanic(level, message, 0);
    if (running_) {
        running_->AssociateException(panic);
    }
}

void Machine::AllocationPanic(AllocationResult::Result result) {
    switch (result) {
        case AllocationResult::FAIL:
        case AllocationResult::OOM:
        case AllocationResult::LIMIT:
            // OOM: Should print stackstrace right now
            PrintStacktrace(STATE->factory()->oom_text()->data());
            if (running_) {
                running_->AssociateException(STATE->factory()->oom_panic());
            }
            break;
        case AllocationResult::NOTHING:
        case AllocationResult::OK:
        default:
            NOREACHED();
            break;
    }
}

/*static*/ Address Machine::GetPrevStackFrame(Address frame_bp, Address stack_hi) {
    while (frame_bp < stack_hi) {
        StackFrame::Maker maker = StackFrame::GetMaker(frame_bp);
        switch (maker) {
            case StackFrame::kStub:
                break;
            case StackFrame::kBytecode:
                return *reinterpret_cast<Address *>(frame_bp);
            case StackFrame::kTrampoline:
            default:
                NOREACHED();
                break;
        }
        frame_bp = *reinterpret_cast<Address *>(frame_bp);
    }
    NOREACHED();
    return nullptr;
}

void Machine::PrintStacktrace(const char *message) {
    if (!running_) {
        ::fprintf(stderr, "👎Call PrintStackstrace() must be in executing\n");
        return;
    }

    ::fprintf(stderr, "❌Stacktrace: M:%d:C:%lld:😱[Panic](0) %s\n", id_, running_->coid(), message);

    Address frame_bp = running_->bp1();
    while (frame_bp < running_->stack()->stack_hi()) {
        StackFrame::Maker maker = StackFrame::GetMaker(frame_bp);
        switch (maker) {
            case StackFrame::kStub:
                // Ignore
                break;
            case StackFrame::kBytecode: {
                Closure *callee = BytecodeStackFrame::GetCallee(frame_bp);
                int32_t pc = BytecodeStackFrame::GetPC(frame_bp);
                
                ::fprintf(stderr, "    at %s", callee->function()->name());
                const SourceLineInfo *info = callee->function()->source_line_info();
                if (info) {
                    ::fprintf(stderr, "(%s:%d)\n", info->file_name(), info->FindSourceLine(pc));
                } else {
                    ::fprintf(stderr, "()\n");
                }
            } break;
            case StackFrame::kTrampoline:
                // Finalize
                return;
            default:
                NOREACHED();
                break;
        }
        frame_bp = *reinterpret_cast<Address *>(frame_bp);
    }
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
