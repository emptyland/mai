#include "sql/decimal-v2.h"
#include "base/arena.h"
#include "base/slice.h"
#include "base/bit-ops.h"
#include <cmath>

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
    uint64_t diff = 0;
    int64_t x = lhs.n, y = rhs.n, restart = rv.n - 1;
    
    // Subtract common parts of both numbers
    while (y > 0) {
        x--;
        y--;
        
        diff = static_cast<uint64_t>(lhs.z[x])
             - static_cast<uint64_t>(rhs.z[y])
             - static_cast<uint32_t>(-(diff >> 32));
        rv.z[restart--] = static_cast<uint32_t>(diff);
    }
    
    // Subtract remainder of longer number
    while (x > 0) {
        x--;
        diff = static_cast<uint64_t>(lhs.z[x])
             - static_cast<uint32_t>(-(diff >> 32));
        rv.z[restart--] = static_cast<uint32_t>(diff);
    }
    
    return static_cast<uint32_t>(diff);
}
    
uint32_t BasicMul(View<uint32_t> x, View<uint32_t> y, MutView<uint32_t> rv) {
/*
 732           int xLen = intLen;
 733           int yLen = y.intLen;
 734           int newLen = xLen + yLen;
 735
 736           // Put z into an appropriate state to receive product
 737           if (z.value.length < newLen)
 738               z.value = new int[newLen];
 739           z.offset = 0;
 740           z.intLen = newLen;
 741
 742           // The first iteration is hoisted out of the loop to avoid extra add
 743           long carry = 0;
 744           for (int j=yLen-1, k=yLen+xLen-1; j >= 0; j--, k--) {
 745                   long product = (y.value[j+y.offset] & LONG_MASK) *
 746                                  (value[xLen-1+offset] & LONG_MASK) + carry;
 747                   z.value[k] = (int)product;
 748                   carry = product >>> 32;
 749           }
 750           z.value[xLen-1] = (int)carry;
 751
 752           // Perform the multiplication word by word
 753           for (int i = xLen-2; i >= 0; i--) {
 754               carry = 0;
 755               for (int j=yLen-1, k=yLen+i; j >= 0; j--, k--) {
 756                   long product = (y.value[j+y.offset] & LONG_MASK) *
 757                                  (value[i+offset] & LONG_MASK) +
 758                                  (z.value[k] & LONG_MASK) + carry;
 759                   z.value[k] = (int)product;
 760                   carry = product >>> 32;
 761               }
 762               z.value[i] = (int)carry;
 763           }
 764
 765           // Remove leading zeros from product
 766           z.normalize();
 */
    int64_t xlen = x.n, ylen = y.n;
    DCHECK_EQ(rv.n, xlen + ylen);
    
    // The first iteration is hoisted out of the loop to avoid extra add
    uint64_t carry = 0;
    for (int64_t j = ylen - 1, k = ylen + xlen - 1; j >= 0; j--, k--) {
        uint64_t product = static_cast<uint64_t>(y.z[j]) *
                           static_cast<uint64_t>(x.z[xlen - 1]) + carry;
        rv.z[k] = static_cast<uint32_t>(product);
        carry = product >> 32;
    }
    rv.z[xlen - 1] = static_cast<uint32_t>(carry);
    
    // Perform the multiplication word by word
    for (int64_t i = xlen - 2; i >= 0; i--) {
        carry = 0;
        for (int64_t j = ylen - 1, k = ylen + i; j >= 0; j--, k--) {
            uint64_t product = static_cast<uint64_t>(y.z[j]) *
                               static_cast<uint64_t>(x.z[i]) +
                               static_cast<uint64_t>(rv.z[k]) + carry;
            rv.z[k] = static_cast<uint32_t>(product);
            carry = product >> 32;
        }
        rv.z[i] = static_cast<uint32_t>(carry);
    }

    return static_cast<uint32_t>(carry);
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

#if 0
static const uint32_t kPow10Ints[][4] = {
    {0,            0,          0,          1}, // 1
    {0,            0,          0,        0xa}, // 10
    {0,            0,          0,       0x64}, // 100
    {0,            0,          0,      0x3e8}, // 1000
    {0,            0,          0,     0x2710}, // 10000
    {0,            0,          0,    0x186a0}, // 100000
    {0,            0,          0,    0xf4240}, // 1000000
    {0,            0,          0,   0x989680}, // 10000000
    {0,            0,          0,  0x5f5e100}, // 100000000
    {0,            0,          0, 0x3b9aca00}, // 1000000000
    {0,            0,        0x2, 0x540be400}, // 10000000000
    {0,            0,       0x17, 0x4876e800}, // 100000000000
    {0,            0,       0xe8, 0xd4a51000}, // 1000000000000
    {0,            0,      0x918, 0x4e72a000}, // 10000000000000
    {0,            0,     0x5af3, 0x107a4000}, // 100000000000000
    {0,            0,    0x38d7e, 0xa4c68000}, // 1000000000000000
    {0,            0,   0x2386f2, 0x6fc10000}, // 10000000000000000
    {0,            0,  0x1634578, 0x5d8a0000}, // 100000000000000000
    {0,            0,  0xde0b6b3, 0xa7640000}, // 1000000000000000000
    {0,            0, 0x8ac72304, 0x89e80000}, // 10000000000000000000
    {0,          0x5, 0x6bc75e2d, 0x63100000}, // 100000000000000000000
    {0,         0x36, 0x35c9adc5, 0xdea00000}, // 1000000000000000000000
    {0,        0x21e, 0x19e0c9ba, 0xb2400000}, // 10000000000000000000000
    {0,       0x152d, 0x02c7e14a, 0xf6800000}, // 100000000000000000000000
    {0,       0xd3c2, 0x1bcecced, 0xa1000000}, // 1000000000000000000000000
    {0,      0x84595, 0x16140148, 0x4a000000}, // 10000000000000000000000000
    {0,     0x52b7d2, 0xdcc80cd2, 0xe4000000}, // 100000000000000000000000000
    {0,    0x33b2e3c, 0x9fd0803c, 0xe8000000}, // 1000000000000000000000000000
    {0,   0x204fce5e, 0x3e250261, 0x10000000}, // 10000000000000000000000000000
    {0x1, 0x431e0fae, 0x6d7217ca, 0xa0000000}, // 100000000000000000000000000000
    {0xc, 0x9f2c9cd0, 0x4674edea, 0x40000000}, // 1000000000000000000000000000000
};
#endif

static const uint32_t kPow10DecimalStub[31][3 + 4] = {
// capacity_  | offset_   |  flags_  segments
    {1, 0, 0,          1, 0, 0, 0}, // 1
    {1, 0, 0,        0xa, 0, 0, 0}, // 10
    {1, 0, 0,       0x64, 0, 0, 0}, // 100
    {1, 0, 0,      0x3e8, 0, 0, 0}, // 1000
    {1, 0, 0,     0x2710, 0, 0, 0}, // 10000
    {1, 0, 0,    0x186a0, 0, 0, 0}, // 100000
    {1, 0, 0,    0xf4240, 0, 0, 0}, // 1000000
    {1, 0, 0,   0x989680, 0, 0, 0}, // 10000000
    {1, 0, 0,  0x5f5e100, 0, 0, 0}, // 100000000
    {1, 0, 0, 0x3b9aca00, 0, 0, 0}, // 1000000000
    
    {2, 0, 0,        0x2, 0x540be400, 0, 0}, // 10000000000
    {2, 0, 0,       0x17, 0x4876e800, 0, 0}, // 100000000000
    {2, 0, 0,       0xe8, 0xd4a51000, 0, 0}, // 1000000000000
    {2, 0, 0,      0x918, 0x4e72a000, 0, 0}, // 10000000000000
    {2, 0, 0,     0x5af3, 0x107a4000, 0, 0}, // 100000000000000
    {2, 0, 0,    0x38d7e, 0xa4c68000, 0, 0}, // 1000000000000000
    {2, 0, 0,   0x2386f2, 0x6fc10000, 0, 0}, // 10000000000000000
    {2, 0, 0,  0x1634578, 0x5d8a0000, 0, 0}, // 100000000000000000
    {2, 0, 0,  0xde0b6b3, 0xa7640000, 0, 0}, // 1000000000000000000
    {2, 0, 0, 0x8ac72304, 0x89e80000, 0, 0}, // 10000000000000000000
    
    {3, 0, 0,          0x5, 0x6bc75e2d, 0x63100000, 0}, // 100000000000000000000
    {3, 0, 0,         0x36, 0x35c9adc5, 0xdea00000, 0}, // 1000000000000000000000
    {3, 0, 0,        0x21e, 0x19e0c9ba, 0xb2400000, 0}, // 10000000000000000000000
    {3, 0, 0,       0x152d, 0x02c7e14a, 0xf6800000, 0}, // 100000000000000000000000
    {3, 0, 0,       0xd3c2, 0x1bcecced, 0xa1000000, 0}, // 1000000000000000000000000
    {3, 0, 0,      0x84595, 0x16140148, 0x4a000000, 0}, // 10000000000000000000000000
    {3, 0, 0,     0x52b7d2, 0xdcc80cd2, 0xe4000000, 0}, // 100000000000000000000000000
    {3, 0, 0,    0x33b2e3c, 0x9fd0803c, 0xe8000000, 0}, // 1000000000000000000000000000
    {3, 0, 0,   0x204fce5e, 0x3e250261, 0x10000000, 0}, // 10000000000000000000000000000
    
    {4, 0, 0, 0x1, 0x431e0fae, 0x6d7217ca, 0xa0000000}, // 100000000000000000000000000000
    {4, 0, 0, 0xc, 0x9f2c9cd0, 0x4674edea, 0x40000000}, // 1000000000000000000000000000000
};
    
static const Decimal *kPow10Decimal[31] = {
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[0]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[1]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[2]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[3]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[4]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[5]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[6]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[7]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[8]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[9]),
    
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[10]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[11]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[12]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[13]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[14]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[15]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[16]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[17]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[18]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[19]),
    
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[20]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[21]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[22]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[23]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[24]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[25]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[26]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[27]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[28]),
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[29]),
    
    reinterpret_cast<const Decimal *>(kPow10DecimalStub[30]),
};
    
static const double kRadixDigitalPerBits[] = {
    0, // 0
    0, // 1
    1.0, // 2
    1.58496250072, // 3
    2.0, // 4
    2.32192809489, // 5
    2.58496250072, // 6
    2.80735492206, // 7
    3.0, // 8
    3.16992500144, // 9
    3.32192809489, // 10
    3.45943161864, // 11
    3.58496250072, // 12
    3.70043971814, // 13
    3.80735492206, // 14
    3.90689059561, // 15
    4.0, // 16
};
    
static const char kRadixDigitals[] = "0123456789abcdef";

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
            rv->set_segment(i, segment(i));
        }
        for (size_t i = segments_size(); i < new_len; ++i) {
            rv->set_segment(i, 0);
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
            set_segment(i, 0);
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
    rv->set_segment(0, 0);
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
    rv->set_segment(0, 0);
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
    Decimal *rv = NewUninitialized(segments_size() + rhs->segments_size(), arena);
    int64_t i = rv->segments_size();
    while (i-- > 0) {
        rv->set_segment(i, 0);
    }
    BasicMul(segment_view(), rhs->segment_view(), rv->segment_mut_view());
    rv->Normalize();
    rv->set_negative_and_exp(negative() != rhs->negative(), exp());
    return rv;
}

Decimal *Decimal::Div(const Decimal *rhs, base::Arena *arena) const {
    // TODO:
    return nullptr;
}

std::string Decimal::ToString(int radix) const {
    DCHECK_GE(radix, kMinRadix);
    DCHECK_LE(radix, kMaxRadix);
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
        uint32_t m = DivWord(p->segment_view(), radix, q->segment_mut_view());
        DCHECK_LT(m, radix);
        buf.insert(buf.begin(), kRadixDigitals[m]);
        ::memcpy(bufp.get(), bufq.get(), size);
    }
    
    if (exp() > 0) {
        DCHECK_EQ(10, radix);
        if (exp() >= buf.size()) {
            for (size_t i = 0; i < exp() - buf.size(); ++i) {
                buf.insert(0, "0");
            }
            buf.insert(0, "0.");
        } else {
            buf.insert(buf.size() - exp(), ".");
        }
        
        int postfix_zero = 0;
        int64_t i = buf.size();
        while (i--) {
            if (buf[i] == '0') {
                postfix_zero++;
            } else {
                break;
            }
        }
        if (postfix_zero > 0) {
            buf.erase(buf.size() - postfix_zero);
        }
        if (buf.back() == '.') {
            buf.erase(buf.size() - 1, 1);
        }
    }
    if (negative()) {
        buf.insert(buf.begin(), '-');
    }

    return buf;
}
    
int Decimal::Compare(const Decimal *rhs) const {
    if (negative() == rhs->negative()) {
        int rv = AbsCompare(this, rhs);
        return negative() ? -rv : rv;
    }
    return negative() ? -1 : 1;
}
    
/*static*/
Decimal *Decimal::NewParsed(const char *s, size_t n, base::Arena *arena) {
    Decimal *rv = nullptr;
    bool negative = false;
    
    int rn = base::Slice::LikeNumber(s, n);
    switch (rn) {
        case 'o': {
            negative = false;
            n--; // skip '0'
            s++;
            
            static const uint32_t scale = 8;
            
            size_t required_bits = GetNumberOfBits(n, 8);
            size_t required_segments = (required_bits + 31) / 32 + 1;
            rv = NewUninitialized(required_segments, arena);
            int64_t i = rv->segments_size();
            while (i-- > 0) { rv->set_segment(i, 0); }
            
            const size_t size = sizeof(Decimal) + rv->capacity_ * sizeof(uint32_t);
            
            std::unique_ptr<uint8_t[]> scoped_buf(new uint8_t[size]);
            ::memset(scoped_buf.get(), 0, size);
            Decimal *tmp = new (scoped_buf.get()) Decimal(rv->capacity_,
                                                          rv->offset_ + 1,
                                                          rv->flags_);
            for (i = 0; i < n; ++i) {
                tmp->Normalize();
                rv->Resize(tmp->segments_size() + 1);
                sql::BasicMul(tmp->segment_view(), MakeView(&scale, 1),
                              rv->segment_mut_view());
                //printf("+%s\n", rv->ToString().c_str());
                //const size_t low = rv->segments_size() - 1;
                //rv->set_segment(low, rv->segment(low) + s[i] - '0');
                
                const uint32_t elem = s[i] - '0';
                sql::Add(rv->segment_view(), MakeView(&elem, 1),
                         rv->segment_mut_view());
                
                //printf("-%s\n", rv->ToString().c_str());
                tmp->set_offset(rv->offset());
                ::memcpy(tmp->segments(), rv->segments(),
                         rv->segments_size() * sizeof(uint32_t));
                //printf("t%s\n", tmp->ToString().c_str());
            }
            rv->Normalize();
        } break;
            
        case 'd':
        case 's': {
            if (rn == 's') {
                negative = (s[0] == '-') ? true : false;
                n--;
                s++;
            } else {
                negative = false;
            }
        } break;
            
        case 'h': {
            negative = false;
            n -= 2; // skip '0x'
            s += 2;
            
            size_t required_bits = GetNumberOfBits(n, 16);
            size_t required_segments = (required_bits + 31) / 32;
            rv = NewUninitialized(required_segments, arena);
            int64_t i = n, bit = 0;
            while (i-- > 0) {
                char c = s[i];
                uint32_t n = 0;
                if (c >= '0' && c <= '9') {
                    n = c - '0';
                } else if (c >= 'a' && c <= 'f') {
                    n = 10 + (c - 'a');
                } else if (c >= 'A' && c <= 'F') {
                    n = 10 + (c - 'A');
                }
                size_t j = rv->segments_size() - (bit >> 5) - 1;
                if ((bit & 0x1f) == 0) {
                    rv->set_segment(j, 0);
                }
                rv->set_segment(j, rv->segment(j) | (n << (bit & 0x1f)));
                bit += 4;
            }
            rv->set_negative_and_exp(negative, 0);
        } break;
            
        case 'f': {
            if (s[0] == '-' || s[0] == '+') {
                negative = (s[0] == '-') ? true : false;
                n--;
                s++;
            } else {
                negative = false;
            }
//            dec = NewUninitialized(arena, n - 1);
//            if (!dec) {
//                return nullptr;
//            }
//            n_segment = dec->segments_size();
//            uint32_t segment = 0;
//            int r = 0;
//            size_t exp = 0, k = n_segment;
//            for (int64_t i = n - 1; i >= 0; --i) {
//                if (s[i] == '.') {
//                    exp = n - i - 1;
//                    continue;
//                }
//                segment += ((s[i] - '0') * kPowExp[r++]);
//                if (r >= 9) {
//                    dec->set_segment(--k, segment);
//                    segment = 0;
//                    r = 0;
//                }
//            }
//            if (r > 0) {
//                dec->set_segment(--k, segment);
//            }
//            dec->set_negative_and_exp(negative, static_cast<int>(exp));
        } break;
            
        default:
            break;
    }
    return rv;
}
    
/*static*/
int Decimal::AbsCompare(const Decimal *lhs, const Decimal *rhs) {
    DCHECK_EQ(lhs->exp(), rhs->exp());
    return sql::Compare(lhs->segment_view(), rhs->segment_view());
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
    
/*static*/ const Decimal *Decimal::GetFastPow10(int exp) {
    DCHECK_GE(exp, 0);
    DCHECK_LT(exp, kMaxExp);
    return kPow10Decimal[exp];
}

// m = n / log2(r)
/*static*/ size_t Decimal::GetNumberOfDigitals(size_t bits, int radix) {
    DCHECK_GE(radix, kMinRadix);
    DCHECK_LE(radix, kMaxRadix);
    return std::ceil(bits / kRadixDigitalPerBits[radix]);
}


// n = m * log2(r)
/*static*/ size_t Decimal::GetNumberOfBits(size_t digitals_count, int radix) {
    DCHECK_GE(radix, kMinRadix);
    DCHECK_LE(radix, kMaxRadix);
    return std::ceil(digitals_count * kRadixDigitalPerBits[radix]);
}
    
} // namespace v2

} // namespace sql

} // namespace mai
