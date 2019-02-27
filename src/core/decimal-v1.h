#ifndef MAI_CORE_DECIMAL_V1_H_
#define MAI_CORE_DECIMAL_V1_H_

#include "base/arena-utils.h"
#include <tuple>

namespace mai {
    
namespace core {
    
namespace v1 {

class Decimal final : public base::ArenaString {
public:
    static constexpr const int kMaxDigitalSize = 72;
    static constexpr const int kMaxExp = 30;
    static constexpr const int kHeaderSize = 4;
    static constexpr const int kMaxSegmentBits = 30;
    static constexpr const int kMinConstVal = -10;
    static constexpr const int kMaxConstVal = 10;
    static constexpr const int kConstZeroIdx = (kMaxConstVal - kMinConstVal) / 2;
    
    static Decimal *const kZero;
    static Decimal *const kOne;
    static Decimal *const kMinusOne;
    static Decimal *const kConstants[kMaxConstVal - kMinConstVal + 1];
    
    int exp() const { return data()[0] & 0x7f; }
    
    bool negative() const { return !positive(); }
    
    bool positive() const { return data()[0] & 0x80; }
    
    int digital(size_t i) const {
        uint32_t d = segment(digital_to_segment(i)) / kPowExp[i % 9];
        return static_cast<int>(d % 10);
    }
    
    size_t digital_to_segment(size_t i) const {
        DCHECK_LT(i, digitals_size());
        return segments_size() - 1 - i / 9;
    }
    
    int rdigital(size_t i) const {
        DCHECK_LT(i, digitals_size());
        uint32_t d = segment(i / 9) / kPowExp[8 - i % 9];
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
    
    size_t highest_digital() const;
    
    int valid_exp() const {
        int i = exp();
        while (i-- > 0) {
            if (digital(i) != 0) {
                return exp() - i;
            }
        }
        return 0;
    }
    
    const uint32_t *segments() const {
        return reinterpret_cast<const uint32_t *>(data() + kHeaderSize);
    }
    
    uint32_t segment(size_t i) const {
        DCHECK_LT(i, segments_size());
        const uint8_t *x = reinterpret_cast<const uint8_t *>(data() + kHeaderSize + i * 4);
        return static_cast<uint32_t>(x[3])      |
        (static_cast<uint32_t>(x[2]) << 8)  |
        (static_cast<uint32_t>(x[1]) << 16) |
        (static_cast<uint32_t>(x[0]) << 24);
    }

    Decimal *Shl(uint32_t n, base::Arena *arena) const;
    Decimal *Shr(uint32_t n, base::Arena *arena) const;
    Decimal *Add(const Decimal *rhs, base::Arena *arena) const;
    Decimal *Sub(const Decimal *rhs, base::Arena *arena) const;
    Decimal *Mul(const Decimal *rhs, base::Arena *arena) const;
    std::tuple<Decimal *, Decimal *> Div(const Decimal *rhs,
                                         base::Arena *arena) const;


    void PrimitiveShl(uint32_t n) {
        DCHECK(!IsConstant());
        RawShl(this, n, this);
    }
    void PrimitiveShr(uint32_t n) {
        DCHECK(!IsConstant());
        RawShr(this, n, this);
    }
    
    int Compare(const Decimal *rhs) const;
    int AbsCompare(const Decimal *rhs) const;
    
    bool IsConstant() const {
        int i = kMaxConstVal - kMinConstVal + 1;
        while (i-- > 0) {
            if (this == kConstants[i]) {
                return true;
            }
        }
        return false;
    }
    
    std::string ToString() const;
    
    Decimal *Extend(int d, int m, base::Arena *arena) const;
    
    Decimal *ExtendFrom(const Decimal *from, base::Arena *arena) const {
        return Extend(static_cast<int>(from->digitals_size()), from->exp(), arena);
    }
    
    static Decimal *GetConstant(int val) {
        DCHECK_GE(val, kMinConstVal);
        DCHECK_LE(val, kMaxConstVal);
        return kConstants[kConstZeroIdx + val];
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
    
    FRIEND_UNITTEST_CASE(DecimalTest, DivSlow);
private:
    static const uint32_t kLimitMod = 1000000000;
    
    uint32_t *segments() {
        return reinterpret_cast<uint32_t *>(prepared() + kHeaderSize);
    }
    
    static Decimal *NewUninitialized(base::Arena *arena, size_t digital_size) {
        size_t n_segment = (digital_size + 8) / 9;
        size_t size = n_segment * 4 + kHeaderSize;
        return static_cast<Decimal *>(NewPrepare(arena, size));
    }
    
    int GetDigitalOrZero(int64_t i) const {
        return (i < 0 || i >= digitals_size()) ? 0 : digital(i);
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
    static Decimal *RawDiv(const Decimal *lhs, const Decimal *rhs, Decimal *rv,
                           Decimal *rem, base::Arena *arena);
    static void RawDivSlow(const Decimal *lhs, const Decimal *rhs,
                           Decimal *tmp, Decimal *rv, Decimal *rem);
    static uint32_t RawDivWord(const Decimal *lhs, uint32_t rhs, Decimal *rv);
    static void RawDivWord(uint64_t n, uint64_t d, uint32_t rv[2]);
    static uint32_t RawShl(const Decimal *lhs, uint32_t n, Decimal *rv);
    static uint32_t RawShr(const Decimal *lhs, uint32_t n, Decimal *rv);
    
    // int divadd(int[] a, int[] result, int offset)
    static uint32_t DivAdd(const Decimal *a, Decimal *rv, size_t offset);
    // int mulsub(int[] q, int[] a, int x, int len, int offset)
    static uint32_t MulSub(const Decimal *a, Decimal *q, uint32_t x, size_t len,
                           size_t offset);
    
    static const uint32_t kPowExp[9];
}; // class Decimal
    
static_assert(sizeof(Decimal) == sizeof(base::ArenaString), "Unexpected size.");
    
} // namespace v1
    
} // namespace core
    
} // namespace mai


#endif // MAI_CORE_DECIMAL_V1_H_
