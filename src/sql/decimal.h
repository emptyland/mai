#ifndef MAI_SQL_DECIMAL_H_
#define MAI_SQL_DECIMAL_H_

#include "base/arena-utils.h"
#include <tuple>

namespace mai {
    
namespace sql {

class Decimal final : public base::ArenaString {
public:
    static const int kMaxDigitalSize = 72;
    static const int kMaxExp = 30;
    static constexpr const int kHeaderSize = 4;
    
    static Decimal *const kZero;
    static Decimal *const kOne;
    static Decimal *const kMinusOne;
    
    int exp() const { return data()[0] & 0x7f; }
    
    bool negative() const { return !positive(); }
    
    bool positive() const { return data()[0] & 0x80; }
    
    int digital(size_t i) const {
        uint32_t d = segment(segments_size() - 1 - i / 9) / kPowExp[i % 9];
        return static_cast<int>(d % 10);
    }
    
    size_t digitals_size() const { return segments_size() * 9; }
    
    size_t segments_size() const {
        DCHECK_EQ(0, (size() - kHeaderSize) % 4);
        return (size() - kHeaderSize) / 4;
    }
    
    bool zero() const {
        if (this == kZero || segments_size() == 0) {
            return true;
        }
        int64_t i = segments_size();
        while (i-- > 0) {
            if (segment(i) != 0) {
                return false;
            }
        }
        return true;
    }
    
    const uint32_t *segments() const {
        return reinterpret_cast<const uint32_t *>(data() + kHeaderSize);
    }
    
    Decimal *Add(const Decimal *rhs, base::Arena *arena) const;
    Decimal *Sub(const Decimal *rhs, base::Arena *arena) const;
    Decimal *Mul(const Decimal *rhs, base::Arena *arena) const;
    std::tuple<Decimal *, Decimal *> Div(const Decimal *rhs,
                                         base::Arena *arena) const;
    
    Decimal *Shl(uint32_t n, base::Arena *arena) const;
    Decimal *Shr(uint32_t n, base::Arena *arena) const;
    
    int Compare(const Decimal *rhs) const;
    int AbsCompare(const Decimal *rhs) const;
    
    std::string ToString() const;
    
    Decimal *Extend(int d, int m, base::Arena *arena) const;
    
    Decimal *ExtendFrom(const Decimal *from, base::Arena *arena) const {
        return Extend(static_cast<int>(from->digitals_size()), from->exp(), arena);
    }

    static Decimal *New(base::Arena *arena, const char *s, size_t n);
    
    static Decimal *New(base::Arena *arena, const char *s) {
        return New(arena, s, ::strlen(s));
    }
    
    static Decimal *NewI64(base::Arena *arena, int64_t value);
    
    static Decimal *NewU64(base::Arena *arena, uint64_t value);
    
    static Decimal *NewWithRawData(base::Arena *arena, const char *s, size_t n) {
        return static_cast<Decimal *>(base::ArenaString::New(arena, s, n));
    }
private:
    static const uint32_t kLimitMod = 1000000000;
    
    static Decimal *NewUninitialized(base::Arena *arena, size_t digital_size) {
        size_t n_segment = (digital_size + 8) / 9;
        size_t size = n_segment * 4 + kHeaderSize;
        return static_cast<Decimal *>(NewPrepare(arena, size));
    }
    
    uint32_t segment(size_t i) const {
        DCHECK_LT(i, segments_size());
        const uint8_t *x = reinterpret_cast<const uint8_t *>(data() + kHeaderSize + i * 4);
        return static_cast<uint32_t>(x[3])      |
            (static_cast<uint32_t>(x[2]) << 8)  |
            (static_cast<uint32_t>(x[1]) << 16) |
            (static_cast<uint32_t>(x[0]) << 24);
        
    }
    
    void set_segment(size_t i, uint32_t segment) {
        DCHECK_LT(i, segments_size());
        uint8_t *x = reinterpret_cast<uint8_t *>(prepared() + kHeaderSize + i * 4);
        x[3] = static_cast<uint8_t>(segment & 0xff);
        x[2] = static_cast<uint8_t>((segment & 0xff00) >> 8);
        x[1] = static_cast<uint8_t>((segment & 0xff0000) >> 16);
        x[0] = static_cast<uint8_t>((segment & 0xff000000) >> 24);
    }
    
    void set_negative_and_exp(bool negative, int exp) {
        prepared()[0] = (negative ? 0x00 : 0x80) | static_cast<int8_t>(exp);
    }
    
    static uint32_t RawAdd(const Decimal *lhs, const Decimal *rhs, Decimal *rv);
    static uint32_t RawSub(const Decimal *lhs, const Decimal *rhs, Decimal *rv);
    static uint32_t RawBasicMul(const Decimal *lhs, const Decimal *rhs,
                                Decimal *rv);
    static uint32_t RawShl(const Decimal *lhs, uint32_t n, Decimal *rv);
    static uint32_t RawShr(const Decimal *lhs, uint32_t n, Decimal *rv);
    
    static const uint32_t kPowExp[9];
}; // class Decimal
    
static_assert(sizeof(Decimal) == sizeof(base::ArenaString), "Unexpected size.");
    
} // namespace sql
    
} // namespace mai


#endif // MAI_SQL_DECIMAL_H_
