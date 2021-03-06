#include "lang/value-inl.h"
#include "lang/machine.h"
#include "lang/coroutine.h"
#include "lang/heap.h"
#include "lang/stack.h"
#include "lang/stack-frame.h"
#include "asm/utils.h"
#include "base/slice.h"
#include "base/hash.h"
#include <stdarg.h>

namespace mai {

namespace lang {

#define MEMBER_OFFSET_OF(owns, field) \
    arch::ObjectTemplate<owns, int32_t>::OffsetOf(&owns :: field)

const int32_t Any::kOffsetKlass = MEMBER_OFFSET_OF(Any, klass_);
const int32_t Any::kOffsetTags = MEMBER_OFFSET_OF(Any, tags_);

const int32_t AbstractArray::kOffsetElemType = MEMBER_OFFSET_OF(AbstractArray, elem_type_);
const int32_t AbstractArray::kOffsetCapacity = MEMBER_OFFSET_OF(AbstractArray, capacity_);
const int32_t AbstractArray::kOffsetLength = MEMBER_OFFSET_OF(AbstractArray, length_);

const int32_t AbstractMap::kOffsetKeyType = MEMBER_OFFSET_OF(AbstractMap, key_type_);
const int32_t AbstractMap::kOffsetValueType = MEMBER_OFFSET_OF(AbstractMap, value_type_);
const int32_t AbstractMap::kOffsetRandomSeed = MEMBER_OFFSET_OF(AbstractMap, random_seed_);
const int32_t AbstractMap::kOffsetLength = MEMBER_OFFSET_OF(AbstractMap, length_);

const int32_t AbstractValue::kOffsetValue = MEMBER_OFFSET_OF(AbstractValue, value_);

const int32_t Closure::kOffsetProto = MEMBER_OFFSET_OF(Closure, mai_fn_);
const int32_t Closure::kOffsetCode = MEMBER_OFFSET_OF(Closure, cxx_fn_);
const int32_t Closure::kOffsetCapturedVarSize = MEMBER_OFFSET_OF(Closure, captured_var_size_);
const int32_t Closure::kOffsetCapturedVar = MEMBER_OFFSET_OF(Closure, captured_var_);

const int32_t Kode::kOffsetKind = MEMBER_OFFSET_OF(Kode, kind_);
const int32_t Kode::kOffsetSlot = MEMBER_OFFSET_OF(Kode, slot_);
const int32_t Kode::kOffsetSize = MEMBER_OFFSET_OF(Kode, size_);
const int32_t Kode::kOffsetOptimizationLevel = MEMBER_OFFSET_OF(Kode, optimization_level_);
const int32_t Kode::kOffsetSourceLineInfo = MEMBER_OFFSET_OF(Kode, source_line_info_);
const int32_t Kode::kOffsetInstructions = MEMBER_OFFSET_OF(Kode, instructions_);

const int32_t Throwable::kOffsetStacktrace = MEMBER_OFFSET_OF(Throwable, stacktrace_);

const int32_t Panic::kOffsetCode = MEMBER_OFFSET_OF(Panic, code_);
const int32_t Panic::kOffsetMessage = MEMBER_OFFSET_OF(Panic, message_);

uint32_t Hash<Any*>::operator () (Any *value) const {
    DCHECK(value != nullptr);
    if (!value) {
        return 0;
    }
    if (value->clazz()->id() == kType_string) {
        String *s = static_cast<String *>(value);
        return base::Hash::Js(s->data(), s->length());
    }
    uintptr_t h = reinterpret_cast<uintptr_t>(value);
    return static_cast<uint32_t>(h * h >> 16);
}

bool Hash<Any*>::operator () (Any *lhs, Any *rhs) const {
    if (!lhs || !rhs) {
        return false;
    }
    if (lhs->clazz() != rhs->clazz()) {
        return false;
    }
    if (lhs->clazz()->id() == kType_string) {
        String *l = static_cast<String *>(lhs);
        String *r = static_cast<String *>(rhs);
        return l->length() == r->length() && !::strncmp(l->data(), r->data(), l->length());
    }
    return lhs == rhs;
}

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

/*static*/
AbstractMap *AbstractMap::NewMap(BuiltinType key, BuiltinType value, uint32_t bucket_size) {
#if defined(DEBUG) || defined(_DEBUG)
    uint32_t seed = 0;
#else
    uint32_t seed = ::rand();
#endif
    return Machine::This()->NewMap(key, value, bucket_size, seed, 0);
}

AbstractMap *AbstractMap::NewMap(uint32_t bucket_size) {
#if defined(DEBUG) || defined(_DEBUG)
    uint32_t seed = 0;
#else
    uint32_t seed = ::rand();
#endif
    return Machine::This()->NewMap(key_type_->id(), value_type_->id(), bucket_size, seed, 0);
}

bool AbstractMap::IsKeyReferenceType() const { return key_type_->is_reference(); }

bool AbstractMap::IsValueReferenceType() const { return value_type_->is_reference(); }

/*static*/ Local<Closure> Closure::New(Code *stub, uint32_t captured_var_size) {
    return Local<Closure>(Machine::This()->NewClosure(stub, captured_var_size, Heap::kOld));
}

/*static*/ AbstractValue *AbstractValue::ValueOf(BuiltinType type, const void *value, size_t n) {
    return Machine::This()->ValueOfNumber(type, value, n);
}

/*static*/ Throwable *Throwable::NewPanic(int code, String *message) {
    return Machine::This()->NewPanic(static_cast<Panic::Level>(code), message, 0/*flags*/);
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
        static_cast<Array<String *> *>(Machine::This()->NewArray(kType_array, lines.size(),
                                                                 lines.size(), 0/*flags*/));
    if (!trace) {
        return nullptr;
    }
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
    AbstractArray *incomplete = Machine::This()->NewArray8(kType_i8, z, length, capacity, 0);
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
            static_cast<Array<char> *>(Machine::This()->ResizeArray(incomplete(), new_capacity));
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

Kode::Kode(const Class *clazz, Code::Kind kind, int32_t optimization_level, uint32_t size,
           const uint8_t *instr, uint32_t tags)
    : Any(clazz, tags)
    , kind_(kind)
    , optimization_level_(optimization_level)
    , size_(size) {
    ::memcpy(instructions_, instr, size_);
}

/*static*/ Local<Kode> Kode::New(Code::Kind kind, int32_t optimization_level, uint32_t size,
                                 const uint8_t *instr) {
    Kode *code = Machine::This()->NewCode(kind, optimization_level, size, instr, 0);
    return Local<Kode>(code);
}

void Object::Iterate(ObjectVisitor *visitor) {
    const Class *type = is_forward() ? forward()->clazz() : clazz();
    for (uint32_t i = 0; i < type->n_fields(); i++) {
        const Field *field = type->field(i);
        if (field->type()->is_reference()) {
            Any **addr = reinterpret_cast<Any **>(GetFieldAddress(field));
            if (*addr) {
                visitor->VisitPointer(this, addr);
            }
        }
    }
}

String *Object::ToString() const {
    IncrementalStringBuilder builder;
    const Class *klass = clazz();

    builder.AppendFormat("%s(", klass->name());
    if (klass->id() == kType_string) {
        const String *s = reinterpret_cast<const String *>(this);
        builder.AppendFormat("\"%s\")", s->data());
        return builder.QuickBuild();
    }
    for (uint32_t i = 0; i <klass->n_fields(); i++) {
        if (i > 0) {
            builder.AppendString(", ");
        }
        const Field *field = klass->field(i);
        builder.AppendFormat("%s:", field->name());
        switch (static_cast<BuiltinType>(field->type()->id())) {
            case kType_bool:
                if (UnsafeGetField<bool>(field)) {
                    builder.AppendString(STATE->factory()->true_string());
                } else {
                    builder.AppendString(STATE->factory()->false_string());
                }
                break;
            case kType_i8:
                builder.AppendFormat("%" PRIu8, UnsafeGetField<int8_t>(field));
                break;
            case kType_u8:
                builder.AppendFormat("%" PRIi8, UnsafeGetField<uint8_t>(field));
                break;
            case kType_i16:
                builder.AppendFormat("%" PRIi16, UnsafeGetField<int16_t>(field));
                break;
            case kType_u16:
                builder.AppendFormat("%" PRIu16, UnsafeGetField<uint16_t>(field));
                break;
            case kType_i32:
                builder.AppendFormat("%" PRIi32, UnsafeGetField<int32_t>(field));
                break;
            case kType_u32:
                builder.AppendFormat("%" PRIu32, UnsafeGetField<uint32_t>(field));
                break;
            case kType_int:
                builder.AppendFormat("%d", UnsafeGetField<int>(field));
                break;
            case kType_uint:
                builder.AppendFormat("%u", UnsafeGetField<unsigned>(field));
                break;
            case kType_i64:
                builder.AppendFormat("%" PRIi64, UnsafeGetField<int64_t>(field));
                break;
            case kType_u64:
                builder.AppendFormat("%" PRIu64, UnsafeGetField<uint64_t>(field));
                break;
            case kType_f32:
                builder.AppendFormat("%f", UnsafeGetField<float>(field));
                break;
            case kType_f64:
                builder.AppendFormat("%f", UnsafeGetField<double>(field));
                break;
            case kType_string:
                builder.AppendFormat("\"%s\"", UnsafeGetField<String *>(field)->data());
                break;
            case kType_closure:
                // TODO:
                break;
            case kType_channel:
                // TODO:
                break;
            default:
                if (field->type()->id() >= kUserTypeIdBase) {
                    Object *obj = UnsafeGetField<Object *>(field);
                    builder.AppendFormat("%s(...)", obj->clazz()->name());
                }
                break;
        }
    }
    builder.AppendString(")");
    return builder.QuickBuild();
}

} // namespace lang

} // namespace mai
