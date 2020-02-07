#include "lang/runtime.h"
#include "lang/isolate-inl.h"
#include "lang/value-inl.h"
#include "lang/machine.h"
#include "lang/coroutine.h"
#include "glog/logging.h"

namespace mai {

namespace lang {

template<class T>
static inline AbstractValue *ValueOf(intptr_t input) {
    T value = static_cast<T>(input);
    return Machine::This()->ValueOfNumber(kType_i8, &value, sizeof(value));
}

/*static*/ AbstractValue *Runtime::BoolValueOf(intptr_t input) {
    int8_t value = static_cast<int8_t>(input);
    return Machine::This()->ValueOfNumber(kType_i8, &value, sizeof(value));
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

} // namespace lang

} // namespace mai
