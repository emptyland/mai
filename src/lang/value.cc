#include "lang/value-inl.h"
#include "lang/machine.h"
#include "lang/coroutine.h"
#include "lang/heap.h"
#include "asm/utils.h"

namespace mai {

namespace lang {

#define MEMBER_OFFSET_OF(owns, field) \
    arch::ObjectTemplate<owns, int32_t>::OffsetOf(&owns :: field)

const int32_t Any::kOffsetKlass = MEMBER_OFFSET_OF(Any, klass_);
const int32_t Any::kOffsetTags = MEMBER_OFFSET_OF(Any, tags_);

const int32_t AbstractArray::kOffsetCapacity = MEMBER_OFFSET_OF(AbstractArray, capacity_);
const int32_t AbstractArray::kOffsetLength = MEMBER_OFFSET_OF(AbstractArray, length_);

const int32_t MutableMapEntry::kOffsetNext = MEMBER_OFFSET_OF(MutableMapEntry, next_);
const int32_t MutableMapEntry::kOffsetHash = MEMBER_OFFSET_OF(MutableMapEntry, hash_);
const int32_t MutableMapEntry::kOffsetKey = MEMBER_OFFSET_OF(MutableMapEntry, key_);
const int32_t MutableMapEntry::kOffsetValue = MEMBER_OFFSET_OF(MutableMapEntry, value_);

const int32_t AbstractValue::kOffsetValue = MEMBER_OFFSET_OF(AbstractValue, value_);

const int32_t Closure::kOffsetProto = MEMBER_OFFSET_OF(Closure, mai_fn_);
const int32_t Closure::kOffsetCode = MEMBER_OFFSET_OF(Closure, cxx_fn_);
const int32_t Closure::kOffsetCapturedVarSize = MEMBER_OFFSET_OF(Closure, captured_var_size_);
const int32_t Closure::kOffsetCapturedVar = MEMBER_OFFSET_OF(Closure, captured_var_);

const int32_t Throwable::kOffsetStacktraceBP = MEMBER_OFFSET_OF(Throwable, stacktrace_bp_);
const int32_t Throwable::kOffsetStacktraceSP = MEMBER_OFFSET_OF(Throwable, stacktrace_sp_);
const int32_t Throwable::kOffsetStacktracePC = MEMBER_OFFSET_OF(Throwable, stacktrace_pc_);

const int32_t Panic::kOffsetCode = MEMBER_OFFSET_OF(Panic, code_);
const int32_t Panic::kOffsetMessage = MEMBER_OFFSET_OF(Panic, message_);

// Use function call, it's slowly calling...
bool Any::SlowlyIs(uint32_t type_id) const { return QuicklyIs(type_id); }

int Any::WriteBarrier(Any **address, size_t n) {
    if (STATE->heap()->InNewArea(this)) {
        return 0;
    }
    return Machine::This()->UpdateRememberRecords(this, address, n);
}

/*static*/ AbstractArray *AbstractArray::NewArray(BuiltinType type, size_t length) {
    uint32_t flags = 0;
    if (length > 2 * base::kKB) { // bit array should in old-space
        flags |= kOldSpace;
    }
    return Machine::This()->NewArraySlow(type, length, flags);
}

/*static*/ AbstractArray *AbstractArray::NewArrayCopied(const AbstractArray *origin, size_t increment) {
    uint32_t flags = 0;
    if (origin->length() + increment > 2 * base::kKB) { // bit array should in old-space
        flags |= kOldSpace;
    }
    return Machine::This()->NewArrayCopiedSlow(origin, increment, flags);
}

/*static*/ Handle<String> String::NewUtf8(const char *utf8_string, size_t n) {
    if (!utf8_string || n == 0) {
        utf8_string = "";
        n = 0;
    }
    return Handle<String>(Machine::This()->NewUtf8String(utf8_string, n, 0/*flags*/));
}

MutableMap::MutableMap(const Class *clazz, uint32_t initial_bucket_shift, uint32_t random_seed)
    : Any(clazz, 0) {
    TODO();
}

/*static*/ Handle<Closure> Closure::New(Code *stub, uint32_t captured_var_size) {
    return Handle<Closure>(Machine::This()->NewClosure(stub, captured_var_size, Heap::kOld));
}

/*static*/ AbstractValue *AbstractValue::ValueOf(BuiltinType type, const void *value, size_t n) {
    return Machine::This()->ValueOfNumber(type, value, n);
}

/*static*/ Throwable *Throwable::NewPanic(int code, String *message) {
    return Machine::This()->NewPanic(code, message, 0/*flags*/);
}

void Throwable::PrintStackstrace(FILE *file) const {
    ::fprintf(file, "👎TODO:\n");
}

/*static*/ void Throwable::Throw(Handle<Throwable> exception) {
    Coroutine::This()->set_exception(*exception);
}

Panic::Panic(const Class *clazz, uint8_t *stacktrace_bp, uint8_t *stacktrace_sp,
             intptr_t stacktrace_pc, int code, String *message, uint32_t tags)
    : Throwable(clazz, stacktrace_bp, stacktrace_sp, stacktrace_pc, tags)
    , code_(code)
    , message_(message) {

    if (message) {
        WriteBarrier(reinterpret_cast<Any **>(&message_));
    }
}

} // namespace lang

} // namespace mai
