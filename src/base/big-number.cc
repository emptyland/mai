#include "base/big-number.h"
#include "base/bit-ops.h"
#include "glog/logging.h"
#include <cmath>

namespace mai {
    
namespace base {
    
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
    
int Big::Compare(View<uint32_t> lhs, View<uint32_t> rhs) {
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

void Big::PrimitiveShl(MutView<uint32_t> vals, int n) {
    int shift = 32 - n;
    uint32_t c = vals.z[0];
    for (size_t i = 0; i < vals.n - 1; ++i) {
        uint32_t b = c;
        c = vals.z[i + 1];
        vals.z[i] = (b << n) | (c >> shift);
    }
    vals.z[vals.n - 1] <<= n;
}

void Big::PrimitiveShr(MutView<uint32_t> vals, int n) {
    int shift = 32 - n;
    uint32_t c = vals.z[vals.n - 1];
    for (size_t i = vals.n - 1; i > 0; i--) {
        uint32_t b = c;
        c = vals.z[i - 1];
        vals.z[i] = (c << shift) | (b >> n);
    }
    vals.z[0] >>= n;
}

uint32_t Big::Add(View<uint32_t> lhs, View<uint32_t> rhs, MutView<uint32_t> rv) {
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

uint32_t Big::Sub(View<uint32_t> lhs, View<uint32_t> rhs, MutView<uint32_t> rv) {
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

uint32_t Big::BasicMul(View<uint32_t> x, View<uint32_t> y, MutView<uint32_t> rv) {
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

void Big::DivWord(uint64_t n, uint64_t d, uint32_t rv[2]) {
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

uint32_t Big::MulSub(MutView<uint32_t> q, MutView<uint32_t> a, uint32_t x,
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

uint32_t Big::DivAdd(MutView<uint32_t> a, MutView<uint32_t> r, size_t offset) {
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

/*static*/ uint32_t Big::DivWord(View<uint32_t> lhs, uint32_t rhs, MutView<uint32_t> rv) {
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
    
// m = n / log2(r)
/*static*/ size_t Big::GetNumberOfDigitals(size_t bits, int radix) {
    DCHECK_GE(radix, kMinRadix);
    DCHECK_LE(radix, kMaxRadix);
    return std::ceil(bits / kRadixDigitalPerBits[radix]);
}


// n = m * log2(r)
/*static*/ size_t Big::GetNumberOfBits(size_t digitals_count, int radix) {
    DCHECK_GE(radix, kMinRadix);
    DCHECK_LE(radix, kMaxRadix);
    return std::ceil(digitals_count * kRadixDigitalPerBits[radix]);
}
    
/*static*/ uint32_t Big::Char2Digital(int c) {
    uint32_t n = 0;
    if (c >= '0' && c <= '9') {
        n = c - '0';
    } else if (c >= 'a' && c <= 'f') {
        n = 10 + (c - 'a');
    } else if (c >= 'A' && c <= 'F') {
        n = 10 + (c - 'A');
    } else {
        NOREACHED();
    }
    return n;
}

} // namespace base
    
} // namespace mai
