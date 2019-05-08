#include "nyaa/nyaa-values.h"
#include "nyaa/thread.h"
#include "nyaa/parser.h"
#include "nyaa/code-gen.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/heap.h"
#include "nyaa/object-factory.h"
#include "nyaa/string-pool.h"
#include "nyaa/memory.h"
#include "nyaa/builtin.h"
#include "base/arena-utils.h"
#include "base/arenas.h"
#include "base/hash.h"
#include "base/big-number.h"
#include "mai-lang/call-info.h"
#include "mai-lang/isolate.h"
#include "mai/env.h"
#include <limits>

namespace mai {
    
namespace nyaa {
    
using big = base::Big;
    
static const char kRadixDigitals[] = "0123456789abcdef";

/*static*/ Object *const Object::kNil = nullptr;
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// class Object:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
bool Object::IsKey(NyaaCore *N) const {
    switch (GetType()) {
        case kTypeSmi:
        case kTypeString:
        case kTypeFloat64:
        case kTypeInt:
            return true;
        default:
            break;
    }
    return false;
}

uint32_t Object::HashVal(NyaaCore *N) const {
    switch (GetType()) {
        case kTypeSmi:
            return static_cast<uint32_t>(ToSmi());
        case kTypeString:
            return NyString::Cast(this)->hash_val();
        case kTypeFloat64:
            return NyFloat64::Cast(this)->HashVal();
        case kTypeInt:
            return NyInt::Cast(this)->HashVal();
        default:
            break;
    }
    DLOG(FATAL) << "Noreached!";
    return 0;
}
    
/*static*/ bool Object::Equal(Object *lhs, Object *rhs, NyaaCore *N) {
    if (lhs == kNil || rhs == kNil) {
        return false;
    }
    if (lhs == rhs) {
        return true;
    }
    if (lhs->IsSmi()) {
        if (rhs->IsSmi()) {
            return lhs->ToSmi() == rhs->ToSmi();
        } else {
            return rhs->ToHeapObject()->Equal(lhs, N);
        }
    }
    return lhs->ToHeapObject()->Equal(rhs, N);
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
            return false;
        case kTypeSmi:
            return ToSmi() == 0;
        case kTypeString:
            return NyString::Cast(this)->size() == 0;
        case kTypeInt:
            return NyInt::Cast(this)->IsZero();
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
    
#undef DEFINE_OBJECT_BIN_ARITH

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
            base::ScopedArena scoped_buf;
            NyInt *ll = NyInt::NewI64(lval, &scoped_buf);
            return ll->Add(rval, N);
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
                base::ScopedArena scoped_buf;
                NyInt *ll = NyInt::NewI64(lval, &scoped_buf);
                return ll->Sub(rval, N);
            }
            return New(lval - rval);
        } break;
            
        case kTypeInt: {
            base::ScopedArena scoped_buf;
            NyInt *ll = NyInt::NewI64(lval, &scoped_buf);
            return ll->Add(NyInt::Cast(rhs), N);
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
            base::ScopedArena scoped_buf;
            NyInt *ll = NyInt::NewI64(lval, &scoped_buf);
            return ll->Mul(rval, N);
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
            
        case kTypeInt: {
            base::ScopedArena scoped_buf;
            NyInt *ll = NyInt::NewI64(lval, &scoped_buf);
            return ll->Div(NyInt::Cast(rhs), N);
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
        case kTypeInt: {
            base::ScopedArena scoped_buf;
            NyInt *ll = NyInt::NewI64(lval, &scoped_buf);
            return ll->Mod(NyInt::Cast(rhs), N);
        } break;
        case kTypeFloat64:
            return N->factory()->NewFloat64(::fmod(lval, NyFloat64::Cast(rhs)->value()));
        default:
            break;
    }
    N->Raisef("smi attempt to call nil `__mod__' meta function.");
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyObject:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
void NyObject::SetMetatable(NyMap *mt, NyaaCore *N) {
    if (mt == Object::kNil) {
        return; // ignore
    }
    N->heap()->BarrierWr(this, reinterpret_cast<Object **>(&mtword_), mt, true /*ismt*/);
#if defined(NYAA_USE_POINTER_COLOR)
    uintptr_t bits = reinterpret_cast<uintptr_t>(mt);
    DCHECK_EQ(bits & kColorMask, 0);
    mtword_ = (bits | (mtword_ & kColorMask));
#else
    mtword_ = reinterpret_cast<uintptr_t>(mt);
#endif
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
            return lval->Compare(rval) == 0;
        } break;
            
        case kTypeInt: {
            auto lval = ToInt();
            if (rhs->IsSmi()) {
                return lval->ToI64() == rhs->ToSmi();
            } else if (NyInt *rval = NyInt::Cast(rhs)) {
                return NyInt::Compare(lval, rval) == 0;
            } else if (NyFloat64 *rval = NyFloat64::Cast(rhs)) {
                return lval->ToF64() == rval->value();
            } else {
                return false;
            }
        } break;
            
        case kTypeFloat64: {
            auto lval = ToFloat64()->value();
            if (rhs->IsSmi()) {
                return lval == rhs->ToSmi();
            } else if (NyInt *rval = NyInt::Cast(rhs)) {
                return lval == rval->ToF64();
            } else if (NyFloat64 *rval = NyFloat64::Cast(rhs)) {
                return lval == rval->value();
            } else {
                return false;
            }
        } break;
            
        case kTypeMap:
            return ToMap()->Equal(rhs, N);
            
        case kTypeUdo:
            return ToUDO()->Equal(rhs, N);
  
        default:
//            if (GetMetatable()->kid() > kUdoKidBegin) {
//                return ToUDO()->Equal(rhs, N);
//            }
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
            
        case kTypeInt: {
            auto lval = ToInt();
            if (rhs->IsSmi()) {
                return lval->ToI64() < rhs->ToSmi();
            } else if (NyInt *rval = NyInt::Cast(rhs)) {
                return NyInt::Compare(lval, rval) < 0;
            } else if (NyFloat64 *rval = NyFloat64::Cast(rhs)) {
                return lval->ToF64() < rval->value();
            } else {
                return false;
            }
        } break;
            
        case kTypeFloat64: {
            auto lval = ToFloat64()->value();
            if (rhs->IsSmi()) {
                return lval < rhs->ToSmi();
            } else if (NyInt *rval = NyInt::Cast(rhs)) {
                return lval < rval->ToF64();
            } else if (NyFloat64 *rval = NyFloat64::Cast(rhs)) {
                return lval < rval->value();
            } else {
                return false;
            }
        } break;
            
        case kTypeMap:
            return ToMap()->LessThan(rhs, N);
            
        case kTypeUdo:
            return ToUDO()->LessThan(rhs, N);
            
        default:
//            if (GetMetatable()->kid() > kUdoKidBegin) {
//                return ToUDO()->LessThan(rhs, N);
//            }
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
            
        case kTypeInt: {
            auto lval = ToInt();
            if (rhs->IsSmi()) {
                return lval->ToI64() <= rhs->ToSmi();
            } else if (NyInt *rval = NyInt::Cast(rhs)) {
                return NyInt::Compare(lval, rval) <= 0;
            } else if (NyFloat64 *rval = NyFloat64::Cast(rhs)) {
                return lval->ToF64() <= rval->value();
            } else {
                return false;
            }
        } break;
            
        case kTypeFloat64: {
            auto lval = ToFloat64()->value();
            if (rhs->IsSmi()) {
                return lval <= rhs->ToSmi();
            } else if (NyInt *rval = NyInt::Cast(rhs)) {
                return lval <= rval->ToF64();
            } else if (NyFloat64 *rval = NyFloat64::Cast(rhs)) {
                return lval <= rval->value();
            } else {
                return false;
            }
        } break;
            
        case kTypeMap:
            return ToMap()->LessEqual(rhs, N);
            
        case kTypeUdo:
            return ToUDO()->LessEqual(rhs, N);
            
        default:
//            if (GetMetatable()->kid() > kUdoKidBegin) {
//                return ToUDO()->LessEqual(rhs, N);
//            }
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
            
        DECL_BUILTIN_TYPES(DEFINE_ITERATE)
    #undef DEFINE_PLACED_SIZE
        case kTypeUdo:
            static_cast<NyUDO *>(this)->Iterate(visitor);
            break;
        default:
            DLOG(FATAL) << "Noreached";
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

        DECL_BUILTIN_TYPES(DEFINE_PLACED_SIZE)
#undef DEFINE_PLACED_SIZE
        case kTypeUdo:
            static_cast<const NyUDO *>(this)->PlacedSize();
            break;
        default:
            DLOG(FATAL) << "Noreached";
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
            case kTypeInt: \
                return ToInt()->name(rhs, N); \
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

NyString *NyObject::ToString(NyaaCore *N) {
    //HandleScope scope(N->stub());

    switch (GetType()) {
        case kTypeString:
            return (const_cast<NyObject *>(this))->ToString();
        case kTypeInt:
            return ToInt()->ToString(N);
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
        default:
//            if (GetMetatable()->kid() > kUdoKidBegin) {
//                return ToUDO()->ToString(N);
//            }
            break;
    }
    DLOG(FATAL) << "Noreached!" << GetMetatable()->kid();
    return nullptr;
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
        case kTypeInt:
            return N->factory()->NewFloat64(callback(lval, NyInt::Cast(rhs)->ToF64()));
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

////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyInt:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
NyInt::NyInt(uint32_t max_len)
    : capacity_(max_len)
    , offset_(0)
    , header_(0) {
    DbgFillInitZag(vals_, capacity_);
}
    
bool NyInt::IsZero() const {
    if (segments_size() == 0) {
        return true;
    }
    for (size_t i = 0; i < segments_size(); ++i) {
        if (segment(i) != 0) {
            return false;
        }
    }
    return true;
}

NyInt *NyInt::Shl(int n, NyaaCore *N) {
    /*
     * If there is enough storage space in this MutableBigInteger already
     * the available space will be used. Space to the right of the used
     * ints in the value array is faster to utilize, so the extra space
     * will be taken from the right if possible.
     */
    if (segments_size() == 0) {
        return this;
    }
    int n_ints = n >> 5; // n / 32
    int n_bits = n & 0x1f;
    int hi_word_bits = 32 - base::Bits::CountLeadingZeros32(segment(0));
    
    // If shift can be done without moving words, do so
    if (n <= 32 - hi_word_bits) {
        big::PrimitiveShl(segment_mut_view(), n);
        return this;
    }
    
    size_t new_len = segments_size() + n_ints + 1;
    if (n_bits <= 32 - hi_word_bits) {
        new_len--;
    }
    
    NyInt *rv = this;
    if (capacity_ < new_len) {
        // The array must grow
        rv = N->factory()->NewUninitializedInt(new_len);
        for (size_t i = 0; i < segments_size(); ++i) {
            rv->set_segment(i, segment(i));
        }
        for (size_t i = segments_size(); i < new_len; ++i) {
            rv->set_segment(i, 0);
        }
    } else if (segments_size() >= new_len) {
        uint32_t new_off  = static_cast<uint32_t>(capacity_ - new_len);
        ::memmove(vals_ + new_off, vals_ + offset_,
                  segments_size() * sizeof(uint32_t));
        offset_ = new_off;
    } else {
        uint32_t new_off  = static_cast<uint32_t>(capacity_ - new_len);
        ::memmove(vals_ + new_off, vals_ + offset_,
                  segments_size() * sizeof(uint32_t));
        offset_ = new_off;
        for (size_t i = segments_size(); i < new_len; ++i) {
            set_segment(i, 0);
        }
    }
    
    if (n_bits == 0) {
        return rv;
    }
    if (n_bits <= 32 - hi_word_bits) {
        big::PrimitiveShl(rv->segment_mut_view(), n_bits);
    } else {
        big::PrimitiveShr(rv->segment_mut_view(), 32 - n_bits);
    }
    return rv;
}

NyInt *NyInt::Shr(int n, NyaaCore *N) {
    if (segments_size() == 0) {
        return this;
    }
    int n_ints = n >> 5; // n / 32
    int n_bits = n & 0x1f;
    if (n_ints > 0) {
        DCHECK_GE(segments_size(), n_ints);
        ::memmove(segments() + n_ints, segments(),
                  (segments_size() - n_ints) * sizeof(uint32_t));
        offset_ += n_ints;
    }
    if (n_bits == 0) {
        return this;
    }
    int hi_word_bits = 32 - base::Bits::CountLeadingZeros32(segment(0));
    if (n_bits >= hi_word_bits) {
        big::PrimitiveShl(segment_mut_view(), 32 - n_bits);
        ::memmove(segments() + 1, segments(),
                  (segments_size() - 1) * sizeof(uint32_t));
        offset_++;
    } else {
        big::PrimitiveShr(segment_mut_view(), n_bits);
    }
    return this;
}

Object *NyInt::Add(Object *rhs, NyaaCore *N) const {
    switch (rhs->GetType()) {
        case kTypeSmi:
            return UnboxIfNeed(Add(rhs->ToSmi(), N));
        case kTypeInt:
            return UnboxIfNeed(Add(NyInt::Cast(rhs), N));
        case kTypeFloat64:
            return N->factory()->NewFloat64(ToF64() + NyFloat64::Cast(rhs)->value());
        default:
            break;
    }
    N->Raisef("type can not `+'.");
    return nullptr;
}

Object *NyInt::Sub(Object *rhs, NyaaCore *N) const {
    switch (rhs->GetType()) {
        case kTypeSmi:
            return UnboxIfNeed(Sub(rhs->ToSmi(), N));
        case kTypeInt:
            return UnboxIfNeed(Sub(NyInt::Cast(rhs), N));
        case kTypeFloat64:
            return N->factory()->NewFloat64(ToF64() - NyFloat64::Cast(rhs)->value());
        default:
            break;
    }
    N->Raisef("type can not `-'.");
    return nullptr;
}
    
Object *NyInt::Mul(Object *rhs, NyaaCore *N) const {
    switch (rhs->GetType()) {
        case kTypeSmi:
            return UnboxIfNeed(Mul(rhs->ToSmi(), N));
        case kTypeInt:
            return UnboxIfNeed(Mul(NyInt::Cast(rhs), N));
        case kTypeFloat64:
            return N->factory()->NewFloat64(ToF64() * NyFloat64::Cast(rhs)->value());
        default:
            break;
    }
    N->Raisef("type can not `*'.");
    return nullptr;
}


Object *NyInt::Div(Object *rhs, NyaaCore *N) const {
    switch (rhs->GetType()) {
        case kTypeSmi:
            if (rhs->ToSmi() == 0) {
                N->Raisef("div zero.");
                return nullptr;
            }
            return UnboxIfNeed(Div(rhs->ToSmi(), N));
        case kTypeInt:
            return UnboxIfNeed(Div(NyInt::Cast(rhs), N));
        case kTypeFloat64:
            return N->factory()->NewFloat64(ToF64())->Div(rhs, N);
        default:
            break;
    }
    N->Raisef("type can not `/'.");
    return nullptr;
}

Object *NyInt::Mod(Object *rhs, NyaaCore *N) const {
    switch (rhs->GetType()) {
        case kTypeSmi:
            if (rhs->ToSmi() == 0) {
                N->Raisef("div zero.");
                return nullptr;
            }
            return UnboxIfNeed(Mod(rhs->ToSmi(), N));
        case kTypeInt:
            return UnboxIfNeed(Mod(NyInt::Cast(rhs), N));
        case kTypeFloat64:
            return N->factory()->NewFloat64(ToF64())->Mod(rhs, N);
        default:
            break;
    }
    N->Raisef("type can not `%'.");
    return nullptr;
}

NyInt *NyInt::Add(int64_t rval, NyaaCore *N) const {
    base::ScopedArena scoped_buf;
    return Add(NewI64(rval, &scoped_buf), N);
}

NyInt *NyInt::Sub(int64_t rval, NyaaCore *N) const {
    base::ScopedArena scoped_buf;
    return Sub(NewI64(rval, &scoped_buf), N);
}

NyInt *NyInt::Mul(int64_t rval, NyaaCore *N) const {
    base::ScopedArena scoped_buf;
    return Mul(NewI64(rval, &scoped_buf), N);
}

NyInt *NyInt::Div(int64_t rval, NyaaCore *N) const {
    base::ScopedArena scoped_buf;
    return Div(NewI64(rval, &scoped_buf), N);
}

NyInt *NyInt::Mod(int64_t rval, NyaaCore *N) const {
    base::ScopedArena scoped_buf;
    return Mod(NewI64(rval, &scoped_buf), N);
}

NyInt *NyInt::Add(const NyInt *rhs, NyaaCore *N) const {
    size_t n_ints = std::max(segments_size(), rhs->segments_size()) + 1;
    NyInt *rv = N->factory()->NewUninitializedInt(n_ints);
    rv->set_segment(0, 0);
    AddRaw(this, rhs, rv);
    return rv;
}

NyInt *NyInt::Sub(const NyInt *rhs, NyaaCore *N) const {
    size_t n_ints = std::max(segments_size(), rhs->segments_size()) + 1;
    NyInt *rv = N->factory()->NewUninitializedInt(n_ints);
    rv->set_segment(0, 0);
    bool neg = negative();
    if (negative() != rhs->negative()) {
        // -lhs -   rhs  = -(lhs + rhs)
        //  lhs - (-rhs) =   lhs + rhs
        big::Add(segment_view(), rhs->segment_view(), rv->segment_mut_view());
    } else {
        //   lhs  -   rhs  = lhs - rhs = -(rhs - lhs)
        // (-lhs) - (-rhs) = rhs - lhs = -(lhs - rhs)
        if (AbsCompare(this, rhs) >= 0) {
            big::Sub(segment_view(), rhs->segment_view(), rv->segment_mut_view());
        } else {
            neg = !neg;
            big::Sub(rhs->segment_view(), segment_view(), rv->segment_mut_view());
        }
    }
    rv->set_negative(neg);
    rv->Normalize();
    return rv;
}

NyInt *NyInt::Mul(const NyInt *rhs, NyaaCore *N) const {
    NyInt *rv = N->factory()->NewUninitializedInt(segments_size() + rhs->segments_size());
    rv->Fill();
    big::BasicMul(segment_view(), rhs->segment_view(), rv->segment_mut_view());
    rv->Normalize();
    rv->set_negative(negative() != rhs->negative());
    return rv;
}

std::tuple<NyInt *, NyInt *> NyInt::CompleteDiv(const NyInt *rhs, NyaaCore *N) const {
    if (rhs->IsZero()) {
        return {nullptr, nullptr};
    }
    
    if (IsZero()) {
        return {NewI64(0, N->factory()), NewI64(0, N->factory())};
    }
    
    int cmp = AbsCompare(this, rhs);
    if (cmp < 0) {
        return {NewI64(0, N->factory()), Clone(N->factory())};
    }
    
    if (cmp == 0) {
        return (negative() == rhs->negative())
        ? std::make_tuple(NewI64( 1, N->factory()), NewI64(0, N->factory()))
        : std::make_tuple(NewI64(-1, N->factory()), NewI64(0, N->factory()));
    }
    const NyInt *lhs = this;
    NyInt *rv = nullptr, *re = nullptr;
    std::tie(rv, re) = DivRaw(lhs, rhs, N);
    
    rv->Normalize();
    rv->set_negative(negative() != negative());
    re->Normalize();
    re->set_negative(rv->negative());
    return {rv, re};
}

uint32_t NyInt::HashVal() const {
    // TODO:
    DLOG(FATAL) << "TODO:";
    return 0;
}

f64_t NyInt::ToF64() const {
    if (IsZero()) {
        return 0;
    }

    base::ScopedArena scoped_buf;
    NyInt *q = NewUninitialized(capacity_, &scoped_buf);
    q->offset_ = offset_;
    q->header_ = header_;
    
    NyInt *p = Clone(&scoped_buf);
    
    double exp = 1.0;
    double rv = 0;
    int i = 0;
    while (!p->IsZero()) {
        uint32_t m = big::DivWord(p->segment_view(), 10, q->segment_mut_view());
        DCHECK_LT(m, 10);
        rv += exp * m;
        ::memcpy(p->segments(), q->segments(), segments_size() * sizeof(uint32_t));
        ++i;
        exp *= 10;
    }
    return rv * sign();
}

int64_t NyInt::ToI64() const {
    int64_t val = 0;
    if (IsZero()) {
        val = 0;
    } else if (segments_size() < 2) {
        val = static_cast<uint64_t>(segment(0));
    } else { // (d->segments_size() >= 2)
        val |= static_cast<uint64_t>(segment(1) & 0x7fffffff) << 32;
        val |= static_cast<uint64_t>(segment(0));
    }
    return val * sign();
}
    
NyString *NyInt::ToString(NyaaCore *N, int radix) const {
    DCHECK_GE(radix, kMinRadix);
    DCHECK_LE(radix, kMaxRadix);
    if (IsZero()) {
        return N->factory()->NewString("0");
    }

    base::ScopedArena scoped_buf;
    NyInt *q = NewUninitialized(capacity_, &scoped_buf);
    q->offset_ = offset_;
    q->header_ = header_;
    
    NyInt *p = Clone(&scoped_buf);
    std::string buf;
    while (!p->IsZero()) {
        uint32_t m = big::DivWord(p->segment_view(), radix, q->segment_mut_view());
        DCHECK_LT(m, radix);
        buf.insert(buf.begin(), kRadixDigitals[m]);
        ::memcpy(p->segments(), q->segments(),
                 segments_size() * sizeof(uint32_t));
    }

    if (negative()) {
        buf.insert(buf.begin(), '-');
    }
    return N->factory()->NewString(buf.data(), buf.size());
}
    
/*static*/ int NyInt::Compare(const NyInt *lhs, const NyInt *rhs) {
    if (lhs->negative() == rhs->negative()) {
        int rv = AbsCompare(lhs, rhs);
        return lhs->negative() ? -rv : rv;
    }
    return lhs->negative() ? -1 : 1;
}
    
void NyInt::InitP64(uint64_t val, bool neg, size_t reserved) {
    uint32_t hi_bits = static_cast<uint32_t>((val & 0xffffffff00000000ull) >> 32);
    if (hi_bits) {
        Resize(2);
        segments()[0] = hi_bits;
        segments()[1] = static_cast<uint32_t>(val);
    } else {
        if (val) {
            Resize(1);
            segments()[0] = static_cast<uint32_t>(val);
        } else {
            Resize(0);
        }
    }
    set_negative(neg);
}
    
/*static*/ NyInt *NyInt::Parse(const char *s, size_t n, ObjectFactory *factory) {
    int rv = base::Slice::LikeNumber(s, n);
    switch (rv) {
        case 'o':
            return ParseOctLiteral(s, n, factory);
        case 'd':
        case 's':
            return ParseDecLiteral(s, n, factory);
        case 'h':
            return ParseHexLiteral(s, n, factory);
        default:
            break;
    }
    return nullptr;
}

/*static*/ NyInt *NyInt::ParseOctLiteral(const char *s, size_t n, ObjectFactory *factory) {
    DCHECK_EQ('o', base::Slice::LikeNumber(s, n));
    
    bool negative = false;
    n--; // skip '0'
    s++;
    NyInt *rv = ParseDigitals(s, n, 8, factory);
    if (rv) {
        rv->set_negative(negative);
    }
    return rv;
}

/*static*/ NyInt *NyInt::ParseHexLiteral(const char *s, size_t n, ObjectFactory *factory) {
    DCHECK_EQ('h', base::Slice::LikeNumber(s, n));
    
    bool negative = false;
    n -= 2; // skip '0x'
    s += 2;
    
    size_t required_bits = big::GetNumberOfBits(n, 16);
    size_t required_segments = (required_bits + 31) / 32;
    NyInt *rv = factory->NewUninitializedInt(required_segments);
    int64_t i = n, bit = 0;
    while (i-- > 0) {
        uint32_t n = big::Char2Digital(s[i]);
        size_t j = rv->segments_size() - (bit >> 5) - 1;
        if ((bit & 0x1f) == 0) {
            rv->set_segment(j, 0);
        }
        rv->set_segment(j, rv->segment(j) | (n << (bit & 0x1f)));
        bit += 4;
    }
    rv->set_negative(negative);
    return rv;
}

/*static*/ NyInt *NyInt::ParseDecLiteral(const char *s, size_t n, ObjectFactory *factory) {
#if defined(DEBUG) || defined(_DEBUG)
    int r = base::Slice::LikeNumber(s, n);
    DCHECK(r == 'd' || r == 's');
#endif
    
    bool negative = false;
    if (s[0] == '-' || s[0] == '+') {
        negative = (s[0] == '-') ? true : false;
        n--; // skip '-'
        s++;
    } else {
        negative = false;
    }
    NyInt *rv = ParseDigitals(s, n, 10, factory);
    if (rv) {
        rv->set_negative(negative);
    }
    return rv;
}
    
/*static*/ NyInt *NyInt::ParseDigitals(const char *s, size_t n, int radix, ObjectFactory *factory) {
    DCHECK_GE(radix, kMinRadix);
    DCHECK_LE(radix, kMaxRadix);
    
    size_t required_bits = big::GetNumberOfBits(n, radix);
    size_t required_segments = (required_bits + 31) / 32 + 1;
    NyInt *rv = factory->NewUninitializedInt(required_segments);
    rv->Fill();

    base::ScopedArena scoped_buf;
    NyInt *tmp = NewUninitialized(rv->capacity_, &scoped_buf);
    tmp->offset_ = rv->offset_ + 1;
    tmp->header_ = rv->header_;
    tmp->Fill();

    const uint32_t scale = radix;
    for (size_t i = 0; i < n; ++i) {
        tmp->Normalize();
        rv->Resize(tmp->segments_size() + 1);
        big::BasicMul(tmp->segment_view(), MakeView(&scale, 1), rv->segment_mut_view());

        const uint32_t elem = big::Char2Digital(s[i]);
        big::Add(rv->segment_view(), MakeView(&elem, 1), rv->segment_mut_view());

        tmp->set_offset(rv->offset());
        ::memcpy(tmp->segments(), rv->segments(), rv->segments_size() * sizeof(uint32_t));
    }
    rv->Normalize();
    return rv;
}
    
/*static*/ NyInt *NyInt::New(const uint32_t *s, size_t n, base::Arena *arena) {
    NyInt *rv = NewUninitialized(n, arena);
    ::memcpy(rv->segments(), s, n * sizeof(uint32_t));
    rv->set_negative(false);
    return rv;
}

/*static*/ NyInt *NyInt::NewUninitialized(size_t capacity, base::Arena *arena) {
    void *chunk = arena->Allocate(RequiredSize(static_cast<uint32_t>(capacity)));
    return new (chunk) NyInt(static_cast<uint32_t>(capacity));
}
    
/*static*/ NyInt *NyInt::NewI64(int64_t val, ObjectFactory *factory) {
    NyInt *rv = factory->NewUninitializedInt(2);
    rv->InitI64(val);
    return rv;
}

/*static*/ NyInt *NyInt::NewI64(int64_t val, base::Arena *arena) {
    NyInt *rv = NewUninitialized(2, arena);
    rv->InitI64(val);
    return rv;
}
    
/*static*/ NyInt *NyInt::NewU64(uint64_t val, ObjectFactory *factory) {
    NyInt *rv = factory->NewUninitializedInt(2);
    rv->InitP64(val, false, val > 0xffffffff ? 2 : 1);
    return rv;
}

/*static*/ NyInt *NyInt::NewU64(uint64_t val, base::Arena *arena) {
    NyInt *rv = NewUninitialized(2, arena);
    rv->InitP64(val, false, val > 0xffffffff ? 2 : 1);
    return rv;
}
    
/*static*/ NyInt *NyInt::New(const uint32_t *s, size_t n, ObjectFactory *factory) {
    NyInt *rv = factory->NewUninitializedInt(n);
    ::memcpy(rv->segments(), s, n * sizeof(uint32_t));
    rv->set_negative(false);
    return rv;
}

/*static*/ int NyInt::AbsCompare(const NyInt *lhs, const NyInt *rhs) {
    return big::Compare(lhs->segment_view(), rhs->segment_view());
}

/*static*/ void NyInt::AddRaw(const NyInt *lhs, const NyInt *rhs, NyInt *rv) {
    bool neg = lhs->negative();
    if (lhs->negative() == rhs->negative()) {
        //   lhs  +   rhs  =   lhs + rhs
        // (-lhs) + (-rhs) = -(lhs + rhs)
        big::Add(lhs->segment_view(), rhs->segment_view(), rv->segment_mut_view());
    } else {
        //   lhs  + (-rhs) = lhs - rhs = -(rhs - lhs)
        // (-lhs) +   rhs  = rhs - lhs = -(lhs - rhs)
        if (AbsCompare(lhs, rhs) >= 0) {
            big::Sub(lhs->segment_view(), rhs->segment_view(), rv->segment_mut_view());
        } else {
            neg = !neg;
            big::Sub(rhs->segment_view(), lhs->segment_view(), rv->segment_mut_view());
        }
    }
    rv->set_negative(neg);
    rv->Normalize();
}
    
/*static*/ std::tuple<NyInt *, NyInt *>
NyInt::DivRaw(const NyInt *lhs, const NyInt *rhs, NyaaCore *N) {
    NyInt *rv, *re;
    if (rhs->segments_size() == 1) {
        rv = N->factory()->NewUninitializedInt(lhs->segments_size());
        auto re_val = big::DivWord(lhs->segment_view(), rhs->segment(0),
                                   rv->segment_mut_view());
        re = NewU64(re_val, N->factory());
    } else {
        DCHECK_GE(lhs->segments_size(), rhs->segments_size());
        //size_t limit = lhs->segments_size() - rhs->segments_size() + 1;
        //const size_t nlen = lhs->segments_size() + 1;
        const size_t limit = (lhs->segments_size() + 1) - rhs->segments_size() + 1;
        rv = N->factory()->NewUninitializedInt(limit);
        
        std::unique_ptr<uint32_t[]> scoped_divisor(new uint32_t[rhs->segments_size()]);
        ::memcpy(scoped_divisor.get(), rhs->segments(),
                 rhs->segments_size() * sizeof(uint32_t));
        re = lhs->DivMagnitude(MakeMutView(scoped_divisor.get(),
                                           rhs->segments_size()), rv, N);
    }
    return {rv, re};
}

NyInt *NyInt::DivMagnitude(MutView<uint32_t> divisor, NyInt *rv, NyaaCore *N) const {
    // Remainder starts as dividend with space for a leading zero
    NyInt *re = N->factory()->NewUninitializedInt(segments_size() + 1);
    re->Fill();
    re->offset_ = 1;
    ::memcpy(re->segments(), segments(), segments_size() * sizeof(uint32_t));
    
    const size_t nlen = segments_size();
    const size_t limit = nlen - divisor.n + 1;
    DCHECK_GE(rv->capacity_, limit);
    rv->Resize(limit);
    rv->Fill();

    // D1 normalize the divisor
    int shift = base::Bits::CountLeadingZeros32(divisor.z[0]);
    if (shift > 0) {
        // First shift will not grow array
        big::PrimitiveShl(divisor, shift);
        // But this one might
        re = re->Shl(shift, N);
    }
    
    // Must insert leading 0 in rem if its length did not change
    if (re->segments_size() == nlen) {
        NyInt *tmp = N->factory()->NewUninitializedInt(re->segments_size() + 1);
        ::memcpy(tmp->segments() + 1, re->segments(), re->segments_size() * sizeof(uint32_t));
        re = tmp;
        re->offset_ = 0;
        re->set_segment(0, 0);
    }
    
    uint64_t dh = divisor.z[0];
    uint32_t dl = divisor.z[1];
    uint32_t qword[2] = {0, 0};
    
    // D2 Initialize j
    for (size_t j = 0; j < limit; j++) {
        // D3 Calculate qhat
        // estimate qhat
        uint32_t qhat = 0, qrem = 0;
        bool skip_correction = false;
        uint32_t nh = re->segment(j);
        uint32_t nh2 = nh + 0x80000000u;
        uint32_t nm = re->segment(j + 1);
        
        if (nh == dh) {
            qhat = ~0;
            qrem = nh + nm;
            skip_correction = qrem + 0x80000000u < nh2;
        } else {
            int64_t chunk = (static_cast<uint64_t>(nh) << 32) |
            (static_cast<uint64_t>(nm));
            if (chunk >= 0) {
                qhat = static_cast<uint32_t>(chunk / dh);
                qrem = static_cast<uint32_t>(chunk - (qhat * dh));
            } else {
                big::DivWord(chunk, dh, qword);
                qhat = qword[0];
                qrem = qword[1];
            }
        }
        
        if (qhat == 0) {
            continue;
        }
        
        if (!skip_correction) { // Correct qhat
            uint64_t nl = static_cast<uint64_t>(re->segment(j + 2));
            uint64_t rs = (static_cast<uint64_t>(qrem) << 32) | nl;
            uint64_t est_product = static_cast<uint64_t>(dl) *
            static_cast<uint64_t>(qhat);
            if (est_product > rs) {
                qhat--;
                qrem = static_cast<uint32_t>(static_cast<uint64_t>(qrem) + dh);
                if (static_cast<uint64_t>(qrem) >= dh) {
                    est_product -= static_cast<uint64_t>(dl);
                    rs = (static_cast<uint64_t>(qrem) << 32) | nl;
                    if (est_product > rs) {
                        qhat--;
                    }
                }
            }
        }
        
        // D4 Multiply and subtract
        re->set_segment(j, 0);
        uint32_t borrow = big::MulSub(re->segment_mut_view(), divisor , qhat, j);
        
        // D5 Test remainder
        if (borrow + 0x80000000u > nh2) {
            // D6 Add back
            big::DivAdd(divisor, re->segment_mut_view(), j + 1);
            qhat--;
        }
        
        // Store the quotient digit
        rv->set_segment(j, qhat);
    } // D7 loop on j
    
    // D8 Unnormalize
    if (shift > 0) {
        re = re->Shr(shift, N);
    }
    
    rv->Normalize();
    re->Normalize();
    return re;
}
    
void NyInt::Normalize() {
    for (size_t i = offset_; i < capacity_; ++i) {
        if (vals_[i] != 0) {
            set_offset(i);
            return;
        }
    }
    set_offset(capacity_/* - 1*/);
}

void NyInt::Resize(size_t n) {
    if (n == segments_size()) {
        return;
    }
    if (n < segments_size()) {
        offset_ += (segments_size() - n);
    } else if (n > segments_size()) {
        DCHECK_LE(n, capacity_);
        offset_ -= (n - segments_size());
    }
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
            Object *args = this;
            N->curr_thd()->Run(fn, &args, 1/*nargs*/, 1/*nrets*/);
            return NyString::Cast(N->curr_thd()->Get(-1));
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
        if (index + 1024 > array_->capacity()) {
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
    
bool NyMap::Equal(Object *rhs, NyaaCore *N) const {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerEq, N)->IsTrue();
}

bool NyMap::LessThan(Object *rhs, NyaaCore *N) const {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerLt, N)->IsTrue();
}

bool NyMap::LessEqual(Object *rhs, NyaaCore *N) const {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerLe, N)->IsTrue();
}
    
Object *NyMap::Add(Object *rhs, NyaaCore *N) const {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerAdd, N);
}

Object *NyMap::Sub(Object *rhs, NyaaCore *N) const {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerSub, N);
}

Object *NyMap::Mul(Object *rhs, NyaaCore *N) const {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerMul, N);
}

Object *NyMap::Div(Object *rhs, NyaaCore *N) const {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerDiv, N);
}

Object *NyMap::Mod(Object *rhs, NyaaCore *N) const {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerMod, N);
}
    
Object *NyMap::AttemptBinaryMetaFunction(Object *rhs, NyString *name, NyaaCore *N) const {
    if (GetMetatable() != N->kmt_pool()->kMap) {
        if (NyRunnable *fn = GetValidMetaFunction(name, N)) {
            Object *args[] = {const_cast<NyMap *>(this), rhs};
            N->curr_thd()->Run(fn, args, 2/*nargs*/, 1/*nrets*/);
            return N->curr_thd()->Get(-1);
        }
    }
    N->Raisef("attempt to call nil `%s' meta function.", name->bytes());
    return nullptr;
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
    Entry *slot = GetSlot(key, N);
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
            DLOG(FATAL) << "noreached";
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
    
    const Entry *slot = GetSlot(key, N);
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
    Entry *slot = GetSlot(key, N);
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
                *slot = *At(prev->next);
                slot->kind = kSlot;
                size_--;
            }
        } break;

        case kFree:
            break;
        case kNode:
        default:
            DLOG(FATAL) << "noreached";
            break;
    }
    return false;
}

bool NyTable::DoPut(Object *key, Object *value, NyaaCore *N) {
    Entry *slot = GetSlot(key, N);
    DCHECK(slot >= entries_ + 1);
    DCHECK(slot < entries_ + 1 + n_slots());
    DCHECK_NOTNULL(value);

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
            DLOG(FATAL) << "noreached";
            break;
    }
    return false;
}
    
void NyTable::Iterate(ObjectVisitor *visitor) {
    for (size_t i = 1; i < capacity_ + 1; ++i) {
        Entry *e = entries_ + i;
        if (e->kind != kFree) {
//            if (NyString *s = NyString::Cast(e->key)) {
//                printf("iterate: %s\n", s->bytes());
//            }
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
    Done(N);
}
    
NyString *NyString::Done(NyaaCore *N) {
    elems_[size_] = 0;
    
    uint32_t *hash_val = reinterpret_cast<uint32_t *>(data());
    if (size() > kLargeStringLength) {
        *hash_val = -1;
    } else {
        *hash_val = base::Hash::Js(bytes(), size());
    }
    
    //data()[this->size()] = 0;
    if (size() < kLargeStringLength && N->kz_pool()) {
        if (!N->kz_pool()->GetOrNull(bytes(), size())) {
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
                return NyInt::NewU64(u64, N->factory());
            }
            i64 = u64;
            break;
        case 'h':
            rv = base::Slice::ParseH64(bytes(), size(), &u64);
            DCHECK_GE(rv, 0);
            if (rv > 0 || u64 > std::numeric_limits<int64_t>::max()) {
                return NyInt::NewU64(u64, N->factory());
            }
            i64 = u64;
            break;
        case 'd':
            rv = base::Slice::ParseI64(bytes(), size(), &i64);
            break;
        case 's':
            rv = base::Slice::ParseI64(bytes(), size(), &i64);
            break;
        case 'f':
        case 'e':
            return N->factory()->NewFloat64(::atof(bytes()));
        case 0:
        default:
            return NySmi::New(0);
    }
    return NySmi::New(i64);
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
        size_ = static_cast<uint32_t>(key);
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
    
int NyRunnable::Apply(Object *argv[], int argc, int nrets, NyaaCore *N) {
//    if (args) {
//        args->SetCallee(Local<Value>::New(this));
//    }
    if (NyClosure *callee = ToClosure()) {
        return callee->Call(argv, argc, nrets, N);
    } else if (NyDelegated *callee = ToDelegated()) {
        return callee->Call(argv, argc, nrets, N);
    }
    DLOG(FATAL) << "Noreached!";
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
    
int NyDelegated::Call(Object *argv[], int argc, int nrets, NyaaCore *N) {
    return N->curr_thd()->TryRun(this, argv, argc, nrets);
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
    return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyFunction:
////////////////////////////////////////////////////////////////////////////////////////////////////

NyFunction::NyFunction(NyString *name, uint8_t n_params, bool vargs, uint32_t n_upvals,
                       uint32_t max_stack, NyString *file_name, NyInt32Array *file_info,
                       NyByteArray *bcbuf, NyArray *proto_pool, NyArray *const_pool, NyaaCore *N)
    : name_(name)
    , n_params_(n_params)
    , vargs_(vargs)
    , n_upvals_(n_upvals)
    , max_stack_(max_stack)
    , file_name_(file_name)
    , file_info_(file_info)
    , bcbuf_(bcbuf)
    , proto_pool_(proto_pool)
    , const_pool_(const_pool) {
    N->BarrierWr(this, &name_, name);
    N->BarrierWr(this, &file_name_, file_name);
    N->BarrierWr(this, &file_info_, file_info);
    N->BarrierWr(this, &bcbuf_, bcbuf);
    N->BarrierWr(this, &proto_pool_, proto_pool);
    N->BarrierWr(this, &const_pool_, const_pool);
    ::memset(upvals_, 0, n_upvals * sizeof(upvals_[0]));
}
    
void NyFunction::SetName(NyString *name, NyaaCore *N) {
    N->BarrierWr(this, &name_, name);
    name_ = name;
}
    
void NyFunction::SetUpval(size_t i, NyString *name, bool in_stack, int32_t index, NyaaCore *N) {
    DCHECK_LT(i, n_upvals_);
    N->BarrierWr(this, &upvals_[i].name, name);
    upvals_[i].name = name;
    upvals_[i].in_stack = in_stack;
    upvals_[i].index = index;
}

void NyFunction::Iterate(ObjectVisitor *visitor) {
    visitor->VisitPointer(this, reinterpret_cast<Object **>(&name_));
    visitor->VisitPointer(this, reinterpret_cast<Object **>(&bcbuf_));
    visitor->VisitPointer(this, reinterpret_cast<Object **>(&file_name_));
    visitor->VisitPointer(this, reinterpret_cast<Object **>(&file_info_));
    visitor->VisitPointer(this, reinterpret_cast<Object **>(&proto_pool_));
    visitor->VisitPointer(this, reinterpret_cast<Object **>(&const_pool_));
    for (uint32_t i = 0; i < n_upvals_; ++i) {
        visitor->VisitPointer(this, reinterpret_cast<Object **>(&upvals_[i].name));
    }
}
    
/*static*/ Handle<NyFunction> NyFunction::Compile(const char *z, size_t n, NyaaCore *N) {
    base::StandaloneArena arena(N->isolate()->env()->GetLowLevelAllocator());
    Parser::Result result = Parser::Parse(z, n, &arena);
    if (result.error) {
        N->Raisef("[%d:%d] %s", result.error_line, result.error_column, result.error->data());
        return Handle<NyFunction>();
    }
    return CodeGen::Generate(N->factory()->NewString(":memory:"), result.block, &arena, N);
}
    
/*static*/ Handle<NyFunction> NyFunction::Compile(const char *file_name, FILE *fp, NyaaCore *N) {
    base::StandaloneArena arena(N->isolate()->env()->GetLowLevelAllocator());
    Parser::Result result = Parser::Parse(fp, &arena);
    if (result.error) {
        N->Raisef("%s [%d:%d] %s", file_name, result.error_line, result.error_column,
                  result.error->data());
        return Handle<NyFunction>();
    }
    return CodeGen::Generate(N->factory()->NewString(file_name), result.block, &arena, N);
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyClosure:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
NyClosure::NyClosure(NyFunction *proto, NyaaCore *N)
    : proto_(DCHECK_NOTNULL(proto)) {
    N->BarrierWr(this, &proto_, proto);
    ::memset(upvals_, 0, proto_->n_upvals() * sizeof(upvals_[0]));
}

void NyClosure::Bind(int i, Object *upval, NyaaCore *N) {
    DCHECK_GE(i, 0);
    DCHECK_LT(i, proto_->n_upvals());
    N->BarrierWr(this, upvals_ + i, upval);
    upvals_[i] = upval;
}

int NyClosure::Call(Object *argv[], int argc, int wanted, NyaaCore *N) {
    return N->curr_thd()->TryRun(this, argv, argc, wanted);
}

/*static*/ Handle<NyClosure> NyClosure::Compile(const char *z, size_t n, NyaaCore *N) {
    try {
        Handle<NyFunction> script = NyFunction::Compile(z, n, N);
        if (script.is_valid()) {
            return N->factory()->NewClosure(*script);
        }
    } catch (NyThread::CatchId e) {
        // ignore
    }
    return Handle<NyClosure>();
}
    
/*static*/ Handle<NyClosure> NyClosure::Compile(const char *file_name, FILE *fp, NyaaCore *N) {
    try {
        Handle<NyFunction> script = NyFunction::Compile(file_name, fp, N);
        if (script.is_valid()) {
            return N->factory()->NewClosure(*script);
        }
    } catch (NyThread::CatchId e) {
        // ignore
    }
    return Handle<NyClosure>();
}
    
/*static*/ int NyClosure::Do(const char *z, size_t n, int wanted, NyMap *env, NyaaCore *N) {
    Handle<NyClosure> script = Compile(z, n, N);
    if (script.is_not_valid()) {
        return -1;
    }
    return N->curr_thd()->TryRun(*script, nullptr/*argv*/, 0/*argc*/, wanted, env);
}
    
/*static*/ int NyClosure::Do(const char *file_name, FILE *fp, int wanted, NyMap *env, NyaaCore *N) {
    Handle<NyClosure> script = Compile(file_name, fp, N);
    if (script.is_not_valid()) {
        return -1;
    }
    return N->curr_thd()->TryRun(*script, nullptr/*argv*/, 0/*argc*/, wanted, env);
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// class NyUDO:
////////////////////////////////////////////////////////////////////////////////////////////////////

void NyUDO::SetFinalizer(Finalizer fp, NyaaCore *N) {
//    uintptr_t word = reinterpret_cast<uintptr_t>(fp);
//    DCHECK_EQ(0, word % 2);
//    tag_ = word | (IgnoreManaged() ? 0x1 : 0);
    N->heap()->AddFinalizer(this, fp);
}
    
NyString *NyUDO::ToString(NyaaCore *N) {
    if (NyRunnable *fn = GetValidMetaFunction(N->bkz_pool()->kInnerStr, N)) {
        Object *args = this;
        N->curr_thd()->Run(fn, &args, 1/*nargs*/, 1/*nrets*/);
        return NyString::Cast(N->curr_thd()->Get(-1));
    }
    return N->factory()->Sprintf("udo: %p", this);
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
    
bool NyUDO::Equal(Object *rhs, NyaaCore *N) const {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerEq, N)->IsTrue();
}

bool NyUDO::LessThan(Object *rhs, NyaaCore *N) const {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerLt, N)->IsTrue();
}

bool NyUDO::LessEqual(Object *rhs, NyaaCore *N) const {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerLe, N)->IsTrue();
}
    
Object *NyUDO::Add(Object *rhs, NyaaCore *N) const {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerAdd, N);
}

Object *NyUDO::Sub(Object *rhs, NyaaCore *N) const {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerSub, N);
}

Object *NyUDO::Mul(Object *rhs, NyaaCore *N) const {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerMul, N);
}

Object *NyUDO::Div(Object *rhs, NyaaCore *N) const {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerDiv, N);
}

Object *NyUDO::Mod(Object *rhs, NyaaCore *N) const {
    return AttemptBinaryMetaFunction(rhs, N->bkz_pool()->kInnerMod, N);
}

Object *NyUDO::AttemptBinaryMetaFunction(Object *rhs, NyString *name, NyaaCore *N) const {
    if (NyRunnable *fn = GetValidMetaFunction(name, N)) {
        Object *args[] = {const_cast<NyUDO *>(this), rhs};
        N->curr_thd()->Run(fn, args, 2/*nargs*/, 1/*nrets*/);
        return N->curr_thd()->Get(-1);
    }
    N->Raisef("attempt to call nil `%s' meta function.", name->bytes());
    return nullptr;
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
DEFINE_TYPE_CHECK(Thread, "thread")

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
        for (i = 0; i < table_->capacity(); ++i) {
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
        for (i = index_ + 1; i < table_->capacity(); ++i) {
            if (array_->Get(i)) {
                break;
            }
        }
        index_ = i;
    }
}

} // namespace nyaa
    
} // namespace mai
