#include "nyaa/nyaa-values.h"
#include "nyaa/function.h"
#include "nyaa/parser.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/heap.h"
#include "nyaa/object-factory.h"
#include "nyaa/string-pool.h"
#include "nyaa/memory.h"
#include "nyaa/builtin.h"
#include "base/arena-utils.h"
#include "base/arenas.h"
#include "base/hash.h"
#include "mai-lang/call-info.h"
#include "mai-lang/isolate.h"
#include "mai/env.h"
#include <limits>

namespace mai {
    
namespace nyaa {

/*static*/ Object *const Object::kNil = nullptr;
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// class Object:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
bool Object::IsKey(NyaaCore *N) const {
    switch (GetType()) {
        case kTypeSmi:
        case kTypeString:
        case kTypeFloat64:
            return true;
        default:
            break;
    }
    return false;
}

/*static*/ bool Object::LessThan(Object *lhs, Object *rhs, NyaaCore *N) {
    if (lhs == kNil || rhs == kNil) {
        return false;
    }
    if (lhs == rhs) {
        return false;
    }
    if (lhs->IsSmi()) {
        if (rhs->IsSmi()) {
            return lhs->ToSmi() < rhs->ToSmi();
        } else {
            return rhs->ToHeapObject()->LessThan(lhs, N);
        }
    }
    return lhs->ToHeapObject()->LessThan(rhs, N);
}

/*static*/ bool Object::LessEqual(Object *lhs, Object *rhs, NyaaCore *N) {
    if (lhs == kNil || rhs == kNil) {
        return false;
    }
    if (lhs == rhs) {
        return true;
    }
    if (lhs->IsSmi()) {
        if (rhs->IsSmi()) {
            return lhs->ToSmi() <= rhs->ToSmi();
        } else {
            return rhs->ToHeapObject()->LessEqual(lhs, N);
        }
    }
    return lhs->ToHeapObject()->LessEqual(rhs, N);
}
    
bool Object::IsFalse() const {
    switch (GetType()) {
        case kTypeNil:
            return true;
        case kTypeSmi:
            return ToSmi() == 0;
        case kTypeString:
            return NyString::Cast(this)->size() == 0;
        case kTypeFloat64:
            return NyFloat64::Cast(this)->value() == 0;
        default:
            break;
    }
    return false;
}

#define DEFINE_OBJECT_BIN_ARITH(op) \
Object *Object::op(Object *lhs, Object *rhs, NyaaCore *N) { \
    if (lhs == Object::kNil || rhs == Object::kNil) { \
        return Object::kNil; \
    } \
    if (lhs->IsObject()) { \
        return lhs->ToHeapObject()->op(rhs, N); \
    } else { \
        return NySmi::op(lhs, rhs, N); \
    } \
}

DEFINE_OBJECT_BIN_ARITH(Add)
DEFINE_OBJECT_BIN_ARITH(Sub)
DEFINE_OBJECT_BIN_ARITH(Mul)
DEFINE_OBJECT_BIN_ARITH(Div)
DEFINE_OBJECT_BIN_ARITH(Mod)
DEFINE_OBJECT_BIN_ARITH(Shl)
DEFINE_OBJECT_BIN_ARITH(Shr)
DEFINE_OBJECT_BIN_ARITH(BitAnd)
DEFINE_OBJECT_BIN_ARITH(BitOr)
DEFINE_OBJECT_BIN_ARITH(BitXor)
    
#undef DEFINE_OBJECT_BIN_ARITH
    
/*static*/ Object *Object::Minus(Object *lhs, NyaaCore *N) {
    if (lhs == Object::kNil) {
        return Object::kNil;
    }
    return lhs->IsObject() ? lhs->ToHeapObject()->Minus(N) : NySmi::Minus(lhs, N);
}
    
/*static*/ Object *Object::BitInv(Object *lhs, NyaaCore *N) {
    if (lhs == Object::kNil) {
        return Object::kNil;
    }
    return lhs->IsObject() ? lhs->ToHeapObject()->BitInv(N) : NySmi::BitInv(lhs, N);
}
    
/*static*/ Object *Object::Get(Object *lhs, Object *key, NyaaCore *N) {
    switch (lhs->GetType()) {
        case kTypeMap:
            return NyMap::Cast(lhs)->Get(key, N);
        case kTypeThread:
        case kTypeUdo:
            return NyUDO::Cast(lhs)->Get(key, N);
        default:
            N->Raisef("attempt get `%s' field.", kBuiltinTypeName[lhs->GetType()]);
            break;
    }
    return nullptr;
}

/*static*/ void Object::Put(Object *lhs, Object *key, Object *value, NyaaCore *N) {
    switch (lhs->GetType()) {
        case kTypeMap:
            NyMap::Cast(lhs)->Put(key, value, N);
            break;
        case kTypeThread:
        case kTypeUdo:
            NyUDO::Cast(lhs)->Put(key, value, N);
            break;
        default:
            N->Raisef("attempt set `%s' field.", kBuiltinTypeName[lhs->GetType()]);
            break;
    }
}

NyString *Object::ToString(NyaaCore *N) {
    if (this == kNil) {
        return N->bkz_pool()->kNil;
    }
    if (IsSmi()) {
        return N->factory()->Sprintf("%" PRId64 , ToSmi());
    }
    return ToHeapObject()->ToString(N);
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NySmi:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
/*static*/ Object *NySmi::Add(Object *lhs, Object *rhs, NyaaCore *N) {
    DCHECK(lhs->IsSmi());
    auto lval = lhs->ToSmi();
    if (rhs->IsSmi()) {
        auto rval = rhs->ToSmi();
        // kMinValue <= lval + rval <= kMaxValue
        // lval <= kMaxValue - rval
        if (((rval > 0) && (lval > (kMaxValue - rval))) ||
            ((rval < 0) && (lval < (kMinValue - rval)))) {
            // TODO:
            return New(lval + rval);
        }
        return New(lval + rval);
    }
    return rhs->ToHeapObject()->Add(lhs, N);
}
    
/*static*/ Object *NySmi::Sub(Object *lhs, Object *rhs, NyaaCore *N) {
    DCHECK(lhs->IsSmi());
    auto lval = lhs->ToSmi();
    switch (rhs->GetType()) {
        case kTypeSmi: {
            auto rval = rhs->ToSmi();
            // kMinValue <= lval - rval <= kMaxValue
            // lval - rval < kMinValue -> lval < kMinValue + rval
            // lval - rval > kMaxValue -> lval > kMaxValue + rval
            //
            if (((rval > 0) && (lval < (kMinValue + rval))) ||
                ((rval < 0) && (lval > (kMaxValue + rval)))) {
                // TODO:
                return New(lval - rval);
            }
            return New(lval - rval);
        } break;
            
        case kTypeFloat64:
            return N->factory()->NewFloat64(lval - NyFloat64::Cast(rhs)->value());
            
        default:
            break;
    }
    N->Raisef("smi attempt to call nil `__sub__' meta function.");
    return nullptr;
}
    
/*static*/ Object *NySmi::Mul(Object *lhs, Object *rhs, NyaaCore *N) {
    DCHECK(lhs->IsSmi());
    auto lval = lhs->ToSmi();
    if (rhs->IsSmi()) {
        bool overflow = false;
        auto rval = rhs->ToSmi();
        if (lval > 0) {  /* lval is positive */
            if (rval > 0) {  /* lval and rval are positive */
                if (lval > (kMaxValue / rval)) {
                    overflow = true;
                }
            } else { /* lval positive, rval nonpositive */
                if (rval < (kMinValue / lval)) {
                    overflow = true;
                }
            } /* lval positive, rval nonpositive */
        } else { /* lval is nonpositive */
            if (rval > 0) { /* lval is nonpositive, rval is positive */
                if (lval < (kMinValue / rval)) {
                    overflow = true;
                }
            } else { /* lval and rval are nonpositive */
                if ( (lval != 0) && (rval < (kMaxValue / lval))) {
                    overflow = true;
                }
            } /* End if lval and rval are nonpositive */
        }
        if (overflow) {
            // TODO:
            return New(lval * rval);
        }
        return New(lval * rval);
    }
    return rhs->ToHeapObject()->Mul(lhs, N);
}
    
/*static*/ Object *NySmi::Div(Object *lhs, Object *rhs, NyaaCore *N) {
    DCHECK(lhs->IsSmi());
    auto lval = lhs->ToSmi();
    switch (rhs->GetType()) {
        case kTypeSmi: {
            auto rval = rhs->ToSmi();
            if (!rval) {
                N->Raisef("div zero.");
                return nullptr;
            }
            return New(lval / rval);
        } break;

        case kTypeFloat64:
            return N->factory()->NewFloat64(lval / NyFloat64::Cast(rhs)->value());

        default:
            break;
    }
    N->Raisef("smi attempt to call nil `__div__' meta function.");
    return nullptr;
}
    
/*static*/ Object *NySmi::Mod(Object *lhs, Object *rhs, NyaaCore *N) {
    DCHECK(lhs->IsSmi());
    auto lval = lhs->ToSmi();
    switch (rhs->GetType()) {
        case kTypeSmi: {
            auto rval = rhs->ToSmi();
            if (!rval) {
                N->Raisef("div zero.");
                return nullptr;
            }
            return New(lval % rval);
        } break;
        case kTypeFloat64:
            return N->factory()->NewFloat64(::fmod(lval, NyFloat64::Cast(rhs)->value()));
        default:
            break;
    }
    N->Raisef("smi attempt to call nil `__mod__' meta function.");
    return nullptr;
}
    
/*static*/ Object *NySmi::Shl(Object *lhs, Object *rhs, NyaaCore *N) {
    DCHECK(lhs->IsSmi());
    auto lval = lhs->ToSmi();
    int n = 0;
    switch (rhs->GetType()) {
        case kTypeSmi:
            n = static_cast<int>(rhs->ToSmi());
            break;
        case kTypeFloat64:
            n = static_cast<int>(NyFloat64::Cast(rhs)->value());
            break;
        default:
            N->Raisef("incorrect type `%s' attempt shl.", kBuiltinTypeName[rhs->GetType()]);
            return Object::kNil;
    }
    int leading1 = 64 - base::Bits::CountLeadingZeros64(lval);
    if (leading1 + n > 62) { // overflow
        // TODO:
        return NySmi::New(lval << n);
    }
    return NySmi::New(lval << n);
}

/*static*/ Object *NySmi::Shr(Object *lhs, Object *rhs, NyaaCore *N) {
    DCHECK(lhs->IsSmi());
    auto lval = lhs->ToSmi();
    int n = 0;
    switch (rhs->GetType()) {
        case kTypeSmi:
            n = static_cast<int>(rhs->ToSmi());
            break;
        case kTypeFloat64:
            n = static_cast<int>(NyFloat64::Cast(rhs)->value());
            break;
        default:
            N->Raisef("incorrect type `%s' attempt shr.", kBuiltinTypeName[rhs->GetType()]);
            return Object::kNil;
    }
    return NySmi::New(static_cast<uint64_t>(lval) >> n);
}

/*static*/ Object *NySmi::BitAnd(Object *lhs, Object *rhs, NyaaCore *N) {
    DCHECK(lhs->IsSmi());
    if (NyString *s = NyString::Cast(rhs)) {
        rhs = s->TryNumeric(N);
    }
    if (!rhs->IsSmi()) {
        N->Raisef("incorrect type `%s' attempt bit and.", kBuiltinTypeName[rhs->GetType()]);
        return Object::kNil;
    }
    return NySmi::New(lhs->ToSmi() & rhs->ToSmi());
}

/*static*/ Object *NySmi::BitOr(Object *lhs, Object *rhs, NyaaCore *N) {
    DCHECK(lhs->IsSmi());
    if (NyString *s = NyString::Cast(rhs)) {
        rhs = s->TryNumeric(N);
    }
    if (!rhs->IsSmi()) {
        N->Raisef("incorrect type `%s' attempt bit or.", kBuiltinTypeName[rhs->GetType()]);
        return Object::kNil;
    }
    return NySmi::New(lhs->ToSmi() | rhs->ToSmi());
}

/*static*/ Object *NySmi::BitXor(Object *lhs, Object *rhs, NyaaCore *N) {
    DCHECK(lhs->IsSmi());
    if (NyString *s = NyString::Cast(rhs)) {
        rhs = s->TryNumeric(N);
    }
    if (!rhs->IsSmi()) {
        N->Raisef("incorrect type `%s' attempt bit xor.", kBuiltinTypeName[rhs->GetType()]);
        return Object::kNil;
    }
    return NySmi::New(lhs->ToSmi() ^ rhs->ToSmi());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyObject:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
void NyObject::SetMetatable(NyMap *mt, NyaaCore *N) {
    if (mt == Object::kNil) {
        return; // ignore
    }
    N->heap()->BarrierWr(this, reinterpret_cast<Object **>(&mtword_), mt, true /*ismt*/);
#if defined(NYAA_USE_POINTER_COLOR) || defined(NYAA_USE_POINTER_TYPE)
    uintptr_t bits = reinterpret_cast<uintptr_t>(mt);
    DCHECK_EQ(bits & kDataMask, 0);
    mtword_ = (bits | (mtword_ & kDataMask));
#else // !defined(NYAA_USE_POINTER_COLOR) && !defined(NYAA_USE_POINTER_TYPE)
    mtword_ = reinterpret_cast<uintptr_t>(mt);
#endif // defined(NYAA_USE_POINTER_COLOR) || defined(NYAA_USE_POINTER_TYPE)
}

bool NyObject::Equal(Object *rhs, NyaaCore *N) {
    if (rhs == nullptr) {
        return false;
    }

    switch (GetType()) {
        case kTypeString: {
            NyString *rval = NyString::Cast(rhs);
            if (!rval) {
                return false;
            }
            NyString *lval = ToString();
            return lval->size() <= kLargeStringLength ? lval == rval : lval->Compare(rval) == 0;
        } break;
        case kTypeFloat64:
            return ToFloat64()->Equal(rhs, N);
        case kTypeMap:
            return ToMap()->Equal(rhs, N);
        case kTypeUdo:
            return ToUDO()->Equal(rhs, N);
        default:
            break;
    }

    N->Raisef("type can not be compare.");
    return false;
}

bool NyObject::LessThan(Object *rhs, NyaaCore *N) {
    if (rhs == nullptr) {
        return false;
    }
    switch (GetType()) {
        case kTypeString: {
            NyString *lval = ToString();
            NyString *rval = NyString::Cast(rhs);
            if (!rval) {
                return false;
            }
            return lval->Compare(rval) < 0;
        } break;
        case kTypeFloat64:
            return ToFloat64()->LessThan(rhs, N);
        case kTypeMap:
            return ToMap()->LessThan(rhs, N);
        case kTypeUdo:
            return ToUDO()->LessThan(rhs, N);
        default:
            break;
    }
    N->Raisef("type can not be compare.");
    return false;
}

bool NyObject::LessEqual(Object *rhs, NyaaCore *N) {
    if (rhs == nullptr) {
        return false;
    }
    
    switch (GetType()) {
        case kTypeString: {
            NyString *lval = ToString();
            NyString *rval = NyString::Cast(rhs);
            if (!rval) {
                return false;
            }
            return lval->Compare(rval) <= 0;
        } break;
        case kTypeFloat64:
            return ToFloat64()->LessEqual(rhs, N);
        case kTypeMap:
            return ToMap()->LessEqual(rhs, N);
        case kTypeUdo:
            return ToUDO()->LessEqual(rhs, N);
        default:
            break;
    }
    N->Raisef("type can not be compare.");
    return false;
}
    
void NyObject::Iterate(ObjectVisitor *visitor) {
    DCHECK(is_direct());
    visitor->VisitMetatablePointer(this, &mtword_);
    
    switch (GetType()) {
    #define DEFINE_ITERATE(type) \
        case kType##type: \
            static_cast<Ny##type *>(this)->Iterate(visitor); \
            break;
            
        // TODO: DECL_BUILTIN_TYPES(DEFINE_ITERATE)
    #undef DEFINE_PLACED_SIZE
        case kTypeUdo:
            static_cast<NyUDO *>(this)->Iterate(visitor);
            break;
        default:
            NOREACHED();
            break;
    }
}
    
size_t NyObject::PlacedSize() const {
    size_t bytes = 0;
    switch (GetType()) {
#define DEFINE_PLACED_SIZE(type) \
    case kType##type: \
        bytes = static_cast<const Ny##type *>(this)->PlacedSize(); \
        break;

        // TODO: DECL_BUILTIN_TYPES(DEFINE_PLACED_SIZE)
#undef DEFINE_PLACED_SIZE
        case kTypeUdo:
            static_cast<const NyUDO *>(this)->PlacedSize();
            break;
        default:
            NOREACHED();
            break;
    }
    return RoundUp(bytes, kAllocateAlignmentSize);
}

#define DEFINE_HEAP_OBJECT_BIN_ARITH(name, literal) \
    Object *NyObject::name(Object *rhs, NyaaCore *N) { \
        if (NyString *s = NyString::Cast(rhs)) { \
            rhs = s->TryNumeric(N); \
        } \
        switch (GetType()) { \
            case kTypeFloat64: \
                return ToFloat64()->name(rhs, N); \
            case kTypeString: \
                return Object::name(ToString()->TryNumeric(N), rhs, N); \
            case kTypeMap: \
                return ToMap()->name(rhs, N); \
            case kTypeUdo: \
                return ToUDO()->name(rhs, N); \
            default: \
                break; \
        } \
        N->Raisef("type can not be `" literal "'."); \
        return nullptr; \
    }

DEFINE_HEAP_OBJECT_BIN_ARITH(Add, "+")
DEFINE_HEAP_OBJECT_BIN_ARITH(Sub, "-")
DEFINE_HEAP_OBJECT_BIN_ARITH(Mul, "*")
DEFINE_HEAP_OBJECT_BIN_ARITH(Div, "/")
DEFINE_HEAP_OBJECT_BIN_ARITH(Mod, "%")
    
#undef DEFINE_HEAP_OBJECT_BIN_ARITH
    
Object *NyObject::Minus(NyaaCore *N) {
    Object *lhs = this;
    if (NyString *s = NyString::Cast(this)) {
        lhs = s->TryNumeric(N);
    }
    switch (lhs->GetType()) {
        case kTypeSmi:
            return NySmi::Minus(lhs, N);
        case kTypeFloat64:
            return N->factory()->NewFloat64(-NyFloat64::Cast(lhs)->value());
        case kTypeMap:
            return NyMap::Cast(lhs)->Minus(N);
        case kTypeUdo:
            return NyUDO::Cast(lhs)->Minus(N);
        default:
            N->Raisef("incorrect type `%s' attempt minus(-unary).", kBuiltinTypeName[GetType()]);
            break;
    }
    return Object::kNil;
}
    
Object *NyObject::Shl(Object *rhs, NyaaCore *N) {
    if (NyString *s = NyString::Cast(rhs)) {
        rhs = s->TryNumeric(N);
    }
    switch (GetType()) {
        case kTypeFloat64:
            return ToFloat64()->Shl(rhs, N);
        default:
            N->Raisef("incorrect type `%s' attempt shl(<<).", kBuiltinTypeName[GetType()]);
            break;
    }
    return Object::kNil;
}

Object *NyObject::Shr(Object *rhs, NyaaCore *N) {
    if (NyString *s = NyString::Cast(rhs)) {
        rhs = s->TryNumeric(N);
    }
    switch (GetType()) {
        case kTypeFloat64:
            return ToFloat64()->Shr(rhs, N);
        default:
            N->Raisef("incorrect type `%s' attempt shr(>>).", kBuiltinTypeName[GetType()]);
            break;
    }
    return Object::kNil;
}

Object *NyObject::BitAnd(Object *rhs, NyaaCore *N) {
    N->Raisef("incorrect type `%s' attempt bit and(&).", kBuiltinTypeName[GetType()]);
    return Object::kNil;
}

Object *NyObject::BitOr(Object *rhs, NyaaCore *N) {
    N->Raisef("incorrect type `%s' attempt bit or(|).", kBuiltinTypeName[GetType()]);
    return Object::kNil;
}

Object *NyObject::BitXor(Object *rhs, NyaaCore *N) {
    N->Raisef("incorrect type `%s' attempt bit xor(^).", kBuiltinTypeName[GetType()]);
    return Object::kNil;
}
    
Object *NyObject::BitInv(NyaaCore *N) {
    N->Raisef("incorrect type `%s' attempt bit inv(~).", kBuiltinTypeName[GetType()]);
    return Object::kNil;
}

NyString *NyObject::ToString(NyaaCore *N) {
    switch (GetType()) {
        case kTypeString:
            return (const_cast<NyObject *>(this))->ToString();
        case kTypeFloat64:
            return N->factory()->Sprintf("%f", ToFloat64()->value());
        case kTypeClosure:
            return N->factory()->Sprintf("closure: %p", this);
        case kTypeFunction:
            return N->factory()->Sprintf("function: %p", this);
        case kTypeMap:
            return ToMap()->ToString(N);
        case kTypeUdo:
            return ToUDO()->ToString(N);
        case kTypeDelegated:
            return N->factory()->Sprintf("delegated: %p(%p)", this, ToDelegated()->fp_addr());
        case kTypeThread:
            return N->factory()->Sprintf("thread: %p", this);
        default:
            break;
    }
    NOREACHED() << GetType();
    return nullptr;
}

Object *NyObject::AttemptBinaryMetaFunction(Object *rhs, NyString *name, NyaaCore *N) {
    if (NyRunnable *fn = GetValidMetaFunction(name, N)) {
        // TODO: Object *args[] = {this, rhs};
        // N->curr_thd()->Run(fn, args, 2/*nargs*/, 1/*nrets*/, N->curr_thd()->CurrentEnv()/*env*/);
//        Object *rv = N->Get(-1);
//        N->Pop(1);
//        return rv;
        TODO();
        return Object::kNil;
    }
    N->Raisef("attempt to call nil `%s' meta function.", name->bytes());
    return Object::kNil;
}

Object *NyObject::AttemptUnaryMetaFunction(NyString *name, NyaaCore *N) {
    if (NyRunnable *fn = GetValidMetaFunction(name, N)) {
        // TODO: Object *args[] = {this};
        // N->curr_thd()->Run(fn, args, 1/*nargs*/, 1/*nrets*/, N->curr_thd()->CurrentEnv()/*env*/);
//        Object *rv = N->Get(-1);
//        N->Pop(1);
//        return rv;
        TODO();
        return Object::kNil;
    }
    N->Raisef("attempt to call nil `%s' meta function.", name->bytes());
    return Object::kNil;
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyFloat64:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
template<class T>
static inline Object *ProcessFlot64Arith(const NyFloat64 *lhs, Object *rhs, const char *literal,
                                         T callback,  NyaaCore *N) {
    if (NyString *s = NyString::Cast(rhs)) {
        rhs = s->TryNumeric(N);
    }
    f64_t lval = lhs->value();
    switch (rhs->GetType()) {
        case kTypeSmi:
            if ((literal[0] == '/' || literal[0] == '%') && rhs->ToSmi() == 0) {
                N->Raisef("div zero.");
                return nullptr;
            }
            return N->factory()->NewFloat64(callback(lval, rhs->ToSmi()));
        case kTypeFloat64:
            return N->factory()->NewFloat64(callback(lval, NyFloat64::Cast(rhs)->value()));
        default:
            break;
    }
    N->Raisef("type can not be `%s'.", literal);
    return nullptr;
}

Object *NyFloat64::Add(Object *rhs, NyaaCore *N) const {
    return ProcessFlot64Arith(this, rhs, "+", [](f64_t lval, f64_t rval){ return lval + rval; }, N);
}

Object *NyFloat64::Sub(Object *rhs, NyaaCore *N) const {
    return ProcessFlot64Arith(this, rhs, "-", [](f64_t lval, f64_t rval){ return lval - rval; }, N);
}

Object *NyFloat64::Mul(Object *rhs, NyaaCore *N) const {
    return ProcessFlot64Arith(this, rhs, "*", [](f64_t lval, f64_t rval){ return lval * rval; }, N);
}

Object *NyFloat64::Div(Object *rhs, NyaaCore *N) const {
    return ProcessFlot64Arith(this, rhs, "/", [](f64_t lval, f64_t rval){ return lval / rval; }, N);
}

Object *NyFloat64::Mod(Object *rhs, NyaaCore *N) const {
    return ProcessFlot64Arith(this, rhs, "%",
                              [](f64_t lval, f64_t rval) { return ::fmod(lval, rval); }, N);
}
    
Object *NyFloat64::Shl(Object *rhs, NyaaCore *N) const {
    int n = 0;
    switch (rhs->GetType()) {
        case kTypeSmi:
            n = static_cast<int>(rhs->ToSmi());
            break;
        case kTypeFloat64:
            n = static_cast<int>(NyFloat64::Cast(rhs)->value());
            break;
        default:
            N->Raisef("incorrect type %s attempt shl.", kBuiltinTypeName[GetType()]);
            return Object::kNil;
    }
    return NySmi::New(static_cast<int64_t>(value_) << n);
}

Object *NyFloat64::Shr(Object *rhs, NyaaCore *N) const {
    int n = 0;
    switch (rhs->GetType()) {
        case kTypeSmi:
            n = static_cast<int>(rhs->ToSmi());
            break;
        case kTypeFloat64:
            n = static_cast<int>(NyFloat64::Cast(rhs)->value());
            break;
        default:
            N->Raisef("incorrect type %s attempt shr.", kBuiltinTypeName[GetType()]);
            return Object::kNil;
    }
    return NySmi::New(static_cast<uint64_t>(value_) >> n);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyMap:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
NyMap::NyMap(NyObject *maybe, uint64_t kid, bool linear, NyaaCore *N)
    : generic_(maybe)
    , kid_(kid)
    , linear_(linear) {

    N->BarrierWr(this, &generic_, maybe);
    DCHECK_LE(kid, 0x00ffffffffffffffull);
}
    
uint32_t NyMap::Length() const { return linear_ ? array_->size() : table_->size(); }

NyString *NyMap::ToString(NyaaCore *N) {
    if (GetMetatable() != N->kmt_pool()->kMap) {
        if (NyRunnable *fn = GetValidMetaFunction(N->bkz_pool()->kInnerStr, N)) {
            // TODO: Object *args = this;
            // N->curr_thd()->Run(fn, &args, 1/*nargs*/, 1/*nrets*/);
            // return NyString::Cast(N->curr_thd()->Get(-1));
            TODO();
            return nullptr;
        }
    }

    NyString *buf = N->factory()->NewUninitializedString(64);
    Iterator iter(this);
    buf = buf->Add("{", 1, N);
    bool first = true;
    for (iter.SeekFirst(); iter.Valid(); iter.Next()) {
        if (!first) {
            buf = buf->Add(", ", 2, N);
        }
        first = false;
        buf = buf->Add(iter.key()->ToString(N), N);
        buf = buf->Add(":", 1, N);
        buf = buf->Add(iter.value()->ToString(N), N);
    }
    buf = buf->Add("}", 1, N);

    return buf->Done(N);
}
    
void NyMap::Put(Object *key, Object *value, NyaaCore *N) {
    NyMap *mt = GetMetatable();
    if (mt == N->kmt_pool()->kMap) {
        RawPut(key, value, N);
        return;
    }
    Object *mo = GetMetaFunction(N->bkz_pool()->kInnerNewindex, N);
    if (NyMap *mm = NyMap::Cast(mo)) {
        mm->RawPut(key, value, N);
    } else if (NyRunnable *mf = NyRunnable::Cast(mo)) {
        // TODO: Object *args[] = {this, key, value};
        // N->curr_thd()->Run(mf, args, 3/*nargs*/, 0/*nrets*/);
    } else {
        RawPut(key, value, N);
    }
}

Object *NyMap::Get(Object *key, NyaaCore *N) const {
    NyMap *mt = GetMetatable();
    if (mt == N->kmt_pool()->kMap) {
        return RawGet(key, N);
    }
    
    Object *val = nullptr;
    Object *mo = GetMetaFunction(N->bkz_pool()->kInnerIndex, N);
    if (NyMap *mm = NyMap::Cast(mo)) {
        val = mm->RawGet(key, N);
    } else if (NyRunnable *mf = NyRunnable::Cast(mo)) {
        // Object *args[] = {const_cast<NyMap *>(this), key};
        // N->curr_thd()->Run(mf, args, 2/*nargs*/, 1/*nrets*/);
        // val = N->Get(-1);
        // N->Pop();
    } else {
        val = RawGet(key, N);
    }
    return val;
}
    
void NyMap::RawPut(Object *key, Object *value, NyaaCore *N) {
    NyObject *old = generic_;
    
    if (!linear_) {
        table_ = table_->RawPut(key, value, N);
        if (generic_ != old) {
            N->BarrierWr(this, &generic_, generic_);
        }
        return;
    }
    
    bool should_table = false;
    int64_t index = 0;
    if (key->IsSmi()) {
        index = key->ToSmi();
        if (index > array_->capacity() + 1024) {
            should_table = true;
        }
    } else {
        should_table = linear_;
    }

    if (should_table) {
        HandleScope scope(N->stub());
        Handle<NyTable> table(N->factory()->NewTable(array_->capacity(), rand()));
        for (int64_t i = 0; i < array_->size(); ++i) {
            if (array_->Get(i) == Object::kNil) {
                continue;
            }
            table = table->RawPut(NySmi::New(i), array_->Get(i), N);
        }
        table_ = *table;
        linear_ = false;
    }
    
    if (linear_) {
        array_ = array_->Put(index, value, N);
    } else {
        table_ = table_->RawPut(key, value, N);
    }
    if (generic_ != old) {
        N->BarrierWr(this, &generic_, generic_);
    }
}

Object *NyMap::RawGet(Object *key, NyaaCore *N) const {
    if (linear_) {
        if (key->IsObject()) {
            return nullptr;
        }
        int64_t index = key->ToSmi();
        if (index < 0 || index > array_->size()) {
            return nullptr;
        }
        return array_->Get(index);
    }
    return table_->RawGet(key, N);
}
    
bool NyMap::Equal(Object *rhs, NyaaCore *N) {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerEq, N)->IsTrue();
}

bool NyMap::LessThan(Object *rhs, NyaaCore *N) {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerLt, N)->IsTrue();
}

bool NyMap::LessEqual(Object *rhs, NyaaCore *N) {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerLe, N)->IsTrue();
}
    
Object *NyMap::Add(Object *rhs, NyaaCore *N) {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerAdd, N);
}

Object *NyMap::Sub(Object *rhs, NyaaCore *N) {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerSub, N);
}

Object *NyMap::Mul(Object *rhs, NyaaCore *N) {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerMul, N);
}

Object *NyMap::Div(Object *rhs, NyaaCore *N) {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerDiv, N);
}

Object *NyMap::Mod(Object *rhs, NyaaCore *N) {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerMod, N);
}

Object *NyMap::Minus(NyaaCore *N) {
    return AttemptUnaryMetaFunction(N->bkz_pool()->kInnerUnm, N);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyTable:
////////////////////////////////////////////////////////////////////////////////////////////////////

NyTable::NyTable(uint32_t seed, uint32_t capacity)
    : seed_(seed)
    , size_(0)
    , capacity_(capacity) {
    DCHECK_GT(capacity_, 4);
    ::memset(entries_, 0, sizeof(Entry) * (capacity + 1));
    free_ = capacity_;
}

std::tuple<Object *, Object *> NyTable::GetFirstPair() {
    Entry *slot = entries_ + 1;
    Entry *last = entries_ + 1 + n_slots();
    DCHECK(slot < last);
    
    for (Entry *i = slot; i < last; ++i) {
        if (i->kind == kSlot) {
            return {i->key, i->value};
        }
    }
    return {nullptr, nullptr};
}

std::tuple<Object *, Object *> NyTable::GetNextPair(Object *key, NyaaCore *N) {
    Entry *slot = HashSlot(key, N);
    DCHECK(slot >= entries_ + 1);
    Entry *last = entries_ + 1 + n_slots();
    DCHECK(slot < last);
    switch (slot->kind) {
        case kFree: {
            for (Entry *i = slot + 1; i < last; ++i) {
                if (i->kind == kSlot) {
                    return {i->key, i->value};
                }
            }
        } break;
        case kSlot: {
            Entry *p = slot;
            while (p) {
                if (Object::Equal(key, p->key, N)) {
                    break;
                }
                p = At(p->next);
            }
            if (Entry *next = !p ? nullptr : At(p->next)) {
                return {next->key, next->value};
            }
            //if (!p || !At(p->next))
            for (Entry *i = slot + 1; i < last; ++i) {
                if (i->kind == kSlot) {
                    return {i->key, i->value};
                }
            }
        } break;
        case kNode:
        default:
            NOREACHED();
            break;
    }
    return {nullptr, nullptr};
}

NyTable *NyTable::RawPut(Object *key, Object *value, NyaaCore *N) {
    if (key != nullptr && key->IsNotKey(N)) {
        N->Raisef("Incorrect hash key.");
        return nullptr;
    }

    NyTable *ob = this;
    if (value) {
        if (free_ <= n_slots() + 1) {
            ob = N->factory()->NewTable(capacity_ << 1, seed_, ob);
            if (!ob) {
                return nullptr;
            }
        }
        ob->DoPut(key, value, N);
    } else {
        ob->DoDelete(key, N);
        if (capacity_ > 32 && size_ < (capacity_ >> 2)) { // size < 1/4 capacity_
            ob = N->factory()->NewTable(capacity_ >> 1, seed_, ob);
            if (!ob) {
                return nullptr;
            }
        }
    }
    return ob;
}

Object *NyTable::RawGet(Object *key, NyaaCore *N) const {
    if (key != nullptr && key->IsNotKey(N)) {
        return kNil;
    }

    const Entry *slot = HashSlot(key, N);
    const Entry *p = slot;
    while (p) {
        if (Object::Equal(key, p->key, N)) {
            break;
        }
        p = At(p->next);
    }
    return !p ? kNil : p->value;
}

NyTable *NyTable::Rehash(NyTable *origin, NyaaCore *N) {
    NyTable *ob = this;
    for (size_t i = 1; i < origin->capacity() + 1; ++i) {
        Entry *e = DCHECK_NOTNULL(origin->At(static_cast<int>(i)));
        if (e->kind != kFree) {
            ob = ob->RawPut(e->key, e->value, N);
        }
    }
    return ob;
}
    
bool NyTable::DoDelete(Object *key, NyaaCore *N) {
    Entry *slot = HashSlot(key, N);
    DCHECK(slot >= entries_ + 1);
    DCHECK(slot < entries_ + 1 + n_slots());
    
    switch (slot->kind) {
        case kSlot: {
            Entry dummy;
            dummy.next = Ptr(slot);
            Entry *p = slot, *prev = &dummy;
            while (p) {
                if (Object::Equal(key, p->key, N)) {
                    break;
                }
                prev = p;
                p = At(p->next);
            }
            if (p) {
                prev->next = p->next;
                Free(p);
                if (prev->next) {
                    *slot = *At(prev->next);
                }
                slot->kind = kSlot;
                size_--;
            }
        } break;

        case kFree:
            break;
        case kNode:
        default:
            NOREACHED();
            break;
    }
    return false;
}

bool NyTable::DoPut(Object *key, Object *value, NyaaCore *N) {
    Entry *slot = HashSlot(key, N);
    DCHECK(slot >= entries_ + 1);
    DCHECK(slot < entries_ + 1 + n_slots());
    DCHECK(value);

    switch (slot->kind) {
        case kFree: {
            slot->kind = kSlot;
            N->BarrierWr(this, &slot->key, key);
            slot->key = key;
            N->BarrierWr(this, &slot->value, value);
            slot->value = value;
            size_++;
        } break;
        case kSlot: {
            Entry *p = slot;
            while (p) {
                if (Object::Equal(key, p->key, N)) {
                    N->BarrierWr(this, &p->value, value);
                    p->value = value;
                    return true;
                }
                p = At(p->next);
            }
            p = DCHECK_NOTNULL(Alloc());
            p->kind = kNode;
            p->next = slot->next;
            slot->next = Ptr(p);
            DCHECK_GT(slot->next, 0);
            N->BarrierWr(this, &p->key, key);
            p->key = key;
            N->BarrierWr(this, &p->value, value);
            p->value = value;
            size_++;
        } break;
        case kNode:
        default:
            NOREACHED();
            break;
    }
    return false;
}
    
void NyTable::Iterate(ObjectVisitor *visitor) {
    for (size_t i = 1; i < capacity_ + 1; ++i) {
        Entry *e = entries_ + i;
        if (e->kind != kFree) {
            visitor->VisitPointer(this, &e->key);
            visitor->VisitPointer(this, &e->value);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyString:
////////////////////////////////////////////////////////////////////////////////////////////////////

NyString::NyString(const char *s, size_t n, NyaaCore *N)
    : NyByteArray(static_cast<uint32_t>(n) + kHeaderSize + 1) {
    size_ += kHeaderSize;
    ::memcpy(elems_ + kHeaderSize, s, n);
    size_ += n;
    NyString *done = Done(N);
    (void)done; DCHECK_EQ(done, this);
}
    
NyString *NyString::Done(NyaaCore *N) {
    elems_[size_] = 0;
    
    uint32_t *hash_val = reinterpret_cast<uint32_t *>(data());
    if (size() > kLargeStringLength) {
        *hash_val = -1;
    } else {
        *hash_val = base::Hash::Js(bytes(), size());
    }

    if (size() <= kLargeStringLength && N->kz_pool()) {
        if (NyString *found = N->kz_pool()->GetOrNull(bytes(), size())) {
            return found;
        } else {
            N->kz_pool()->Add(this);
        }
    }
    return this;
}
    
int NyString::Compare(const char *z, size_t n) const {
    const size_t min_len = size() < n ? size() : n;
    int r = ::memcmp(bytes(), z, min_len);
    if (r == 0) {
        if (size() < n) {
            r = -1;
        }
        else if (size() > n) {
            r = +1;
        }
    }
    return r;
}
    
Object *NyString::TryNumeric(NyaaCore *N) const {
    // 0 = not a number
    // 'o' = octal
    // 'd' = decimal
    // 's' = signed decimal
    // 'h' = hexadecimal
    // 'f' = float
    // 'e' = float with exp
    
    int64_t i64 = 0;
    uint64_t u64 = 0;
    int rv = 0;
    switch (base::Slice::LikeNumber(bytes(), size())) {
        case 'o':
            rv = base::Slice::ParseO64(bytes(), size(), &u64);
            DCHECK_GE(rv, 0);
            if (rv > 0 || u64 > std::numeric_limits<int64_t>::max()) {
                // TODO:
            }
            i64 = u64;
            break;
        case 'h':
            rv = base::Slice::ParseH64(bytes(), size(), &u64);
            DCHECK_GE(rv, 0);
            if (rv > 0 || u64 > std::numeric_limits<int64_t>::max()) {
                // TODO:
            }
            i64 = u64;
            break;
        case 'd':
            rv = base::Slice::ParseI64(bytes(), size(), &i64);
            if (rv > 0 || i64 > NySmi::kMaxValue) {
                // TODO:
            }
            break;
        case 's':
            rv = base::Slice::ParseI64(bytes(), size(), &i64);
            if (rv > 0 || (i64 > NySmi::kMaxValue || i64 < NySmi::kMinValue)) {
                // TODO:
            }
            break;
        case 'f':
        case 'e':
            return N->factory()->NewFloat64(::atof(bytes()));
        case 0:
        default:
            return const_cast<NyString *>(this);
    }
    return NySmi::New(i64);
}
    
int64_t NyString::TryI64(bool *ok) const {
    int64_t i64 = 0;
    uint64_t u64 = 0;
    int rv = 0;
    *ok = true;
    switch (base::Slice::LikeNumber(bytes(), size())) {
        case 'o':
            if (size() > 22) {
                rv = base::Slice::ParseO64(bytes() + size() - 22, 22, &u64);
            } else {
                rv = base::Slice::ParseO64(bytes(), size(), &u64);
            }
            DCHECK_GE(rv, 0);
            return static_cast<int64_t>(u64);
        case 'h':
            if (size() > 16) {
                rv = base::Slice::ParseH64(bytes() + size() - 16, 16, &u64);
            } else {
                rv = base::Slice::ParseH64(bytes(), size(), &u64);
            }
            DCHECK_GE(rv, 0);
            return static_cast<int64_t>(u64);
        case 'd':
            if (size() > 19) {
                rv = base::Slice::ParseI64(bytes() + size() - 19, 19, &i64);
            } else {
                rv = base::Slice::ParseI64(bytes(), size(), &i64);
            }
            return i64;
        case 's':
            if (size() > 20) {
                rv = base::Slice::ParseI64(bytes() + size() - 20, 20, &i64);
                DCHECK_GE(i64, 0);
                i64 = -i64;
            } else {
                rv = base::Slice::ParseI64(bytes(), size(), &i64);
            }
            return i64;
        case 'f':
        case 'e':
            return static_cast<int64_t>(::atof(bytes()));
        case 0:
        default:
            *ok = false;
            break;
    }
    return 0;
}

f64_t NyString::TryF64(bool *ok) const {
    int64_t i64 = 0;
    uint64_t u64 = 0;
    int rv = 0;
    *ok = true;
    switch (base::Slice::LikeNumber(bytes(), size())) {
        case 'o':
            if (size() > 22) {
                rv = base::Slice::ParseO64(bytes() + size() - 22, 22, &u64);
            } else {
                rv = base::Slice::ParseO64(bytes(), size(), &u64);
            }
            DCHECK_GE(rv, 0);
            return static_cast<f64_t>(u64);
        case 'h':
            if (size() > 16) {
                rv = base::Slice::ParseH64(bytes() + size() - 16, 16, &u64);
            } else {
                rv = base::Slice::ParseH64(bytes(), size(), &u64);
            }
            DCHECK_GE(rv, 0);
            return static_cast<f64_t>(u64);
        case 'd':
            if (size() > 19) {
                rv = base::Slice::ParseI64(bytes() + size() - 19, 19, &i64);
            } else {
                rv = base::Slice::ParseI64(bytes(), size(), &i64);
            }
            return static_cast<f64_t>(i64);
        case 's':
            if (size() > 20) {
                rv = base::Slice::ParseI64(bytes() + size() - 20, 20, &i64);
                DCHECK_GE(i64, 0);
                i64 = -i64;
            } else {
                rv = base::Slice::ParseI64(bytes(), size(), &i64);
            }
            return static_cast<f64_t>(i64);
        case 'f':
        case 'e':
            return ::atof(bytes());
        case 0:
        default:
            *ok = false;
            break;
    }
    return std::numeric_limits<f64_t>::quiet_NaN();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyByteArray:
////////////////////////////////////////////////////////////////////////////////////////////////////

NyByteArray *NyByteArray::Add(Byte value, NyaaCore *N) {
    NyByteArray *ob = this;
    if (size() + 1 > capacity()) {
        ob = N->factory()->NewByteArray(capacity_ << 1, this);
    }
    ob->elems_[size_++] = value;
    return ob;
}

NyByteArray *NyByteArray::Add(const void *value, size_t n, NyaaCore *N) {
    NyByteArray *ob = this;
    const uint32_t k = static_cast<uint32_t>(n);
    if (size() + k > capacity()) {
        ob = N->factory()->NewByteArray((capacity_ << 1) + k, this);
    }
    ::memcpy(elems_ + size(), value, n);
    size_ += k;
    return ob;
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyInt32Array:
////////////////////////////////////////////////////////////////////////////////////////////////////

NyInt32Array *NyInt32Array::Add(int32_t value, NyaaCore *N) {
    NyInt32Array *ob = this;
    if (size() + 1 > capacity()) {
        ob = N->factory()->NewInt32Array(capacity_ << 1, this);
    }
    ob->elems_[size_++] = value;
    return ob;
}
    
NyInt32Array *NyInt32Array::Add(int32_t *value, size_t n, NyaaCore *N) {
    NyInt32Array *ob = this;
    const uint32_t k = static_cast<uint32_t>(n);
    if (size() + k > capacity()) {
        ob = N->factory()->NewInt32Array((capacity_ << 1) + k, this);
    }
    ::memcpy(elems_ + size(), value, n * sizeof(uint32_t));
    size_ += k;
    return ob;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyArray:
////////////////////////////////////////////////////////////////////////////////////////////////////

NyArray *NyArray::Put(int64_t key, Object *value, NyaaCore *N) {
    NyArray *ob = this;
    if (key > capacity()) {
        uint32_t new_cap = static_cast<uint32_t>(key + (key - capacity()) * 2);
        ob = N->factory()->NewArray(new_cap, this);
    }

    N->BarrierWr(this, ob->elems_ + key, value);
    ob->elems_[key] = value;
    
    if (key > size()) {
        size_ = static_cast<uint32_t>(key) + 1;
    }
    return ob;
}

NyArray *NyArray::Add(Object *value, NyaaCore *N) {
    NyArray *ob = this;
    if (size() + 1 > capacity()) {
        ob = N->factory()->NewArray(capacity_ << 1, this);
    }

    N->BarrierWr(this, ob->elems_ + size_, value);
    ob->elems_[size_++] = value;
    
    return ob;
}
    
void NyArray::Refill(const NyArray *base, NyaaCore *N) {
    for (int64_t i = 0; i < base->size(); ++i) {
        N->BarrierWr(this, elems_ + i, base->Get(i));
    }
    NyArrayBase<Object*>::Refill(base);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyRunnable:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
int NyRunnable::Run(Object *argv[], int argc, int nrets, NyaaCore *N) {
    // TODO: return N->curr_thd()->TryRun(this, argv, argc, nrets);
    TODO();
    return -1;
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyDelegated:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
void NyDelegated::Bind(int i, Object *upval, NyaaCore *N) {
    DCHECK_GE(i, 0);
    DCHECK_LT(i, n_upvals_);
    N->BarrierWr(this, upvals_ + i, upval);
    upvals_[i] = upval;
}
    
int NyDelegated::Call(Object *argv[], int argc, int wanted, NyaaCore *N) {
    // TODO: return N->curr_thd()->TryRun(this, argv, argc, wanted);
    TODO();
    return -1;
}
    
int NyDelegated::Apply(const FunctionCallbackInfo<Object> &info) {
    switch (kind()) {
        case kFunctionCallback:
            fn_fp_(info);
            break;
        default:
            DLOG(FATAL) << "TODO:";
            break;
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyUDO:
////////////////////////////////////////////////////////////////////////////////////////////////////

void NyUDO::SetFinalizer(Finalizer fp, NyaaCore *N) {
    N->heap()->AddFinalizer(this, fp);
}
    
NyString *NyUDO::ToString(NyaaCore *N) {
    if (NyRunnable *fn = GetValidMetaFunction(N->bkz_pool()->kInnerStr, N)) {
        // TODO: Object *args = this;
        // N->curr_thd()->Run(fn, &args, 1/*nargs*/, 1/*nrets*/);
        // return NyString::Cast(N->curr_thd()->Get(-1));
        TODO();
        return nullptr;
    }
    return N->factory()->Sprintf("udo: %p", this);
}
    
void NyUDO::Put(Object *key, Object *value, NyaaCore *N) {
    if (NyRunnable *mf = GetValidMetaFunction(N->bkz_pool()->kInnerNewindex, N)) {
        // TODO: Object *args[] = {this, key, value};
        // N->curr_thd()->Run(mf, args, arraysize(args)/*argc*/, 0/*wanted*/, nullptr/*TODO: env*/);
        TODO();
    } else {
        RawPut(key, value, N);
    }
}

Object *NyUDO::Get(Object *key, NyaaCore *N) {
    Object *val = nullptr;
    if (NyRunnable *mf = GetValidMetaFunction(N->bkz_pool()->kInnerIndex, N)) {
        // TODO: Object *args[] = {this, key};
        // N->curr_thd()->Run(mf, args, arraysize(args)/*argc*/, 1/*wanted*/, nullptr/*TODO: env*/);
        // val = N->Get(-1);
        // N->Pop();
    } else {
        val = RawGet(key, N);
    }
    return val;
}
    
Object *NyUDO::RawGet(Object *key, NyaaCore *N) {
    DCHECK_NOTNULL(key);
    bool ignore_access_check = false;
    if (NyString *name = NyString::Cast(key)) {
        if (name->bytes()[name->size() - 1] == '_') {
            ignore_access_check = true;
            key = N->factory()->NewString(name->bytes(), name->size() - 1, false);
        }
    }

    Object *mfield = GetMetatable()->RawGet(key, N);
    if (mfield == Object::kNil) {
        return Object::kNil;
    }
    if (mfield->IsSmi()) {
        if (!ignore_access_check) {
            int64_t access = mfield->ToSmi() & 0x3;
            if (!(access & 0x1)) {
                N->Raisef("udo access fail: read");
                return Object::kNil;
            }
        }
        return GetField(mfield->ToSmi() >> 2, N);
    }
    NyObject *ob = mfield->ToHeapObject();
    return ob->ToRunnable();
}

void NyUDO::RawPut(Object *key, Object *value, NyaaCore *N) {
    DCHECK_NOTNULL(key);
    bool ignore_access_check = false;
    if (NyString *name = NyString::Cast(key)) {
        if (name->bytes()[name->size() - 1] == '_') {
            ignore_access_check = true;
            key = N->factory()->NewString(name->bytes(), name->size() - 1, false);
        }
    }
    
    Object *mfield = GetMetatable()->RawGet(key, N);
    if (mfield == Object::kNil) {
        return;
    }
    if (mfield->IsSmi()) {
        if (!ignore_access_check) {
            int64_t access = mfield->ToSmi() & 0x3;
            if (!(access & 0x2)) {
                N->Raisef("udo access fail: write");
                return;
            }
        }
        SetField(mfield->ToSmi() >> 2, value, N);
    }
}
    
Object *NyUDO::GetField(size_t i, NyaaCore *N) {
#if defined(DEBUG) || defined(_DEBUG)
    Object *msize = GetMetatable()->RawGet(N->bkz_pool()->kInnerSize, N);
    DCHECK(DCHECK_NOTNULL(msize)->IsSmi());
    DCHECK_EQ(msize->ToSmi(), n_fields());
#endif
    DCHECK_LT(i, n_fields());
    return fields_[i];
}
    
void NyUDO::SetField(size_t i, Object *value, NyaaCore *N) {
#if defined(DEBUG) || defined(_DEBUG)
    Object *msize = GetMetatable()->RawGet(N->bkz_pool()->kInnerSize, N);
    DCHECK(DCHECK_NOTNULL(msize)->IsSmi());
    DCHECK_EQ(msize->ToSmi(), n_fields());
#endif
    DCHECK_LT(i, n_fields());
    N->BarrierWr(this, fields_ + i, value);
    fields_[i] = value;
}
    
bool NyUDO::Equal(Object *rhs, NyaaCore *N) {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerEq, N)->IsTrue();
}

bool NyUDO::LessThan(Object *rhs, NyaaCore *N) {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerLt, N)->IsTrue();
}

bool NyUDO::LessEqual(Object *rhs, NyaaCore *N) {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerLe, N)->IsTrue();
}
    
Object *NyUDO::Add(Object *rhs, NyaaCore *N) {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerAdd, N);
}

Object *NyUDO::Sub(Object *rhs, NyaaCore *N) {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerSub, N);
}

Object *NyUDO::Mul(Object *rhs, NyaaCore *N) {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerMul, N);
}

Object *NyUDO::Div(Object *rhs, NyaaCore *N) {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerDiv, N);
}

Object *NyUDO::Mod(Object *rhs, NyaaCore *N) {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerMod, N);
}
    
Object *NyUDO::Minus(NyaaCore *N) {
    return AttemptUnaryMetaFunction(N->bkz_pool()->kInnerUnm, N);
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// Others:
////////////////////////////////////////////////////////////////////////////////////////////////////

#define DEFINE_TYPE_CHECK(type, name) \
    bool Ny##type::EnsureIs(const NyObject *o, NyaaCore *N) { \
    if (!o->Is##type ()) { \
        N->Raisef("unexpected type: " name "."); \
            return false; \
        } \
        return true; \
    }
    
DEFINE_TYPE_CHECK(Float64, "float")
DEFINE_TYPE_CHECK(String, "string")
DEFINE_TYPE_CHECK(Delegated, "delegated")
DEFINE_TYPE_CHECK(Map, "map")
DEFINE_TYPE_CHECK(Table, "table")
DEFINE_TYPE_CHECK(ByteArray, "array[byte]")
DEFINE_TYPE_CHECK(Int32Array, "array[int32]")
DEFINE_TYPE_CHECK(Array, "array")
DEFINE_TYPE_CHECK(Closure, "closure")
DEFINE_TYPE_CHECK(Function, "function")
// TODO: DEFINE_TYPE_CHECK(Thread, "thread")

void MapIterator::SeekFirst() {
    if (table_) {
        int64_t i;
        for (i = 0; i < table_->capacity(); ++i) {
            if (table_->entries_[i + 1].kind != NyTable::kFree) {
                break;
            }
        }
        index_ = i;
    } else {
        int64_t i;
        for (i = 0; i < array_->capacity(); ++i) {
            if (array_->Get(i)) {
                break;
            }
        }
        index_ = i;
    }
}
    
void MapIterator::Next() {
    if (table_) {
        int64_t i;
        for (i = index_ + 1; i < table_->capacity(); ++i) {
            if (table_->entries_[i + 1].kind != NyTable::kFree) {
                break;
            }
        }
        index_ = i;
    } else {
        int64_t i;
        for (i = index_ + 1; i < array_->size(); ++i) {
            if (array_->Get(i)) {
                break;
            }
        }
        index_ = i;
    }
}

} // namespace nyaa
    
} // namespace mai
