#include "core/decimal-v2.h"
#include "base/arenas.h"
#include "base/arena.h"
#include "base/slice.h"
#include "base/bit-ops.h"
#include <cmath>

namespace mai {

namespace core {
    
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

uint32_t MulSub(MutView<uint32_t> q, MutView<uint32_t> a, uint32_t x,
                int64_t offset) {
    uint64_t xl = static_cast<uint64_t>(x), carry = 0;
    offset += a.n;
    
    for (int64_t j = a.n - 1; j >= 0; j--) {
        DCHECK_LT(j, a.n);
        uint64_t product = static_cast<uint64_t>(a.z[j]) * xl + carry;
        
        DCHECK_LT(offset, q.n);
        uint64_t diff = q.z[offset] - product;
        
        DCHECK_GE(offset, 0);
        q.z[offset--] = static_cast<uint32_t>(diff);
        carry = (product >> 32) +
            (static_cast<int64_t>(diff) > static_cast<int64_t>(~static_cast<uint32_t>(product)) ? 1 : 0);
    }
    return static_cast<uint32_t>(carry);
}

uint32_t DivAdd(MutView<uint32_t> a, MutView<uint32_t> r, size_t offset) {
    uint64_t carry = 0;
    
    for (int64_t j = a.n - 1; j >= 0; j--) {
        DCHECK_LT(j, a.n);
        DCHECK_LT(j + offset, r.n);
        uint64_t sum = static_cast<uint64_t>(a.z[j]) +
                       static_cast<uint64_t>(r.z[j + offset]) + carry;
        r.z[j + offset] = static_cast<uint32_t>(sum);
        carry = sum >> 32;
    }

    return static_cast<uint32_t>(carry);
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
    
static inline uint32_t Char2Digital(int c) {
    uint32_t n = 0;
    if (c >= '0' && c <= '9') {
        n = c - '0';
    } else if (c >= 'a' && c <= 'f') {
        n = 10 + (c - 'a');
    } else if (c >= 'A' && c <= 'F') {
        n = 10 + (c - 'A');
    } else {
        DLOG(FATAL) << "Noreached!";
    }
    return n;
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
    
static const double kFloatingExps[] = {
       1,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,
    1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19,
    1e20, 1e21, 1e22, 1e23, 1e24, 1e25, 1e26, 1e27, 1e28, 1e29,
    1e30,
};

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
        core::PrimitiveShl(segment_mut_view(), n);
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
//        for (int64_t i = new_len - 1; i >= 0; --i) {
//            base()[new_off + i] = segment(i);
//        }
        ::memmove(base() + new_off, base() + offset_,
                  segments_size() * sizeof(uint32_t));
        offset_ = new_off;
    } else {
        uint32_t new_off  = static_cast<uint32_t>(capacity_ - new_len);
//        for (int64_t i = segments_size() - 1; i >= 0; --i) {
//            base()[new_off + i] = segment(i);
//        }
        ::memmove(base() + new_off, base() + offset_,
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
        core::PrimitiveShl(rv->segment_mut_view(), n_bits);
    } else {
        core::PrimitiveShr(rv->segment_mut_view(), 32 - n_bits);
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
        core::PrimitiveShl(segment_mut_view(), 32 - n_bits);
        ::memmove(segments() + 1, segments(),
                  (segments_size() - 1) * sizeof(uint32_t));
        offset_++;
    } else {
        core::PrimitiveShr(segment_mut_view(), n_bits);
    }
    return this;
}
    
Decimal *Decimal::Add(const Decimal *rhs) {
    DCHECK_EQ(exp(), rhs->exp());
    DCHECK_LE(rhs->segments_size(), capacity_);
    size_t n_ints = std::max(segments_size(), rhs->segments_size()) + 1;
    if (n_ints <= capacity_) {
        size_t d = n_ints - segments_size();
        Resize(n_ints);
        for (size_t i = 0; i < d; ++i) {
            set_segment(i, 0);
        }
    }
    AddRaw(this, rhs, this);
    return this;
}
    
Decimal *Decimal::Add(uint64_t val) {
    base::ScopedArena scoped_buf;
    Decimal *rhs = NewU64(val, &scoped_buf);
    rhs = rhs->NewPrecision(exp(), &scoped_buf);
    AddRaw(this, DCHECK_NOTNULL(rhs), this);
    return this;
}

Decimal *Decimal::Add(int64_t val) {
    base::ScopedArena scoped_buf;
    Decimal *rhs = NewI64(val, &scoped_buf);
    rhs = rhs->NewPrecision(exp(), &scoped_buf);
    AddRaw(this, DCHECK_NOTNULL(rhs), this);
    return this;
}

Decimal *Decimal::Add(const Decimal *rhs, base::Arena *arena) const {
    DCHECK_EQ(exp(), rhs->exp());
    
    size_t n_ints = std::max(segments_size(), rhs->segments_size()) + 1;
    Decimal *rv = NewUninitialized(n_ints, arena);
    rv->set_segment(0, 0);
    AddRaw(this, rhs, rv);
    return rv;
}

Decimal *Decimal::Sub(const Decimal *rhs, base::Arena *arena) const {
    DCHECK_EQ(exp(), rhs->exp());
    
    size_t n_ints = std::max(segments_size(), rhs->segments_size()) + 1;
    Decimal *rv = NewUninitialized(n_ints, arena);
    rv->set_segment(0, 0);
    bool neg = negative();
    if (negative() != rhs->negative()) {
        // -lhs -   rhs  = -(lhs + rhs)
        //  lhs - (-rhs) =   lhs + rhs
        core::Add(segment_view(), rhs->segment_view(), rv->segment_mut_view());
    } else {
        //   lhs  -   rhs  = lhs - rhs = -(rhs - lhs)
        // (-lhs) - (-rhs) = rhs - lhs = -(lhs - rhs)
        if (AbsCompare(this, rhs) >= 0) {
            core::Sub(segment_view(), rhs->segment_view(), rv->segment_mut_view());
        } else {
            neg = !neg;
            core::Sub(rhs->segment_view(), segment_view(), rv->segment_mut_view());
        }
    }
    rv->set_negative_and_exp(neg, exp());
    rv->Normalize();
    return rv;
}

// 010.01 : 100
// 000.10 : 100
// 001.00 : t
// 100.10 : f
// ------------
// 010.00 : 100
// 001.00 : 100
// 000.10 : t
// 010.00 : f
//-------------
// 010.01  : 100
// 000.100 : 1000
// 000.100 : t
// 100.100 : f
Decimal *Decimal::Mul(const Decimal *rhs, base::Arena *arena) const {
    DCHECK_EQ(exp(), rhs->exp());
    
    Decimal *rv = NewUninitialized(segments_size() + rhs->segments_size(), arena);
    rv->Fill(0);
    BasicMul(segment_view(), rhs->segment_view(), rv->segment_mut_view());
    rv->Normalize();
    if (exp() > 0) {
        Decimal *scaled = nullptr;
        std::tie(scaled, std::ignore) = DivRaw(rv, scale(), arena);
        scaled->set_negative_and_exp(negative() != rhs->negative(), exp());
        return scaled;
    }
    
    rv->set_negative_and_exp(negative() != rhs->negative(), exp());
    return rv;
}
    
Decimal *Decimal::Div(uint64_t val) {
    base::ScopedArena scoped_buf;
    const Decimal *rhs = NewU64(val, &scoped_buf);
    rhs = rhs->NewPrecision(exp(), &scoped_buf);
    return Div(rhs);
}

Decimal *Decimal::Div(int64_t val) {
    base::ScopedArena scoped_buf;
    const Decimal *rhs = NewI64(val, &scoped_buf);
    rhs = rhs->NewPrecision(exp(), &scoped_buf);
    return Div(rhs);
}

Decimal *Decimal::Div(const Decimal *rhs) {
    base::ScopedArena scoped_buf;
    const Decimal *rv = nullptr;
    std::tie(rv, std::ignore) = Div(rhs, &scoped_buf);
    return CopyFrom(rv);
}

std::tuple<Decimal *, Decimal *> Decimal::Div(const Decimal *rhs, base::Arena *arena) const {
    DCHECK_EQ(exp(), rhs->exp());
    if (rhs->zero()) {
        return {nullptr, nullptr};
    }

    if (zero()) {
        return {NewI64(0, arena), NewI64(0, arena)};
    }

    int cmp = AbsCompare(this, rhs);
    if (cmp < 0 && exp() == 0 && rhs->exp() == 0) {
        return {NewI64(0, arena), NewCopied(this, arena)};
    }
    
    if (cmp == 0) {
        return (negative() == rhs->negative())
        ? std::make_tuple(NewI64( 1, arena), NewI64(0, arena))
        : std::make_tuple(NewI64(-1, arena), NewI64(0, arena));
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
    const Decimal *lhs = this;
    base::ScopedArena scoped_buf;
    if (exp() > 0) {
        const Decimal *s = scale();
        Decimal *scaled = NewUninitialized(segments_size() + s->segments_size(),
                                           &scoped_buf);
        scaled->Fill(0);
        core::BasicMul(segment_view(), s->segment_view(),
                      scaled->segment_mut_view());
        lhs = scaled;
    }

    Decimal *rv = nullptr, *re = nullptr;
    std::tie(rv, re) = DivRaw(lhs, rhs, arena);

    rv->Normalize();
    rv->set_negative_and_exp(negative() != negative(), exp());
    re->Normalize();
    re->set_negative_and_exp(rv->negative(), rv->exp());
    return {rv, re};
}
    
Decimal *Decimal::Sqrt(base::Arena *arena) const {
    if (negative()) {
        return nullptr;
    }
    base::ScopedArena scoped_buf;
    Decimal *y = Decimal::NewI64(1, &scoped_buf);
    y = y->NewPrecision(exp(), &scoped_buf);
    
    const Decimal *n = this;
    // if exp is odd number
    if (n->exp() & 0x1 /*n->exp() % 2 == 1*/) {
        Decimal *tmp = NewUninitialized(segments_size() + 1, &scoped_buf);
        tmp->Fill();
        core::BasicMul(n->segment_view(), GetFastPow10(1)->segment_view(),
                      tmp->segment_mut_view());
        tmp->set_exp(exp() + 1);
        n = tmp;
    }
    
    y->set_exp(0);
    for (int i = 0; i < 1024; ++i) {
        Decimal *t1;
        std::tie(t1, std::ignore) = DivRaw(n, y, &scoped_buf);
        Decimal *t2 = y->Add(t1, &scoped_buf);

        Decimal *x = Decimal::NewUninitialized(t2->segments_size(), &scoped_buf);
        core::DivWord(t2->segment_view(), 2, x->segment_mut_view());

        if (AbsCompare(x, y) == 0) {
            //y = x;
            break;
        }
        y = x;
    }
    // sqrt(81) = 9
    // 8.1
    // 8.10 = 0.28
    // ------------------
    // 8.100
    // 8.1000 = 0.2840
    if (n != this) {
        y->set_exp(n->exp() / 2);
        return y->Clone(arena);
    }
    y->set_exp(exp() / 2); // restore exp
    return y->Clone(arena);
}
    
void Decimal::Normalize() {
    for (size_t i = offset_; i < capacity_; ++i) {
        if (base()[i] != 0) {
            set_offset(i);
            return;
        }
    }
    set_offset(capacity_/* - 1*/);
}

std::string Decimal::ToString(int radix) const {
    DCHECK_GE(radix, kMinRadix);
    DCHECK_LE(radix, kMaxRadix);
    if (zero()) {
        return "0";
    }

    base::ScopedArena scoped_buf;
    Decimal *q = NewUninitialized(capacity_, &scoped_buf);
    q->offset_ = offset_;
    q->flags_  = flags_;
    
    Decimal *p = Clone(&scoped_buf);
    
    std::string buf;
    while (!p->zero()) {
        uint32_t m = DivWord(p->segment_view(), radix, q->segment_mut_view());
        DCHECK_LT(m, radix);
        buf.insert(buf.begin(), kRadixDigitals[m]);
        ::memcpy(p->segments(), q->segments(),
                 segments_size() * sizeof(uint32_t));
    }

    if (exp() > 0) {
        DCHECK_EQ(10, radix);
        if (exp() >= buf.size()) {
            while (exp() > buf.size()) {
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
    
int64_t Decimal::ToI64() const {
    const Decimal *d = this;
    base::ScopedArena arena;
    if (exp() > 0) {
        d = Shrink(0, &arena);
    }

    int64_t val = 0;
    if (d->zero()) {
        val = 0;
    } else if (d->segments_size() < 2) {
        val = static_cast<uint64_t>(d->segment(0));
    } else { // (d->segments_size() >= 2)
        val |= static_cast<uint64_t>(d->segment(1) & 0x7fffffff) << 32;
        val |= static_cast<uint64_t>(d->segment(0));
    }

    return val * sign();
}
    
uint64_t Decimal::ToU64() const {
    const Decimal *d = this;
    base::ScopedArena arena;
    if (exp() > 0) {
        d = Shrink(0, &arena);
    }
    
    uint64_t val = 0;
    if (d->zero()) {
        val = 0;
    } else if (d->segments_size() < 2) {
        val = static_cast<uint64_t>(d->segment(0));
    } else { // (d->segments_size() >= 2)
        val |= static_cast<uint64_t>(d->segment(1) & 0xffffffff) << 32;
        val |= static_cast<uint64_t>(d->segment(0));
    }
    return negative() ? -val : val;
}

double Decimal::ToF64() const {
    if (zero()) {
        return 0;
    }

    base::ScopedArena scoped_buf;
    Decimal *q = NewUninitialized(capacity_, &scoped_buf);
    q->offset_ = offset_;
    q->flags_  = flags_;
    
    Decimal *p = Clone(&scoped_buf);
    
    double rv = 0;
    int i = 0;
    while (!p->zero()) {
        uint32_t m = DivWord(p->segment_view(), 10, q->segment_mut_view());
        DCHECK_LT(m, 10);
        if (i < exp()) {
            rv += (1.0f / kFloatingExps[exp() - i]) * m;
        } else { // i >= exp()
            rv += kFloatingExps[i - exp()] * m;
        }
        ::memcpy(p->segments(), q->segments(),
                 segments_size() * sizeof(uint32_t));
        ++i;
    }
    return rv * sign();
}

// 1.00
// 1.0000
Decimal *Decimal::Extend(int new_exp, base::Arena *arena) const {
    DCHECK_LE(new_exp, kMaxExp);
    if (exp() >= new_exp) {
        return const_cast<Decimal *>(this);
    }
    int delta = new_exp - exp();
    DCHECK_LE(delta, kMaxExp);
    const Decimal *scale = GetFastPow10(delta);
    Decimal *rv = NewUninitialized(segments_size() + scale->segments_size(),
                                   arena);
    rv->Fill();
    core::BasicMul(segment_view(), scale->segment_view(), rv->segment_mut_view());
    rv->set_negative_and_exp(negative(), new_exp);
    rv->Normalize();
    return rv;
}
    
Decimal *Decimal::Shrink(int new_exp, base::Arena *arena) const {
    DCHECK_LE(new_exp, kMaxExp);
    if (exp() <= new_exp) {
        return const_cast<Decimal *>(this);
    }
    int delta = exp() - new_exp;
    DCHECK_LE(delta, kMaxExp);
    const Decimal *scale = GetFastPow10(delta);
    
    Decimal *rv;
    std::tie(rv, std::ignore) = DivRaw(this, scale, arena);
    rv->set_negative_and_exp(negative(), new_exp);
    rv->Normalize();
    return rv;
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
    int rv = base::Slice::LikeNumber(s, n);
    switch (rv) {
        case 'o':
            return NewOctLiteral(s, n, arena);
        case 'd':
        case 's':
            return NewDecLiteral(s, n, arena);
        case 'h':
            return NewHexLiteral(s, n, arena);
        case 'f':
            return NewPointLiteral(s, n, arena);
        case 'e':
            return NewExpLiteral(s, n, arena);
        default:
            break;
    }
    return nullptr;
}
    
/*static*/
Decimal *Decimal::NewOctLiteral(const char *s, size_t n, base::Arena *arena) {
    DCHECK_EQ('o', base::Slice::LikeNumber(s, n));

    bool negative = false;
    n--; // skip '0'
    s++;
    Decimal *rv = ParseDigitals(s, n, 8, arena);
    if (rv) {
        DCHECK_EQ(0, rv->exp());
        rv->set_negative(negative);
    }
    return rv;
}

/*static*/
Decimal *Decimal::NewDecLiteral(const char *s, size_t n, base::Arena *arena) {
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
    Decimal *rv = ParseDigitals(s, n, 10, arena);
    if (rv) {
        DCHECK_EQ(0, rv->exp());
        rv->set_negative(negative);
    }
    return rv;
}

/*static*/
Decimal *Decimal::NewHexLiteral(const char *s, size_t n, base::Arena *arena) {
    DCHECK_EQ('h', base::Slice::LikeNumber(s, n));
    
    bool negative = false;
    n -= 2; // skip '0x'
    s += 2;

    size_t required_bits = GetNumberOfBits(n, 16);
    size_t required_segments = (required_bits + 31) / 32;
    Decimal *rv = NewUninitialized(required_segments, arena);
    int64_t i = n, bit = 0;
    while (i-- > 0) {
        uint32_t n = core::Char2Digital(s[i]);
        size_t j = rv->segments_size() - (bit >> 5) - 1;
        if ((bit & 0x1f) == 0) {
            rv->set_segment(j, 0);
        }
        rv->set_segment(j, rv->segment(j) | (n << (bit & 0x1f)));
        bit += 4;
    }
    rv->set_negative_and_exp(negative, 0);
    return rv;
}

/*static*/
Decimal *Decimal::NewPointLiteral(const char *s, size_t n, base::Arena *arena) {
    DCHECK_EQ('f', base::Slice::LikeNumber(s, n));
    
    bool negative = false;
    if (s[0] == '-' || s[0] == '+') {
        negative = (s[0] == '-') ? true : false;
        n--; // skip '-' / '+'
        s++;
    } else {
        negative = false;
    }
    Decimal *rv = ParseDigitals(s, n, 10, arena);
    if (rv) {
        rv->set_negative(negative);
    }
    return rv;
}

/*static*/
Decimal *Decimal::NewExpLiteral(const char *s, size_t n, base::Arena *arena) {
    DCHECK_EQ('e', base::Slice::LikeNumber(s, n));
    
    bool negative = false;
    if (s[0] == '-' || s[0] == '+') {
        negative = (s[0] == '-');
        n--; // skip '-' / '+'
        s++;
    } else {
        negative = false;
    }
    size_t k = 0;
    for (size_t i = 0; i < n; ++i) {
        if (s[i] == 'e' || s[i] == 'E') {
            k = i;
            break;
        }
    }
    DCHECK_LT(k, n);
    Decimal *rv = ParseDigitals(s, k, 10, arena);
    // 1.085e-10 (5) (9)
    int e = 0;
    if (base::Slice::ParseI32(s + k + 1, n - k - 1, &e) != 0) {
        return nullptr;
    }
    if (e < 0) {
        if (-e + rv->exp() > kMaxExp) {
            return nullptr;
        }
        rv->set_negative_and_exp(negative, -e + rv->exp());
    } else if (e > 0) {
        // 1.075e7
        // 1 0750000.000
        // --------------
        // 1.075e2
        // 107.500
        if (e > kMaxExp) {
            return nullptr;
        }
        //printf("%s\n", rv->ToString().c_str());
        const Decimal *s = GetFastPow10(e);
        Decimal *scaled = NewUninitialized(
                                           rv->segments_size() + s->segments_size(), arena);
        core::BasicMul(rv->segment_view(), s->segment_view(),
                       scaled->segment_mut_view());
        scaled->set_negative_and_exp(negative, rv->exp());
        rv = scaled;
    } else {
        rv->set_negative(negative);
    }
    return rv;
}
    
/*static*/ Decimal *Decimal::ParseDigitals(const char *s, size_t n, int radix,
                                           base::Arena *arena) {
    DCHECK_GE(radix, kMinRadix);
    DCHECK_LE(radix, kMaxRadix);

    size_t required_bits = GetNumberOfBits(n, radix);
    size_t required_segments = (required_bits + 31) / 32 + 1;
    Decimal *rv = NewUninitialized(required_segments, arena);
    rv->Fill();

    base::ScopedArena scoped_buf;
    Decimal *tmp = NewUninitialized(rv->capacity_, &scoped_buf);
    tmp->offset_ = rv->offset_ + 1;
    tmp->flags_  = rv->flags_;
    tmp->Fill();

    int exp = 0;
    const uint32_t scale = radix;
    for (size_t i = 0; i < n; ++i) {
        if (s[i] == '.') {
            DCHECK_EQ(10, radix);
            exp = static_cast<int>(n - i - 1);
            continue;
        }

        tmp->Normalize();
        rv->Resize(tmp->segments_size() + 1);
        core::BasicMul(tmp->segment_view(), MakeView(&scale, 1),
                      rv->segment_mut_view());
    
        const uint32_t elem = core::Char2Digital(s[i]);
        core::Add(rv->segment_view(), MakeView(&elem, 1),
                 rv->segment_mut_view());
        
        tmp->set_offset(rv->offset());
        ::memcpy(tmp->segments(), rv->segments(),
                 rv->segments_size() * sizeof(uint32_t));
    }
    rv->set_exp(exp);
    rv->Normalize();
    return rv;
}
    
/*static*/
int Decimal::AbsCompare(const Decimal *lhs, const Decimal *rhs) {
    DCHECK_EQ(lhs->exp(), rhs->exp());
    return core::Compare(lhs->segment_view(), rhs->segment_view());
}
    
/*static*/ Decimal *Decimal::NewP64(uint64_t val, bool neg, int exp, size_t reserved,
                                    base::Arena *arena) {
    Decimal *rv = NewUninitialized(reserved, arena);
    uint32_t hi_bits = static_cast<uint32_t>((val & 0xffffffff00000000ull) >> 32);
    if (hi_bits) {
        rv->Resize(2);
        rv->segments()[0] = hi_bits;
        rv->segments()[1] = static_cast<uint32_t>(val);
    } else {
        if (val) {
            rv->Resize(1);
            rv->segments()[0] = static_cast<uint32_t>(val);
        } else {
            rv->Resize(0);
        }
    }
    rv->set_negative_and_exp(neg, exp);
    return rv;
}
    
/*static*/ Decimal *Decimal::NewCopied(const uint32_t *s, size_t n,
                                       base::Arena *arena) {
    Decimal *rv = NewUninitialized(n, arena);
    ::memcpy(rv->segments(), s, n * sizeof(uint32_t));
    rv->set_negative_and_exp(false, 0);
    return rv;
}

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
    
/*static*/ std::tuple<Decimal *, Decimal *>
Decimal::DivRaw(const Decimal *lhs, const Decimal *rhs, base::Arena *arena) {
    Decimal *rv, *re;
    if (rhs->segments_size() == 1) {
        rv = NewUninitialized(lhs->segments_size(), arena);
        auto re_val = core::DivWord(lhs->segment_view(), rhs->segment(0),
                                   rv->segment_mut_view());
        re = NewU64(re_val, arena);
        
    } else {
        DCHECK_GE(lhs->segments_size(), rhs->segments_size());
        //size_t limit = lhs->segments_size() - rhs->segments_size() + 1;
        //const size_t nlen = lhs->segments_size() + 1;
        const size_t limit = (lhs->segments_size() + 1) - rhs->segments_size() + 1;
        rv = NewUninitialized(limit, arena);
        
        std::unique_ptr<uint32_t[]> scoped_divisor(new uint32_t[rhs->segments_size()]);
        ::memcpy(scoped_divisor.get(), rhs->segments(),
                 rhs->segments_size() * sizeof(uint32_t));
        re = lhs->DivMagnitude(MakeMutView(scoped_divisor.get(),
                                           rhs->segments_size()), rv, arena);
    }
    return {rv, re};
}
    
/*static*/ void Decimal::AddRaw(const Decimal *lhs, const Decimal *rhs, Decimal *rv) {
    bool neg = lhs->negative();
    if (lhs->negative() == rhs->negative()) {
        //   lhs  +   rhs  =   lhs + rhs
        // (-lhs) + (-rhs) = -(lhs + rhs)
        core::Add(lhs->segment_view(), rhs->segment_view(), rv->segment_mut_view());
    } else {
        //   lhs  + (-rhs) = lhs - rhs = -(rhs - lhs)
        // (-lhs) +   rhs  = rhs - lhs = -(lhs - rhs)
        if (AbsCompare(lhs, rhs) >= 0) {
            core::Sub(lhs->segment_view(), rhs->segment_view(), rv->segment_mut_view());
        } else {
            neg = !neg;
            core::Sub(rhs->segment_view(), lhs->segment_view(), rv->segment_mut_view());
        }
    }
    rv->set_negative_and_exp(neg, lhs->exp());
    rv->Normalize();
}
    
Decimal *Decimal::DivMagnitude(MutView<uint32_t> divisor, Decimal *rv,
                               base::Arena *arena) const {
    // Remainder starts as dividend with space for a leading zero
    Decimal *re = NewUninitialized(segments_size() + 1, arena);
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
        core::PrimitiveShl(divisor, shift);
        // But this one might
        re = re->Shl(shift, arena);
    }

    // Must insert leading 0 in rem if its length did not change
    if (re->segments_size() == nlen) {
        Decimal *tmp = NewUninitialized(re->segments_size() + 1, arena);
        ::memcpy(tmp->segments() + 1, re->segments(),
                 re->segments_size() * sizeof(uint32_t));
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
                core::DivWord(chunk, dh, qword);
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
        uint32_t borrow = core::MulSub(re->segment_mut_view(), divisor , qhat, j);
        
        // D5 Test remainder
        if (borrow + 0x80000000u > nh2) {
            // D6 Add back
            DivAdd(divisor, re->segment_mut_view(), j + 1);
            qhat--;
        }
        
        // Store the quotient digit
        rv->set_segment(j, qhat);
    } // D7 loop on j

    // D8 Unnormalize
    if (shift > 0) {
        re = re->Shr(shift, arena);
    }
    
    rv->Normalize();
    re->Normalize();
    return re;
}
    
} // namespace v2

} // namespace core

} // namespace mai
