#include "sql/decimal-v2.h"
#include "base/arena.h"
#include "base/bit-ops.h"

namespace mai {

namespace sql {
    
static int Compare(View<uint32_t> lhs, View<uint32_t> rhs) {
    int64_t n = std::max(lhs.n, rhs.n);
    for (int64_t i = 0; i < n; ++i) {
        
    }
    
    return 0;
}
    
static void DivWord(uint64_t n, uint64_t d, uint32_t rv[2]) {
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
    
static uint32_t DivWord(View<uint32_t> lhs, uint32_t rhs, MutView<uint32_t> rv) {
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
   
Decimal *Decimal::Shl(int bits, base::Arena *arena) const {
    // TODO:
    return nullptr;
}

Decimal *Decimal::Shr(int bits, base::Arena *arena) const {
    // TODO:
    return nullptr;
}

Decimal *Decimal::Add(const Decimal *rhs, base::Arena *arena) const {
    // TODO:
    return nullptr;
}

Decimal *Decimal::Sub(const Decimal *rhs, base::Arena *arena) const {
    // TODO:
    return nullptr;
}

Decimal *Decimal::Mul(const Decimal *rhs, base::Arena *arena) const {
    // TODO:
    return nullptr;
}

Decimal *Decimal::Div(const Decimal *rhs, base::Arena *arena) const {
    // TODO:
    return nullptr;
}
    
void Decimal::PrimitiveShl(int bits) {
    // TODO:
}

void Decimal::PrimitiveShr(int bits) {
    // TODO:
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
