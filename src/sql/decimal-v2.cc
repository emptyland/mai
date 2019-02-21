#include "sql/decimal-v2.h"
#include "base/arena.h"
#include "base/bit-ops.h"

namespace mai {

namespace sql {
    
// 0 1 2 3 4 5 6
//     0 1 2 3 4
//
int Compare(View<uint32_t> lhs, View<uint32_t> rhs) {
    int64_t n = std::max(lhs.n, rhs.n);
    int64_t d = lhs.n > rhs.n ? lhs.n - rhs.n : rhs.n - lhs.n;
    for (int64_t i = 0; i < n; ++i) {
        uint32_t lval = 0;
        if (n == lhs.n) {
            lval = lhs.z[i];
        } else {
            if (i - d < 0) {
                lval = 0;
            } else {
                lval = lhs.z[i - d];
            }
        }
        
        uint32_t rval = 0;
        if (n == rhs.n) {
            rval = rhs.z[i];
        } else {
            if (i - d < 0) {
                rval = 0;
            } else {
                rval = rhs.z[i - d];
            }
        }
        
        if (lval < rval) {
            return -1;
        } else if (lval > rval) {
            return 1;
        }
    }
    return 0;
}
    
void PrimitiveShl(MutView<uint32_t> vals, int n) {
    int shift = 32 - n;
    uint32_t c = vals.z[0];
    for (size_t i = 0; i < vals.n - 1; ++i) {
        uint32_t b = c;
        c = vals.z[i + 1];
        vals.z[i] = (b << n) | (c >> shift);
    }
    vals.z[vals.n - 1] <<= n;
}
    
void PrimitiveShr(MutView<uint32_t> vals, int n) {
    int shift = 32 - n;
    uint32_t c = vals.z[vals.n - 1];
    for (size_t i = vals.n - 1; i > 0; i--) {
        uint32_t b = c;
        c = vals.z[i - 1];
        vals.z[i] = (c << shift) | (b >> n);
    }
    vals.z[0] >>= n;
}
    
uint32_t Add(View<uint32_t> lhs, View<uint32_t> rhs, MutView<uint32_t> rv) {
    int64_t x = lhs.n, y = rhs.n;
    DCHECK_GE(rv.n, std::max(lhs.n, rhs.n));
    
    int64_t restart = rv.n - 1;
    uint64_t sum = 0, carry = 0;
    
    // Add common parts of both numbers
    while (x > 0 && y > 0) {
        x--;
        y--;
        sum = static_cast<uint64_t>(lhs.z[x])
            + static_cast<uint64_t>(rhs.z[y]) + carry;
        rv.z[restart--] = static_cast<uint32_t>(sum);
        carry = sum >> 32;
    }
    
    // Add remainder of the longer number
    while (x > 0) {
        x--;
        sum = static_cast<uint64_t>(lhs.z[x]) + carry;
        rv.z[restart--] = static_cast<uint32_t>(sum);
        carry = sum >> 32;
    }
    while (y > 0) {
        y--;
        sum = static_cast<uint64_t>(rhs.z[y]) + carry;
        rv.z[restart--] = static_cast<uint32_t>(sum);
        carry = sum >> 32;
    }
    
    return static_cast<uint32_t>(carry);
}
    
uint32_t Sub(View<uint32_t> lhs, View<uint32_t> rhs, MutView<uint32_t> rv) {
    
    return 0;
}
    
void DivWord(uint64_t n, uint64_t d, uint32_t rv[2]) {
    if (d == 1) {
        rv[0] = static_cast<uint32_t>(n);
        rv[1] = 0;
        return;
    }
    
    uint64_t q = (n >> 1) / (d >> 1);
    uint64_t r = n - q * d;
    
    while (r < 0) {
        r += d;
        q--;
    }
    
    rv[0] = static_cast<uint32_t>(q);
    rv[1] = static_cast<uint32_t>(r);
}
    
uint32_t DivWord(View<uint32_t> lhs, uint32_t rhs, MutView<uint32_t> rv) {
    uint64_t divisor = static_cast<uint64_t>(rhs);
    if (lhs.n == 1) {
        uint64_t dividend = static_cast<uint64_t>(lhs.z[0]);
        uint32_t q = static_cast<uint32_t>(dividend / divisor);
        uint32_t r = static_cast<uint32_t>(dividend - q * divisor);
        
        for (size_t i = 0; i < rv.n; ++i) {
            rv.z[i] = 0;
        }
        rv.z[0] = q;
        return r;
    }
    
    int shift = base::Bits::CountLeadingZeros32(rhs);
    uint32_t r = lhs.z[0];
    uint64_t rem = static_cast<uint64_t>(r);
    if (rem < divisor) {
        rv.z[0] = 0;
    } else {
        uint32_t d = static_cast<uint32_t>(rem / divisor);
        rv.z[0] = d;
        r = static_cast<uint32_t>(rem - (d * divisor));
        rem = static_cast<uint64_t>(r);
    }
    
    uint32_t qword[2] = {0, 0};
    for (int64_t i = 1; i < lhs.n; ++i) {
        int64_t estimate = (rem << 32) | static_cast<int64_t>(lhs.z[i]);
        if (estimate >= 0) {
            qword[0] = static_cast<uint32_t>(estimate / divisor);
            qword[1] = static_cast<uint32_t>(estimate - qword[0] * divisor);
        } else {
            DivWord(estimate, divisor, qword);
        }
        rv.z[i] = qword[0];
        r = qword[1];
        rem = static_cast<uint64_t>(r);
    }

    return shift > 0 ? r % divisor : r;
}
    
namespace v2 {
    
////////////////////////////////////////////////////////////////////////////////
/// class Decimal
////////////////////////////////////////////////////////////////////////////////
    
bool Decimal::zero() const {
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
   
Decimal *Decimal::Shl(int n, base::Arena *arena) {
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
        sql::PrimitiveShl(segment_mut_view(), n);
        return this;
    }
    
    size_t new_len = segments_size() + n_ints + 1;
    if (n_bits <= 32 - hi_word_bits) {
        new_len--;
    }
    
    Decimal *rv = this;
    if (capacity_ < new_len) {
        // The array must grow
        rv = NewUninitialized(new_len, arena);
        for (size_t i = 0; i < segments_size(); ++i) {
            rv->segments()[i] = segment(i);
        }
        for (size_t i = segments_size(); i < new_len; ++i) {
            rv->segments()[i] = 0;
        }
    } else if (segments_size() >= new_len) {
        uint32_t new_off  = static_cast<uint32_t>(capacity_ - new_len);
        for (int64_t i = new_len - 1; i >= 0; --i) {
            base()[new_off + i] = segment(i);
        }
        offset_ = new_off;
    } else {
        uint32_t new_off  = static_cast<uint32_t>(capacity_ - new_len);
        for (int64_t i = segments_size() - 1; i >= 0; --i) {
            base()[new_off + i] = segment(i);
        }
        offset_ = new_off;
        for (size_t i = segments_size(); i < new_len; ++i) {
            segments()[i] = 0;
        }
    }
    
    if (n_bits == 0) {
        return rv;
    }
    if (n_bits <= 32 - hi_word_bits) {
        sql::PrimitiveShl(rv->segment_mut_view(), n_bits);
    } else {
        sql::PrimitiveShr(rv->segment_mut_view(), 32 - n_bits);
    }
    return rv;
}

Decimal *Decimal::Shr(int n, base::Arena *arena) {
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
        sql::PrimitiveShl(segment_mut_view(), 32 - n_bits);
        ::memmove(segments() + 1, segments(),
                  (segments_size() - 1) * sizeof(uint32_t));
        offset_++;
    } else {
        sql::PrimitiveShr(segment_mut_view(), n_bits);
    }
    return this;
}

Decimal *Decimal::Add(const Decimal *rhs, base::Arena *arena) const {
    size_t n_ints = std::max(segments_size(), rhs->segments_size()) + 1;
    Decimal *rv = NewUninitialized(n_ints, arena);
    rv->segments()[0] = 0;
    bool neg = negative();
    if (negative() == rhs->negative()) {
        //   lhs  +   rhs  =   lhs + rhs
        // (-lhs) + (-rhs) = -(lhs + rhs)
        sql::Add(segment_view(), rhs->segment_view(), rv->segment_mut_view());
    } else {
        //   lhs  + (-rhs) = lhs - rhs = -(rhs - lhs)
        // (-lhs) +   rhs  = rhs - lhs = -(lhs - rhs)
        if (AbsCompare(this, rhs) >= 0) {
            sql::Sub(segment_view(), rhs->segment_view(), rv->segment_mut_view());
        } else {
            neg = !neg;
            sql::Sub(rhs->segment_view(), segment_view(), rv->segment_mut_view());
        }
    }
    rv->set_negative_and_exp(neg, exp());
    rv->Normalize();
    return rv;
}

Decimal *Decimal::Sub(const Decimal *rhs, base::Arena *arena) const {
    size_t n_ints = std::max(segments_size(), rhs->segments_size()) + 1;
    Decimal *rv = NewUninitialized(n_ints, arena);
    bool neg = negative();
    if (negative() != rhs->negative()) {
        // -lhs -   rhs  = -(lhs + rhs)
        //  lhs - (-rhs) =   lhs + rhs
        sql::Add(segment_view(), rhs->segment_view(), rv->segment_mut_view());
    } else {
        //   lhs  -   rhs  = lhs - rhs = -(rhs - lhs)
        // (-lhs) - (-rhs) = rhs - lhs = -(lhs - rhs)
        if (AbsCompare(this, rhs) >= 0) {
            sql::Sub(segment_view(), rhs->segment_view(), rv->segment_mut_view());
        } else {
            neg = !neg;
            sql::Sub(rhs->segment_view(), segment_view(), rv->segment_mut_view());
        }
    }
    rv->set_negative_and_exp(neg, exp());
    rv->Normalize();
    return rv;
}

Decimal *Decimal::Mul(const Decimal *rhs, base::Arena *arena) const {
    // TODO:
    return nullptr;
}

Decimal *Decimal::Div(const Decimal *rhs, base::Arena *arena) const {
    // TODO:
    return nullptr;
}

std::string Decimal::ToString() const {
    if (zero()) {
        return "0";
    }
    const size_t size = sizeof(Decimal) + capacity_ * sizeof(uint32_t);
    
    std::unique_ptr<uint8_t[]> bufq(new uint8_t[size]);
    ::memset(bufq.get(), 0, size);
    Decimal *q = new (bufq.get()) Decimal(capacity_, offset_, flags_);

    std::unique_ptr<uint8_t[]> bufp(new uint8_t[size]);
    ::memcpy(bufp.get(), this, size);
    Decimal *p = reinterpret_cast<Decimal *>(bufp.get());
    
    std::string buf;
    while (!p->zero()) {
        uint32_t m = DivWord(p->segment_view(), 10, q->segment_mut_view());
        DCHECK_LT(m, 10);
        buf.insert(buf.begin(), '0' + m);
        ::memcpy(bufp.get(), bufq.get(), size);
    }
    if (negative()) {
        buf.insert(buf.begin(), '-');
    }

    return buf;
}

    
/*static*/
int Decimal::AbsCompare(const Decimal *lhs, const Decimal *rhs) {
    DCHECK_EQ(lhs->exp(), rhs->exp());
    return Compare(lhs->segment_view(), rhs->segment_view());
}

/*static*/ Decimal *Decimal::NewI64(int64_t val, base::Arena *arena) {
    bool neg = val < 0;
    if (neg) {
        val = -val;
    }
    uint32_t hi_bits = static_cast<uint32_t>((val & 0xffffffff00000000ull) >> 32);
    Decimal *rv = NewUninitialized((hi_bits ? 2 : 1), arena);
    rv->set_negative_and_exp(neg, 0);
    if (hi_bits > 0) {
        rv->segments()[0] = hi_bits;
        rv->segments()[1] = static_cast<uint32_t>(val);
    } else {
        rv->segments()[0] = static_cast<uint32_t>(val);
    }
    return rv;
}

/*static*/ Decimal *Decimal::NewU64(uint64_t val, base::Arena *arena) {
    uint32_t hi_bits = static_cast<uint32_t>((val & 0xffffffff00000000ull) >> 32);
    Decimal *rv = NewUninitialized((hi_bits ? 2 : 1), arena);
    if (hi_bits > 0) {
        rv->segments()[0] = hi_bits;
        rv->segments()[1] = static_cast<uint32_t>(val);
    } else {
        rv->segments()[0] = static_cast<uint32_t>(val);
    }
    return rv;
}
    
/*static*/ Decimal *Decimal::NewCopied(View<uint32_t> segments,
                                       base::Arena *arena) {
    Decimal *rv = NewUninitialized(segments.n, arena);
    ::memcpy(rv->segments(), segments.z, segments.n * sizeof(uint32_t));
    rv->set_negative_and_exp(false, 0);
    return rv;
}

// 010.01 : 100
// 000.10 : 100
// 100.10 : t
// 001.00 : f
// ------------
// 010.00 : 100
// 001.00 : 100
// 010.00 : t
// 000.10 : f
//-------------
// 010.01  : 100
// 000.100 : 1000
// 100.100 : t
// 000.100 : f
/*static*/ Decimal *Decimal::NewUninitialized(size_t capacity,
                                              base::Arena *arena) {
    void *chunk = arena->Allocate(sizeof(Decimal) +
                                  capacity * sizeof(uint32_t));

    return new (chunk) Decimal(capacity, 0, 0);
}
    
} // namespace v2

} // namespace sql

} // namespace mai
