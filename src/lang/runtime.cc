#include "lang/runtime.h"
#include "lang/isolate-inl.h"
#include "lang/garbage-collector.h"
#include "lang/channel.h"
#include "lang/value-inl.h"
#include "lang/machine.h"
#include "lang/coroutine.h"
#include "lang/scheduler.h"
#include "lang/stack-frame.h"
#include "base/spin-locking.h"
#include "glog/logging.h"

namespace mai {

namespace lang {

// [Safepoint]
/*static*/ Any *Runtime::NewObject(const Class *clazz, uint32_t flags) {
    if (!clazz) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return nullptr;
    }
    SafepointScope safepoint(STATE->gc());
    return Machine::This()->NewObject(clazz, flags);
}

template<class T>
static inline AbstractValue *ValueOf(intptr_t input) {
    T value = static_cast<T>(input);
    return Machine::This()->ValueOfNumber(TypeTraits<T>::kType, &value, sizeof(value));
}

/*static*/ AbstractValue *Runtime::BoolValueOf(intptr_t input) {
    int8_t value = static_cast<int8_t>(input);
    return Machine::This()->ValueOfNumber(kType_bool, &value, sizeof(value));
}

/*static*/ AbstractValue *Runtime::I8ValueOf(intptr_t value) { return ValueOf<int8_t>(value); }
/*static*/ AbstractValue *Runtime::U8ValueOf(uintptr_t value) { return ValueOf<uint8_t>(value); }
/*static*/ AbstractValue *Runtime::I16ValueOf(intptr_t value) { return ValueOf<int16_t>(value); }
/*static*/ AbstractValue *Runtime::U16ValueOf(uintptr_t value) { return ValueOf<uint16_t>(value); }
/*static*/ AbstractValue *Runtime::I32ValueOf(intptr_t value) { return ValueOf<int32_t>(value); }
/*static*/ AbstractValue *Runtime::U32ValueOf(uintptr_t value) { return ValueOf<uint32_t>(value); }
/*static*/ AbstractValue *Runtime::IntValueOf(intptr_t value) { return ValueOf<int32_t>(value); }
/*static*/ AbstractValue *Runtime::UIntValueOf(uintptr_t value) { return ValueOf<uint32_t>(value); }
/*static*/ AbstractValue *Runtime::I64ValueOf(intptr_t value) { return ValueOf<int64_t>(value); }
/*static*/ AbstractValue *Runtime::U64ValueOf(uintptr_t value) { return ValueOf<uint64_t>(value); }

/*static*/ AbstractValue *Runtime::F32ValueOf(float value) {
    return Machine::This()->ValueOfNumber(kType_f32, &value, sizeof(value));
}

/*static*/ AbstractValue *Runtime::F64ValueOf(double value) {
    return Machine::This()->ValueOfNumber(kType_f64, &value, sizeof(value));
}

/*static*/ String *Runtime::BoolToString(int value) {
    return value ? STATE->factory()->true_string() : STATE->factory()->false_string();
}

/*static*/ String *Runtime::I8ToString(int8_t value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%" PRId8, value);
}

/*static*/ String *Runtime::U8ToString(uint8_t value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%" PRIu8, value);
}

/*static*/ String *Runtime::I16ToString(int16_t value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%" PRId16, value);
}

/*static*/ String *Runtime::U16ToString(uint16_t value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%" PRIu16, value);
}

/*static*/ String *Runtime::I32ToString(int32_t value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%" PRId32, value);
}

/*static*/ String *Runtime::U32ToString(uint32_t value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%" PRIu32, value);
}

/*static*/ String *Runtime::IntToString(int value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%d", value);
}

/*static*/ String *Runtime::UIntToString(unsigned value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%u", value);
}

/*static*/ String *Runtime::I64ToString(int64_t value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%" PRId64, value);
}

/*static*/ String *Runtime::U64ToString(uint64_t value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%" PRIu64, value);
}

/*static*/ String *Runtime::F32ToString(float value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%f", value);
}

/*static*/ String *Runtime::F64ToString(double value) {
    return Machine::This()->NewUtf8StringWithFormat(0, "%f", value);
}

// [Safepoint]
// 2018846
// 1269759
//  521255
/*static*/ String *Runtime::StringContact(String **parts, String **end) {
    SafepointScope safepoint(STATE->gc());
    IncrementalStringBuilder builder;
    for (auto i = end - 1; i >= parts; i--) {
        builder.AppendString(*i);
    }
    return builder.QuickBuild();
}

/*static*/ Channel *Runtime::NewChannel(uint32_t data_typeid, uint32_t capacity) {
    return Machine::This()->NewChannel(data_typeid, capacity, 0/*flags*/);
}

/*static*/ void Runtime::CloseChannel(Channel *chan) {
    if (!chan) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return;
    }

    std::lock_guard<std::mutex> lock(*chan->mutable_mutex());
    if (chan->has_close()) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->dup_close_chan_error_text());
        return;
    }
    chan->CloseLockless();
    DCHECK(chan->has_close());
}

template<class T>
static inline T InternalChannelRecv(Channel *chan) {
    if (!chan) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return 0;
    }
    if (chan->has_close()) {
        return 0;
    }
    T value = 0;
    chan->Recv(&value, sizeof(value));
    return value;
}

template<class T>
static inline void InternalChannelSendNoBarrier(Channel *chan, T value) {
    if (!chan) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
    }
    if (!chan->has_close()) {
        chan->SendNoBarrier(&value, sizeof(value));
    }
}

/*static*/ int32_t Runtime::ChannelRecv32(Channel *chan) {
    return InternalChannelRecv<int32_t>(chan);
}

/*static*/ int64_t Runtime::ChannelRecv64(Channel *chan) {
    return InternalChannelRecv<int64_t>(chan);
}

/*static*/ Any *Runtime::ChannelRecvPtr(Channel *chan) {
    return InternalChannelRecv<Any *>(chan);
}

/*static*/ float Runtime::ChannelRecvF32(Channel *chan) {
    return InternalChannelRecv<float>(chan);
}

/*static*/ double Runtime::ChannelRecvF64(Channel *chan) {
    return InternalChannelRecv<double>(chan);
}

/*static*/ void Runtime::ChannelSend64(Channel *chan, int64_t value) {
    InternalChannelSendNoBarrier<int64_t>(chan, value);
}

/*static*/ void Runtime::ChannelSend32(Channel *chan, int32_t value) {
    InternalChannelSendNoBarrier<int32_t>(chan, value);
}

/*static*/ void Runtime::ChannelSendPtr(Channel *chan, Any *value) {
    if (!chan) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
    }
    if (chan->has_close()) {
        chan->SendAny(value);
    }
}

/*static*/ void Runtime::ChannelSendF32(Channel *chan, float value) {
    InternalChannelSendNoBarrier<float>(chan, value);
}

/*static*/ void Runtime::ChannelSendF64(Channel *chan, double value) {
    InternalChannelSendNoBarrier<double>(chan, value);
}

/*static*/ Any *Runtime::WriteBarrierWithOffset(Any *host, int32_t offset) {
    DCHECK(!STATE->heap()->InNewArea(host));
    DCHECK_GE(offset, sizeof(Any));
    Address address = reinterpret_cast<Address>(host) + offset;
    Machine::This()->UpdateRememberRecords(host, reinterpret_cast<Any **>(address), 1);
    return host;
}

/*static*/ Any *Runtime::WriteBarrierWithAddress(Any *host, Any **address) {
    DCHECK(!STATE->heap()->InNewArea(host));
    Machine::This()->UpdateRememberRecords(host, address, 1);
    return host;
}

/*static*/ Coroutine *Runtime::RunCoroutine(uint32_t flags, Closure *entry_point, Address params,
                                            uint32_t params_bytes_size) {
    if (!entry_point) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return nullptr;
    }
    if (STATE->scheduler()->shutting_down() > 0) {
        // Is shutting-donw, Ignore coroutine creation
        Coroutine::This()->Yield();
        return nullptr;
    }

    Coroutine *co = STATE->scheduler()->NewCoroutine(entry_point, false/*co0*/);
    if (!co) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->new_coroutine_error_text());
        return nullptr;
    }
    
    // Prepare coroutine
    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
    co->CopyArgv(params, params_bytes_size);

    STATE->scheduler()->PostRunnableBalanced(co, true/*now*/);
    return co;
}

/*static*/ Throwable *Runtime::NewNilPointerPanic() {
    return Machine::This()->NewPanic(Panic::kFatal, STATE->factory()->nil_error_text(), 0);
}

/*static*/ Throwable *Runtime::NewStackoverflowPanic() {
    return Machine::This()->NewPanic(Panic::kFatal, STATE->factory()->stack_overflow_error_text(),
                                     0);
}

/*static*/ Throwable *Runtime::MakeStacktrace(Throwable *expect) {
    Address frame_bp = TLS_STORAGE->coroutine->bp1();
    Array<String *> *stackstrace = Throwable::MakeStacktrace(frame_bp);
    if (!stackstrace) {
        return nullptr;
    }
    DCHECK_NOTNULL(expect)->QuickSetStacktrace(stackstrace);
    return expect;
}

/*static*/ Closure *Runtime::CloseFunction(Function *func, uint32_t flags) {
    return Machine::This()->CloseFunction(func, flags);
}

/*static*/ void Runtime::DebugAbort(const char *message) {
    Machine *m = Machine::This();
    Coroutine *co = m->running();
    uint64_t coid = !co ? -1 : co->coid();

    if (co) {
        co->set_state(Coroutine::kPanic);
    }
#if defined(DEBUG) || defined(_DEBUG)
    DLOG(FATAL) << "‼️ Coroutine ABORT:\n    ☠️M:" << m->id() << ":C:" << coid << message;
#else
    fprintf(stdout, "‼️ Coroutine ABORT:\n    ☠️M:%dC:%" PRIu64 ":%s", m->id(), coid, message);
    abort();
#endif
}

/*static*/ void Runtime::Println(String *input) {
    // TODO:
    ::fwrite(input->data(), 1, input->length(), stdout);
    ::puts("");
}

/*static*/ void Runtime::Assert(int expect, String *message) {
    if (expect) { return; }
    HandleScope handle_scope(HandleScope::INITIALIZER);
    IncrementalStringBuilder builder;
    builder.AppendString("Assert fail: ");
    if (message) {
        builder.AppendString(Local<String>(message));
    } else {
        builder.AppendString("\"\"");
    }
    Machine::This()->ThrowPanic(Panic::kFatal, builder.QuickBuild());
}

/*static*/ void Runtime::Abort(String *message) {
    Machine::This()->ThrowPanic(Panic::kError, DCHECK_NOTNULL(message));
}

/*static*/ void Runtime::Fatal(String *message) {
    Machine::This()->ThrowPanic(Panic::kFatal, DCHECK_NOTNULL(message));
}

/*static*/ int Runtime::Object_HashCode(Any *any) {
    if (!any) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return -1;
    }
    return static_cast<int>(bit_cast<intptr_t>(any) >> 2) | 0x1;
}

/*static*/ String *Runtime::Object_ToString(Any *any) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    if (!any) {
        return Machine::This()->NewUtf8String("nil", 3, 0);
    } else {
        return static_cast<Object *>(any)->ToString();
    }
}

/*static*/ void Runtime::Exception_PrintStackstrace(Any *any) {
    // TODO:
}

/*static*/ int64_t Runtime::System_CurrentTimeMillis(Any */*any*/) {
    return STATE->env()->CurrentTimeMicros() / 1000L;
}

/*static*/ int64_t Runtime::System_MicroTime(Any */*any*/) {
    return STATE->env()->CurrentTimeMicros();
}

/*static*/ void Runtime::System_GC(Any */*any*/) { MinorGC(); }

// std::mutex mutex;

/*static*/ Any *Runtime::WaitGroup_Init(Any *self) {
    DCHECK(!::strcmp("lang.WaitGroup", self->clazz()->name()));
    const Field *number_of_works = self->clazz()->field(0);
    DCHECK(!::strcmp("numberOfWorks", number_of_works->name()));
    const Field *mutex = self->clazz()->field(1);
    DCHECK(!::strcmp("mutex", mutex->name()));
    const Field *request = self->clazz()->field(2);
    DCHECK(!::strcmp("requestDummy", request->name()));
    WaittingRequest *rq = self->UnsafeAccess<WaittingRequest>(request);
    rq->next_ = rq;
    rq->prev_ = rq;
    return self;
}

/*static*/ void Runtime::WaitGroup_Add(Any *self, int n) {
    DCHECK(!::strcmp("lang.WaitGroup", self->clazz()->name()));
    const Field *number_of_works_field = self->clazz()->field(0);
    DCHECK(!::strcmp("numberOfWorks", number_of_works_field->name()));
    const Field *mutex_field = self->clazz()->field(1);
    DCHECK(!::strcmp("mutex", mutex_field->name()));
    base::SpinMutex *mutex = self->UnsafeAccess<base::SpinMutex>(mutex_field);
    base::SpinLock lock(mutex);
    int *works = self->UnsafeAccess<int>(number_of_works_field);
    (*works) += n;
}

/*static*/ void Runtime::WaitGroup_Done(Any *self) {
    DCHECK(!::strcmp("lang.WaitGroup", self->clazz()->name()));
    const Field *number_of_works_field = self->clazz()->field(0);
    DCHECK(!::strcmp("numberOfWorks", number_of_works_field->name()));
    const Field *mutex_field = self->clazz()->field(1);
    DCHECK(!::strcmp("mutex", mutex_field->name()));
    base::SpinMutex *mutex = self->UnsafeAccess<base::SpinMutex>(mutex_field);
    base::SpinLock lock(mutex);
    int *works = self->UnsafeAccess<int>(number_of_works_field);
    if (*works == 0) {
        return; // Ignore
    }
    if (--(*works) == 0) {
        const Field *request = self->clazz()->field(2);
        DCHECK(!::strcmp("requestDummy", request->name()));
        WaittingRequest *req = self->UnsafeAccess<WaittingRequest>(request);
        if (!req->co) {
            return; // No waiter, Ignore
        }

        //req->co->Wakeup();
        while (!req->co->AcquireState(Coroutine::kWaitting, Coroutine::kRunnable)) {
            std::this_thread::yield();
        }
        Machine *owner = req->co->owner();
        owner->TakeWaittingCoroutine(req->co);
        owner->PostRunnable(req->co);
        //STATE->scheduler()->PostRunnableBalanced(req->co, true);
    }
}

/*static*/ void Runtime::WaitGroup_Wait(Any *self) {
    DCHECK(!::strcmp("lang.WaitGroup", self->clazz()->name()));
    const Field *number_of_works_field = self->clazz()->field(0);
    DCHECK(!::strcmp("numberOfWorks", number_of_works_field->name()));
    const Field *mutex_field = self->clazz()->field(1);
    DCHECK(!::strcmp("mutex", mutex_field->name()));
    base::SpinMutex *mutex = self->UnsafeAccess<base::SpinMutex>(mutex_field);
    base::SpinLock lock(mutex);

    if (self->UnsafeGetField<int>(number_of_works_field) > 0) {
        Coroutine *co = Coroutine::This();
        const Field *request = self->clazz()->field(2);
        DCHECK(!::strcmp("requestDummy", request->name()));
        WaittingRequest *rq = self->UnsafeAccess<WaittingRequest>(request);
        if (rq->co != nullptr) {
            // TODO:
        }
        rq->co = co;
        co->Yield(rq);
    }
}

/*static*/ int Runtime::CurrentSourceLine(int level) {
    Address frame_bp = Coroutine::This()->bp1();
    while (frame_bp < Coroutine::This()->stack()->stack_hi()) {
        StackFrame::Maker maker = StackFrame::GetMaker(frame_bp);
        switch (maker) {
            case StackFrame::kBytecode: {
                Function *fun = BytecodeStackFrame::GetCallee(frame_bp)->function();
                if (!level--) {
                    if (!fun->source_line_info()) {
                        return 0;
                    }
                    intptr_t pc = BytecodeStackFrame::GetPC(frame_bp);
                    return fun->source_line_info()->FindSourceLine(pc);
                }
            } break;
            case StackFrame::kStub:
                // Skip stub frame
                break;
            case StackFrame::kTrampoline:
                return 0;
            default:
                NOREACHED();
                break;
        }
        frame_bp = *reinterpret_cast<Address *>(frame_bp);
    }
    return 0;
}

/*static*/ String *Runtime::CurrentSourceName(int level) {
    Address frame_bp = Coroutine::This()->bp1();
    while (frame_bp < Coroutine::This()->stack()->stack_hi()) {
        StackFrame::Maker maker = StackFrame::GetMaker(frame_bp);
        switch (maker) {
            case StackFrame::kBytecode: {
                Function *fun = BytecodeStackFrame::GetCallee(frame_bp)->function();
                if (!level--) {
                    if (!fun->source_line_info()) {
                        return STATE->factory()->empty_string();
                    }
                    const MDStrHeader *name = MDStrHeader::FromStr(fun->source_line_info()->file_name());
                    return Machine::This()->NewUtf8String(name->data(), name->length(), 0);
                }
            } break;
            case StackFrame::kStub:
                // Skip stub frame
                break;
            case StackFrame::kTrampoline:
                return STATE->factory()->empty_string();
            default:
                NOREACHED();
                break;
        }
        frame_bp = *reinterpret_cast<Address *>(frame_bp);
    }
    return STATE->factory()->empty_string();
}

/*static*/ void Runtime::Sleep(uint64_t mills) {
    std::this_thread::sleep_for(std::chrono::milliseconds(mills));
}

/*static*/ int Runtime::CurrentMachineID() {
    return Machine::This()->id();
}

/*static*/ uint64_t Runtime::CurrentCoroutineID() {
    return Coroutine::This()->coid();
}

/*static*/ void Runtime::MinorGC() {
    if (STATE->scheduler()->shutting_down()) {
        return;
    }
    if (!STATE->gc()->AcquireState(GarbageCollector::kIdle, GarbageCollector::kReady)) {
        Machine::This()->Park();
        return;
    }
    if (!STATE->scheduler()->Pause()) {
        Machine::This()->Park();
        return;
    }
    STATE->gc()->MinorCollect();
    bool ok = STATE->gc()->AcquireState(GarbageCollector::kDone, GarbageCollector::kIdle);
    DCHECK(ok);
    (void)ok;
    STATE->scheduler()->Resume();
}

/*static*/ void Runtime::MajorGC() {
    if (STATE->scheduler()->shutting_down()) {
        return;
    }
    if (!STATE->gc()->AcquireState(GarbageCollector::kIdle, GarbageCollector::kReady)) {
        Machine::This()->Park();
        return;
    }
    if (!STATE->scheduler()->Pause()) {
        Machine::This()->Park();
        return;
    }
    STATE->gc()->MajorCollect();
    bool ok = STATE->gc()->AcquireState(GarbageCollector::kDone, GarbageCollector::kIdle);
    DCHECK(ok);
    (void)ok;
    STATE->scheduler()->Resume();
}

// [Safepoint]
/*static*/ Any *Runtime::GetMemoryHistogram() {
    SafepointScope safepoint_scope(STATE->gc());
    const Class *type = STATE->metadata_space()->FindClassOrNull("runtime.MemoryHistogram");
    Object *histogram = static_cast<Object *>(Machine::This()->NewObject(type, 0));
    MetadataSpace *ms = STATE->metadata_space();
    Heap *heap = STATE->heap();
    
    const Field *field = ms->FindClassFieldOrNull(type, "newSpaceSize");
    histogram->UnsafeSetField<uint64_t>(field, STATE->new_space_initial_size());
    
    field = ms->FindClassFieldOrNull(type, "oldSpaceSize");
    histogram->UnsafeSetField<uint64_t>(field, STATE->old_space_limit_size());
    
    field = ms->FindClassFieldOrNull(type, "newSpaceUsed");
    histogram->UnsafeSetField<uint64_t>(field, heap->new_space()->GetUsedSize());
    
    field = ms->FindClassFieldOrNull(type, "oldSpaceUsed");
    histogram->UnsafeSetField<uint64_t>(field, heap->old_space()->used_size() +
                                        heap->large_space()->used_size());
    
    field = ms->FindClassFieldOrNull(type, "newSpaceRSS");
    histogram->UnsafeSetField<uint64_t>(field, heap->new_space()->GetRSS());
    
    field = ms->FindClassFieldOrNull(type, "oldSpaceRSS");
    histogram->UnsafeSetField<uint64_t>(field, heap->old_space()->GetRSS() +
                                        heap->large_space()->rss_size());
    
    field = ms->FindClassFieldOrNull(type, "gcTick");
    histogram->UnsafeSetField<uint64_t>(field, STATE->gc()->tick());
    return histogram;
}

/*static*/ void Runtime::Schedule() { Coroutine::This()->Yield(); }

} // namespace lang

} // namespace mai
