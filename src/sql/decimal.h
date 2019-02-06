#ifndef MAI_SQL_DECIMAL_H_
#define MAI_SQL_DECIMAL_H_

#include "base/arena-utils.h"

namespace mai {
    
namespace sql {

class Decimal final : public base::ArenaString {
public:
    int exp() const { return data()[0] & 0x7f; }
    
    bool negative() const { return !positive(); }
    
    bool positive() const { return data()[0] & 0x80; }
    
    int digital(size_t i) const {
        uint32_t d = segment(i / 9) / kPowExp[i % 9];
        return static_cast<int>(d % 10);
    }
    
    size_t digital_size() const { return segment_size() * 9; }
    
    size_t segment_size() const { return (size() - 1) / 4; }
    
    Decimal *Add(const Decimal *rhs, base::Arena *arena) const;
    Decimal *Sub(const Decimal *rhs, base::Arena *arena) const;
    Decimal *Mul(const Decimal *rhs, base::Arena *arena) const;
    Decimal *Div(const Decimal *rhs, base::Arena *arena) const;
    
    std::string ToString() const;

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
    uint32_t segment(size_t i) const {
        DCHECK_LT(i, segment_size());
        const uint8_t *x = reinterpret_cast<const uint8_t *>(data() + 1 + i * 4);
        return static_cast<uint32_t>(x[3])      |
            (static_cast<uint32_t>(x[2]) << 8)  |
            (static_cast<uint32_t>(x[1]) << 16) |
            (static_cast<uint32_t>(x[0]) << 24);
        
    }
    
    void set_segment(size_t i, uint32_t segment) {
        DCHECK_LT(i, segment_size());
        uint8_t *x = reinterpret_cast<uint8_t *>(prepared() + 1 + i * 4);
        x[3] = static_cast<uint8_t>(segment & 0xff);
        x[2] = static_cast<uint8_t>((segment & 0xff00) >> 8);
        x[1] = static_cast<uint8_t>((segment & 0xff0000) >> 16);
        x[0] = static_cast<uint8_t>((segment & 0xff000000) >> 24);
    }
    
    static const uint32_t kPowExp[9];
}; // class Decimal
    
static_assert(sizeof(Decimal) == sizeof(base::ArenaString), "Unexpected size.");
    
} // namespace sql
    
} // namespace mai


#endif // MAI_SQL_DECIMAL_H_
