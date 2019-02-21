#ifndef MAI_SQL_DECIMAL_V2_H
#define MAI_SQL_DECIMAL_V2_H

#include "base/base.h"
#include "glog/logging.h"

namespace mai {
namespace base {
class Arena;
} // namespace base
namespace sql {
    
namespace v2 {
    
class Decimal final {
    static constexpr const uint32_t kSignedMask = 0x80000000;
    static constexpr const uint32_t kExpMask = ~kSignedMask;

public:
    int exp() const { return flags() & kExpMask; }
    
    void set_exp(int e) { set_negative_and_exp(negative(), e); }

    bool negative() const { return flags() & kSignedMask; }

    bool positive() const { return !negative(); }
    
    void set_negative(bool neg) { set_negative_and_exp(neg, exp()); }

    size_t segments_size() const { return capacity_ - offset_; }

    size_t segments_capacity() const { return capacity_;  }
    
    uint32_t segment(size_t i) const {
        DCHECK_LT(i, segments_size());
        return segments()[i];
    }
    
    void set_segment(size_t i, uint32_t s) {
        DCHECK_LT(i, segments_size());
        segments()[i] = s;
    }

    size_t offset() const { return offset_; }
    
    void set_offset(size_t i) {
        DCHECK_LT(i, segments_size());
        offset_ = static_cast<uint32_t>(i);
    }
    
    // n / log2(10)
    size_t digitals_size() const {
        // TODO:
        return 0;
    }
    
    bool zero() const;
    
    View<uint32_t> segment_view() const {
        return MakeView(segments(), segments_size());
    }
    
    MutView<uint32_t> segment_mut_view() {
        return MakeMutView(segments(), segments_size());
    }
    
    const uint32_t *segments() const { return base() + offset_; }
    
    uint32_t *segments() { return base() + offset_; }
    
    void set_negative_and_exp(bool neg, int exp) {
        flags_ = MakeFlags(neg, exp);
    }
    
    void Normalize() {
        uint32_t *base = reinterpret_cast<uint32_t *>(this + 1);
        for (size_t i = offset(); i < segments_capacity(); ++i) {
            if (base[i] != 0) {
                set_offset(i);
                break;
            }
        }
    }
    
    Decimal *Shl(int bits, base::Arena *arena);
    
    Decimal *Shr(int bits, base::Arena *arena);
    
    Decimal *Add(const Decimal *rhs, base::Arena *arena) const;
    
    Decimal *Sub(const Decimal *rhs, base::Arena *arena) const;

    Decimal *Mul(const Decimal *rhs, base::Arena *arena) const;
    
    Decimal *Div(const Decimal *rhs, base::Arena *arena) const;

    std::string ToString() const;
    
    Decimal *Clone(base::Arena *arena) const { return NewCopied(this, arena); }
    
    static int AbsCompare(const Decimal *lhs, const Decimal *rhs);
    
    static Decimal *NewCopied(const Decimal *other, base::Arena *arena) {
        Decimal *rv = NewCopied(other->segment_view(), arena);
        rv->set_negative_and_exp(other->negative(), other->exp());
        return rv;
    }
    static Decimal *NewCopied(View<uint32_t> segments, base::Arena *arena);
    
    static Decimal *NewI64(int64_t val, base::Arena *arena);
    static Decimal *NewU64(uint64_t val, base::Arena *arena);
    
    static Decimal *NewUninitialized(size_t capacity, base::Arena *arena);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Decimal);
private:
    Decimal(size_t capacity, size_t offset, uint32_t flags)
        : capacity_(static_cast<uint32_t>(capacity))
        , offset_(static_cast<uint32_t>(offset))
        , flags_(flags) {
    }

    DEF_VAL_GETTER(uint32_t, flags);
    
    uint32_t *base() { return reinterpret_cast<uint32_t *>(this + 1); }

    const uint32_t *base() const { return reinterpret_cast<const uint32_t *>(this + 1); }
    
    static uint32_t MakeFlags(bool neg, int exp) {
        DCHECK_GE(exp, 0);
        DCHECK_LT(exp, kSignedMask);
        return (neg ? kSignedMask : 0) | exp;
    }
    
    uint32_t capacity_;
    uint32_t offset_;
    uint32_t flags_ = 0;
}; // class MutDecimal
    
static_assert(sizeof(Decimal) == sizeof(uint32_t) * 3, "Invalid Decimal size.");
    
} // namespace v2
    
} // namespace sql
    
} // namespace mai

#endif // MAI_SQL_DECIMAL_V2_H
