#ifndef MAI_CORE_DECIMAL_V2_H
#define MAI_CORE_DECIMAL_V2_H

#include "base/base.h"
#include "glog/logging.h"

namespace mai {
namespace base {
class Arena;
} // namespace base
namespace core {
    
namespace v2 {
    
class Decimal final {
    static constexpr const uint32_t kSignedMask = 0x80000000;
    static constexpr const uint32_t kExpMask = ~kSignedMask;

public:
    static constexpr const int kMaxExp = 31;
    static constexpr const int kMinRadix = 2;
    static constexpr const int kMaxRadix = 16;
    
    int exp() const { return flags() & kExpMask; }
    
    void set_exp(int e) { set_negative_and_exp(negative(), e); }
    
    int sign() const { return negative() ? -1 : 1; }

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
        DCHECK_LE(i, capacity_);
        offset_ = static_cast<uint32_t>(i);
    }
    
    // n / log2(10)
    size_t max_digitals_size(int radix = 10) const {
        return GetNumberOfDigitals(segments_size() * 32, radix);
    }
    
    const Decimal *scale() const { return GetFastPow10(exp()); }
    
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
    
    void Normalize();
    
    Decimal *Shl(int bits, base::Arena *arena);
    
    Decimal *Shr(int bits, base::Arena *arena);
    
    Decimal *Add(const Decimal *rhs, base::Arena *arena) const;
    
    Decimal *Sub(const Decimal *rhs, base::Arena *arena) const;

    Decimal *Mul(const Decimal *rhs, base::Arena *arena) const;
    
    std::tuple<Decimal *, Decimal *> Div(const Decimal *rhs,
                                         base::Arena *arena) const;
    
    Decimal *Sqrt(base::Arena *arena) const;

    std::string ToString(int radix = 10) const;
    
    int64_t ToI64() const;
    
    float ToF32() const { return static_cast<float>(ToF64()); }
    
    double ToF64() const;
    
    Decimal *Clone(base::Arena *arena) const { return NewCopied(this, arena); }
    
    int Compare(const Decimal *rhs) const;
    
    Decimal *NewPrecision(int new_exp, base::Arena *arena) const {
        if (exp() > new_exp) {
            return Shrink(new_exp, arena);
        } else if (exp() < new_exp) {
            return Extend(new_exp, arena);
        } else {
            return const_cast<Decimal *>(this);
        }
    }
    
    Decimal *Extend(int new_exp, base::Arena *arena) const;
    
    Decimal *Shrink(int new_exp, base::Arena *arena) const;
    
    void Fill(uint32_t s = 0) {
        int64_t i = segments_size();
        while (i-- > 0) { set_segment(i, s); }
    }
    
    static int AbsCompare(const Decimal *lhs, const Decimal *rhs);
    
    static Decimal *NewCopied(const Decimal *other, base::Arena *arena) {
        Decimal *rv = NewCopied(other->segment_view(), arena);
        rv->set_negative_and_exp(other->negative(), other->exp());
        return rv;
    }
    static Decimal *NewCopied(View<uint32_t> segments, base::Arena *arena) {
        return NewCopied(segments.z, segments.n, arena);
    }
    
    static Decimal *NewCopied(const uint32_t *s, size_t n, base::Arena *arena);
    
    static Decimal *NewI64(int64_t val, base::Arena *arena);
    static Decimal *NewU64(uint64_t val, base::Arena *arena);
    
    static Decimal *NewParsed(const char *s, base::Arena *arena) {
        return NewParsed(s, strlen(s), arena);
    }
    
    static Decimal *NewParsed(const std::string &s, base::Arena *arena) {
        return NewParsed(s.c_str(), s.length(), arena);
    }
    
    static Decimal *NewParsed(const char *s, size_t n, base::Arena *arena);
    
    static Decimal *NewOctLiteral(const char *s, size_t n, base::Arena *arena);
    
    static Decimal *NewHexLiteral(const char *s, size_t n, base::Arena *arena);
    
    static Decimal *NewDecLiteral(const char *s, size_t n, base::Arena *arena);
    
    static Decimal *NewRealLiteral(const char *s, size_t n, base::Arena *arena);
    
    static Decimal *NewExpLiteral(const char *s, size_t n, base::Arena *arena);
    
    static Decimal *ParseDigitals(const char *s, size_t n, int radix,
                                  base::Arena *arena);
    
    static Decimal *NewUninitialized(size_t capacity, base::Arena *arena);
    
    static const Decimal *GetFastPow10(int exp);
    
    static const Decimal *ValueOf(int64_t value, base::Arena *arena);
    
    static size_t GetNumberOfDigitals(size_t bits, int radix);

    static size_t GetNumberOfBits(size_t digitals_count, int radix);
    
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
    
    void Resize(size_t n) {
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
    
    static std::tuple<Decimal *, Decimal *>
    DivRaw(const Decimal *lhs, const Decimal *rhs, base::Arena *arena);
    
    Decimal *DivMagnitude(MutView<uint32_t> divisor, Decimal *rv,
                          base::Arena *arena) const;
    
    static uint32_t MakeFlags(bool neg, int exp) {
        DCHECK_GE(exp, 0);
        DCHECK_LT(exp, kSignedMask);
        return (neg ? kSignedMask : 0) | exp;
    }
    
    uint32_t capacity_;
    uint32_t offset_;
    uint32_t flags_;
}; // class MutDecimal
    
static_assert(sizeof(Decimal) == sizeof(uint32_t) * 3, "Invalid Decimal size.");
    
} // namespace v2
    
} // namespace core
    
} // namespace mai

#endif // MAI_CORE_DECIMAL_V2_H
