#include "lang/runtime.h"
#include "lang/isolate-inl.h"
#include "lang/channel.h"
#include "lang/value-inl.h"
#include "lang/machine.h"
#include "lang/coroutine.h"
#include "lang/scheduler.h"
#include "glog/logging.h"

namespace mai {

namespace lang {

/*static*/ Any *Runtime::NewObject(const Class *clazz, uint32_t flags) {
    if (!clazz) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return nullptr;
    }
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

/*static*/ String *Runtime::BoolToString(intptr_t value) {
    return value ? STATE->factory()->true_string() : STATE->factory()->false_string();
}

/*static*/ String *Runtime::I8ToString(intptr_t value) {
    TODO();
    return nullptr;
}

/*static*/ String *Runtime::U8ToString(uintptr_t value) {
    TODO();
    return nullptr;
}

/*static*/ String *Runtime::I16ToString(intptr_t value) {
    TODO();
    return nullptr;
}

/*static*/ String *Runtime::U16ToString(uintptr_t value) {
    TODO();
    return nullptr;
}

/*static*/ String *Runtime::I32ToString(intptr_t value) {
    TODO();
    return nullptr;
}

/*static*/ String *Runtime::U32ToString(uintptr_t value) {
    TODO();
    return nullptr;
}

/*static*/ String *Runtime::IntToString(int value) {
    char buf[64] = {0};
    ::snprintf(buf, sizeof(buf), "%d", value);
    return Machine::This()->NewUtf8String(buf, ::strlen(buf), 0);
}

/*static*/ String *Runtime::UIntToString(unsigned value) {
    TODO();
    return nullptr;
}

/*static*/ String *Runtime::I64ToString(int64_t value) {
    TODO();
    return nullptr;
}

/*static*/ String *Runtime::U64ToString(uint64_t value) {
    TODO();
    return nullptr;
}

/*static*/ String *Runtime::F32ToString(float value) {
    char buf[64] = {0};
    ::snprintf(buf, sizeof(buf), "%f", value);
    return Machine::This()->NewUtf8String(buf, ::strlen(buf), 0);
}

/*static*/ String *Runtime::F64ToString(double value) {
    TODO();
    return nullptr;
}

/*static*/ String *Runtime::StringContact(String **parts, String **end) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    IncrementalStringBuilder builder;
    for (auto i = end - 1; i >= parts; i--) {
        builder.AppendString(Local<String>(*i));
    }
    return builder.QuickBuild();
}

/*static*/ Channel *Runtime::NewChannel(uint32_t data_typeid, uint32_t capacity) {
    return Machine::This()->NewChannel(data_typeid, capacity, 0/*flags*/);
}

/*static*/ void Runtime::ChannelClose(Channel *chan) {
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

/*static*/ Coroutine *Runtime::RunCoroutine(uint32_t flags, Closure *entry_point, Address params,
                                            uint32_t params_bytes_size) {
    if (!entry_point) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return nullptr;
    }
    if (STATE->scheduler()->shutting_down() > 0) {
        // Is shutting-donw, Ignore coroutine creation
        Coroutine::This()->RequestYield();
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

/*static*/ Closure *CloseFunction(Function *func, uint32_t flags) {
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
    fprintf(stdout, "‼️ Coroutine ABORT:\n    ☠️M:%dC:%lld:%s", m->id(), coid, message);
    abort();
#endif
}

/*static*/ void Runtime::Println(String *input) {
    // TODO:
    ::puts(input->data());
}

/*static*/ int Runtime::Any_HashCode(Any *any) {
    if (!any) {
        Machine::This()->ThrowPanic(Panic::kError, STATE->factory()->nil_error_text());
        return -1;
    }
    return static_cast<int>(bit_cast<intptr_t>(any) >> 2) | 0x1;
}

/*static*/ String *Runtime::Any_ToString(Any *any) {
    // TODO:
    return Machine::This()->NewUtf8String("[any]", 5, 0);
}

/*static*/ void Runtime::Exception_PrintStackstrace(Any *any) {
    // TODO:
}

} // namespace lang

} // namespace mai
