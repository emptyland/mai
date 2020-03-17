#include "lang/value-inl.h"
#include "lang/machine.h"
#include "lang/coroutine.h"
#include "lang/heap.h"
#include "lang/stack.h"
#include "lang/stack-frame.h"
#include "asm/utils.h"
#include "base/slice.h"
#include <stdarg.h>

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

const int32_t Throwable::kOffsetStacktrace = MEMBER_OFFSET_OF(Throwable, stacktrace_);

const int32_t Panic::kOffsetCode = MEMBER_OFFSET_OF(Panic, code_);
const int32_t Panic::kOffsetMessage = MEMBER_OFFSET_OF(Panic, message_);

const int32_t Exception::kOffsetMessage = MEMBER_OFFSET_OF(Exception, message_);
const int32_t Exception::kOffsetCause = MEMBER_OFFSET_OF(Exception, cause_);

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
    return Machine::This()->NewArray(type, length, length, flags);
}

/*static*/ AbstractArray *AbstractArray::NewArrayCopied(const AbstractArray *origin, size_t increment) {
    uint32_t flags = 0;
    if (origin->length() + increment > 2 * base::kKB) { // bit array should in old-space
        flags |= kOldSpace;
    }
    return Machine::This()->NewArrayCopied(origin, increment, flags);
}

/*static*/ Local<String> String::NewUtf8(const char *utf8_string, size_t n) {
    if (!utf8_string || n == 0) {
        utf8_string = "";
        n = 0;
    }
    return Local<String>(Machine::This()->NewUtf8String(utf8_string, n, 0/*flags*/));
}

MutableMap::MutableMap(const Class *clazz, uint32_t initial_bucket_shift, uint32_t random_seed)
    : Any(clazz, 0) {
    TODO();
}

/*static*/ Local<Closure> Closure::New(Code *stub, uint32_t captured_var_size) {
    return Local<Closure>(Machine::This()->NewClosure(stub, captured_var_size, Heap::kOld));
}

/*static*/ AbstractValue *AbstractValue::ValueOf(BuiltinType type, const void *value, size_t n) {
    return Machine::This()->ValueOfNumber(type, value, n);
}

/*static*/ Throwable *Throwable::NewPanic(int code, String *message) {
    return Machine::This()->NewPanic(static_cast<Panic::Level>(code), message, 0/*flags*/);
}

// Handle<Any> argv[2] = { Handle<Any>::Null(), String::New("fail") };
// Handle<Throwable> e = NewUserObject<Throwable>("foo.DokiException", argv)
// Throwable::Throw(e);
/*static*/ Throwable *Throwable::NewException(String *message, Exception *cause) {
    return Machine::This()->NewException(kType_Exception, message, cause, 0/*flags*/);
}

void Throwable::PrintStackstrace(FILE *file) const {
    for (int i = 0; i < stacktrace_->length(); i++) {
        ::fprintf(file, "    at %s\n", stacktrace_->quickly_get(i)->data());
    }
}

/*static*/ Array<String *> *Throwable::MakeStacktrace(Address frame_bp) {
    Coroutine *co = TLS_STORAGE->coroutine;
    std::vector<String *> lines;
    if (!co) {
        char s[] = "👎Call PrintStackstrace() must be in executing";
        lines.push_back(Machine::This()->NewUtf8String(s, sizeof(s)-1, 0));
    } else {
        while (frame_bp < co->stack()->stack_hi()) {
            StackFrame::Maker maker = StackFrame::GetMaker(frame_bp);
            switch (maker) {
                case StackFrame::kStub:
                    // Ignore
                    break;
                case StackFrame::kBytecode: {
                    IncrementalStringBuilder builder;
                    Closure *callee = BytecodeStackFrame::GetCallee(frame_bp);
                    int32_t pc = BytecodeStackFrame::GetPC(frame_bp);
                    
                    builder.AppendString(callee->function()->name());
                    const SourceLineInfo *info = callee->function()->source_line_info();
                    if (info) {
                        builder.AppendFormat("(%s:%d)", info->file_name(), info->FindSourceLine(pc));
                    } else {
                        builder.AppendString("()");
                    }
                    lines.push_back(builder.QuickBuild());
                    if (lines.size() >= kMaxStacktraceLevels) {
                        lines.push_back(Machine::This()->NewUtf8String("...", 3, 0));
                        goto out;
                    }
                } break;
                case StackFrame::kTrampoline:
                    goto out;
                default:
                    NOREACHED();
                    break;
            }

            frame_bp = *reinterpret_cast<Address *>(frame_bp);
        }
    }
    
out:
    Array<String *> *trace =
        static_cast<Array<String *> *>(Machine::This()->NewArray(kType_array, lines.size(), lines.size(), 0/*flags*/));
    for (size_t i = 0; i < lines.size(); i++) {
        trace->quickly_set_nobarrier(i, lines[i]);
    }
    return trace;
}

/*static*/ void Throwable::Throw(const Handle<Throwable> &exception) {
    Coroutine::This()->AssociateException(*exception);
}

Panic::Panic(const Class *clazz, Array<String *> *stacktrace, Level code, String *message,
             uint32_t tags)
    : Throwable(clazz, stacktrace, tags)
    , code_(code)
    , message_(message) {
    
    if (message) {
        WriteBarrier(reinterpret_cast<Any **>(&message_));
    }
}

Exception::Exception(const Class *clazz, Array<String *> *stacktrace, Exception *cause,
                     String *message, uint32_t tags)
    : Throwable(clazz, stacktrace, tags)
    , cause_(cause)
    , message_(message) {
    if (cause) {
        WriteBarrier(reinterpret_cast<Any **>(&cause_));
    }
    if (message) {
        WriteBarrier(reinterpret_cast<Any **>(&message_));
    }
}

IncrementalStringBuilder::IncrementalStringBuilder(const char *z, size_t n) {
    size_t length = 0, capacity = 0;
    if (!z || n == 0) {
        z = "";
        length = 0;
        capacity = kInitSize;
    } else {
        length = n;
        capacity = n + kInitSize;
    }
    AbstractArray *incomplete = Machine::This()->NewMutableArray8(z, length, capacity, 0);
    incomplete_ = reinterpret_cast<Array<char> **>(GlobalHandles::NewHandle(incomplete));
}

void IncrementalStringBuilder::AppendFormat(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    AppendVFormat(fmt, ap);
    va_end(ap);
}

void IncrementalStringBuilder::AppendVFormat(const char *fmt, va_list ap) {
    AppendString(base::Vsprintf(fmt, ap));
}

void IncrementalStringBuilder::AppendString(const char *z, size_t n) {
    if (incomplete()->length() + n + 1 > incomplete()->capacity()) {
        size_t new_capacity = incomplete()->capacity() ? incomplete()->capacity() << 1 : kInitSize;
        new_capacity += n + 1;
        *incomplete_ =
            static_cast<Array<char> *>(Machine::This()->ResizeMutableArray(incomplete(),
                                                                           new_capacity));
    }
    incomplete()->QuicklyAppendNoResize(z, n);
}

String *IncrementalStringBuilder::Finish() const {
    if (!incomplete_ || !incomplete()) {
        return nullptr;
    }
    if (incomplete()->length() == 0) {
        return STATE->factory()->empty_string();
    }
    String *result = Machine::This()->Array8ToString(incomplete());
    *incomplete_ = nullptr;
    return result;
}

} // namespace lang

} // namespace mai
