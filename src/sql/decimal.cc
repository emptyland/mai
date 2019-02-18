#include "sql/decimal.h"
#include "base/slice.h"
#include "base/bit-ops.h"

namespace mai {

namespace sql {
    
#define DEF_CONST_DECIMAL_STUB_HEADER(neg, seg) \
    0, 0, 0, 0, \
    0, static_cast<uint8_t>(Decimal::kHeaderSize + (seg) * 4), 0, 0, \
    ((neg) ? 0 : 0x80), 0, 0, 0, \

#define DEF_CONST_DECIMAL_STUB(neg, seg, val) \
    DEF_CONST_DECIMAL_STUB_HEADER(neg, seg) \
    0, 0, 0, val,
    
static
uint8_t const_decimal_stub[Decimal::kMaxConstVal - Decimal::kMinConstVal + 1]
                          [sizeof(Decimal) + Decimal::kHeaderSize + 4] = {
    {DEF_CONST_DECIMAL_STUB(true,  1, 10)},
    {DEF_CONST_DECIMAL_STUB(true,  1, 9)},
    {DEF_CONST_DECIMAL_STUB(true,  1, 8)},
    {DEF_CONST_DECIMAL_STUB(true,  1, 7)},
    {DEF_CONST_DECIMAL_STUB(true,  1, 6)},
    {DEF_CONST_DECIMAL_STUB(true,  1, 5)},
    {DEF_CONST_DECIMAL_STUB(true,  1, 4)},
    {DEF_CONST_DECIMAL_STUB(true,  1, 3)},
    {DEF_CONST_DECIMAL_STUB(true,  1, 2)},
    {DEF_CONST_DECIMAL_STUB(true,  1, 1)},
    {DEF_CONST_DECIMAL_STUB(false, 0, 0)},
    {DEF_CONST_DECIMAL_STUB(false, 1, 1)},
    {DEF_CONST_DECIMAL_STUB(false, 1, 2)},
    {DEF_CONST_DECIMAL_STUB(false, 1, 3)},
    {DEF_CONST_DECIMAL_STUB(false, 1, 4)},
    {DEF_CONST_DECIMAL_STUB(false, 1, 5)},
    {DEF_CONST_DECIMAL_STUB(false, 1, 6)},
    {DEF_CONST_DECIMAL_STUB(false, 1, 7)},
    {DEF_CONST_DECIMAL_STUB(false, 1, 8)},
    {DEF_CONST_DECIMAL_STUB(false, 1, 9)},
    {DEF_CONST_DECIMAL_STUB(false, 1, 10)},
};
    
/*static*/
Decimal *const Decimal::kZero = Decimal::kConstants[kConstZeroIdx];
    
/*static*/
Decimal *const Decimal::kOne = Decimal::kConstants[kConstZeroIdx + 1];

/*static*/
Decimal *const Decimal::kMinusOne = Decimal::kConstants[kConstZeroIdx - 1];
    
/*static*/ Decimal *const Decimal::kConstants[kMaxConstVal - kMinConstVal + 1] = {
    reinterpret_cast<Decimal *>(const_decimal_stub[0]),
    reinterpret_cast<Decimal *>(const_decimal_stub[1]),
    reinterpret_cast<Decimal *>(const_decimal_stub[2]),
    reinterpret_cast<Decimal *>(const_decimal_stub[3]),
    reinterpret_cast<Decimal *>(const_decimal_stub[4]),
    reinterpret_cast<Decimal *>(const_decimal_stub[5]),
    reinterpret_cast<Decimal *>(const_decimal_stub[6]),
    reinterpret_cast<Decimal *>(const_decimal_stub[7]),
    reinterpret_cast<Decimal *>(const_decimal_stub[8]),
    reinterpret_cast<Decimal *>(const_decimal_stub[9]),
    reinterpret_cast<Decimal *>(const_decimal_stub[10]),
    reinterpret_cast<Decimal *>(const_decimal_stub[11]),
    reinterpret_cast<Decimal *>(const_decimal_stub[12]),
    reinterpret_cast<Decimal *>(const_decimal_stub[13]),
    reinterpret_cast<Decimal *>(const_decimal_stub[14]),
    reinterpret_cast<Decimal *>(const_decimal_stub[15]),
    reinterpret_cast<Decimal *>(const_decimal_stub[16]),
    reinterpret_cast<Decimal *>(const_decimal_stub[17]),
    reinterpret_cast<Decimal *>(const_decimal_stub[18]),
    reinterpret_cast<Decimal *>(const_decimal_stub[19]),
    reinterpret_cast<Decimal *>(const_decimal_stub[20]),
};
    
/*static*/
//const int Decimal::kHeaderSize = 4;

/*static*/ const uint32_t Decimal::kPowExp[9] = {
    1, // 1
    10, // 2
    100, // 3
    1000, // 4
    10000, // 5
    100000, // 6
    1000000, // 7
    10000000, // 8
    100000000, // 9
};
    
size_t Decimal::highest_digital() const {
    size_t ridx = digitals_size();
    for (size_t i = 0; i < segments_size(); ++i) {
        uint32_t s = segment(i);
        ridx -= 9;
        if (s != 0) {
            int64_t j = 9;
            while (j-- > 0) {
                if (s >= kPowExp[j]) {
                    return ridx + j;
                }
            }
        }
    }
    return digitals_size();
}
    
Decimal *Decimal::Shl(uint32_t n, base::Arena *arena) const {
    if (zero()) {
        return const_cast<Decimal *>(this);
    }
    Decimal *rv = const_cast<Decimal *>(this);
    
    int hi_word_bits = kMaxSegmentBits - base::Bits::CountLeadingZeros32(segment(0));
    if (n <= kMaxSegmentBits - hi_word_bits) {
        if (IsConstant()) {
            rv = NewWithRawData(arena, data(), size());
        }
        rv->PrimitiveShl(n);
        return rv;
    }
    
    size_t n_segments = n / kMaxSegmentBits;
    uint32_t n_bits = n % kMaxSegmentBits;
    size_t new_len = segments_size() + n_segments + 1;
    
    if (n_bits <= kMaxSegmentBits - hi_word_bits) {
        new_len--;
    }
    rv = NewUninitialized(arena, new_len * 9);
    rv->set_negative_and_exp(negative(), exp());
    //       100000000000
    // 000000100000000000
    for (size_t i = 0; i < segments_size(); ++i) {
        rv->set_segment(new_len - i - 1, segment(segments_size() - i - 1));
    }
    for (size_t i = segments_size(); i < new_len; ++i) {
        rv->set_segment(new_len - i - 1, 0);
    }
    
    rv->PrimitiveShl(n);
    return rv;
}

Decimal *Decimal::Shr(uint32_t n, base::Arena *arena) const {
    // FIXME: 
    Decimal *rv = const_cast<Decimal *>(this);
    if (IsConstant()) {
        rv = NewWithRawData(arena, data(), size());
    }
    rv->PrimitiveShr(n);
    return rv;
}
    
Decimal *Decimal::Add(const Decimal *rhs, base::Arena *arena) const {
    Decimal *rv = NewUninitialized(arena, digitals_size());
    bool neg = negative();
    if (negative() == rhs->negative()) {
        //   lhs  +   rhs  =   lhs + rhs
        // (-lhs) + (-rhs) = -(lhs + rhs)
        RawAdd(this, rhs, rv);
    } else {
        //   lhs  + (-rhs) = lhs - rhs = -(rhs - lhs)
        // (-lhs) +   rhs  = rhs - lhs = -(lhs - rhs)
        if (AbsCompare(rhs) >= 0) {
            RawSub(this, rhs, rv);
        } else {
            neg = !neg;
            RawSub(rhs, this, rv);
        }
    }
    rv->set_negative_and_exp(neg, exp());
    return rv;
}
    
Decimal *Decimal::Sub(const Decimal *rhs, base::Arena *arena) const {
    Decimal *rv = NewUninitialized(arena, digitals_size());
    bool neg = negative();
    if (negative() != rhs->negative()) {
        // -lhs -   rhs  = -(lhs + rhs)
        //  lhs - (-rhs) =   lhs + rhs
        RawAdd(this, rhs, rv);
    } else {
        //   lhs  -   rhs  = lhs - rhs = -(rhs - lhs)
        // (-lhs) - (-rhs) = rhs - lhs = -(lhs - rhs)
        if (AbsCompare(rhs) >= 0) {
            RawSub(this, rhs, rv);
        } else {
            neg = !neg;
            RawSub(rhs, this, rv);
        }
    }
    rv->set_negative_and_exp(neg, exp());
    return rv;
}

Decimal *Decimal::Mul(const Decimal *rhs, base::Arena *arena) const {
    Decimal *rv = NewUninitialized(arena, digitals_size() + rhs->digitals_size());
    RawBasicMul(this, rhs, rv);
    rv->set_negative_and_exp(negative() != rhs->negative(), exp() + rhs->exp());
    return rv;
}

std::tuple<Decimal *, Decimal *> Decimal::Div(const Decimal *rhs,
                                              base::Arena *arena) const {
    if (rhs->zero()) {
        return {nullptr, nullptr};
    }
    if (zero()) {
        return {kZero, kZero};
    }
    
    int cmp = AbsCompare(rhs);
    if (cmp < 0 && exp() == 0 && rhs->exp() == 0) {
        return {kZero, NewWithRawData(arena, data(), size())};
    }
    if (cmp == 0) {
        return (negative() == rhs->negative())
            ? std::make_tuple(kOne, kZero) : std::make_tuple(kMinusOne, kZero);
    }

    Decimal *rv = nullptr, *rem = nullptr;
    if (rhs->segments_size() == 1) {
        rv = NewUninitialized(arena, digitals_size());
        rem = NewU64(arena, RawDivWord(this, rhs->segment(0), rv));
    } else {
        rv = NewUninitialized(arena, digitals_size());
        rem = NewUninitialized(arena, digitals_size());
        Decimal *tmp = NewUninitialized(arena, digitals_size());
        tmp->set_negative_and_exp(negative(), exp());
        rem->set_negative_and_exp(negative(), exp());
        RawDivSlow(this, rhs, tmp, rv, rem);
    }

    // 0.001 / 0.1 = 0.01
    // 0.01 / 0.1 = 0.1
    // 0.1 / 1    = 0.1
    // 1 / 0.1    = 10
    // 0.1 / 0.1  = 1
    int delta_exp = valid_exp() - rhs->valid_exp();
    if (delta_exp > 0) {
        rv->set_negative_and_exp(negative() != rhs->negative(), delta_exp);
    } else if (delta_exp < 0) {
        // 0.1 / 0.001 = 10
        if (exp() + delta_exp > 0) {
            rv->set_negative_and_exp(negative() != rhs->negative(),
                                     exp() + delta_exp - 1);
        } else {
            // TODO:
            rv = rv->Mul(NewU64(arena, kPowExp[-delta_exp]), arena);
            rv->set_negative_and_exp(negative() != rhs->negative(), 0);
        }
    } else {
        rv->set_negative_and_exp(negative() != rhs->negative(), rhs->exp());
    }
    rem->set_negative_and_exp(negative() != rhs->negative(), rhs->exp());
    return {rv, rem};
}
    
/*static*/ uint32_t Decimal::RawAdd(const Decimal *lhs, const Decimal *rhs,
                                    Decimal *rv) {
    DCHECK_EQ(lhs->digitals_size(), rhs->digitals_size());
    DCHECK_EQ(lhs->exp(), rhs->exp());
    
    uint32_t carry = 0;
    int64_t i = lhs->segments_size();
    while (i-- > 0) {
        uint32_t lval = lhs->segment(i);
        uint32_t rval = rhs->segment(i);
        uint32_t val = lval + rval + carry;
        rv->set_segment(i, val % kLimitMod);

        carry = val / kLimitMod;
    }
    return carry;
}
    
/*static*/ uint32_t Decimal::RawSub(const Decimal *lhs, const Decimal *rhs,
                                    Decimal *rv) {
    DCHECK_EQ(lhs->digitals_size(), rhs->digitals_size());
    DCHECK_EQ(lhs->exp(), rhs->exp());
    
    uint32_t carry = 0;
    int64_t i = lhs->segments_size();
    while (i-- > 0) {
        uint32_t lval = lhs->segment(i);
        uint32_t rval = rhs->segment(i);
         int32_t val = lval - rval - carry;
        rv->set_segment(i, (val < 0) ? kLimitMod + val : val);

        carry = (val < 0) ? 1 : 0;
    }
    return carry;
}
    
/*static*/ uint32_t Decimal::RawBasicMul(const Decimal *x, const Decimal *y,
                                    Decimal *rv) {
    DCHECK_EQ(rv->digitals_size(), x->digitals_size() + y->digitals_size());

    int64_t xstart = x->segments_size() - 1;
    int64_t ystart = y->segments_size() - 1;
    
    uint64_t carry = 0;
    for (int64_t j = ystart, k = ystart + 1 + xstart; j >= 0; j--, k--) {
        auto l = static_cast<uint64_t>(y->segment(j)),
             r = static_cast<uint64_t>(x->segment(xstart));

        uint64_t product = l * r + carry;
        rv->set_segment(k, static_cast<uint32_t>(product % kLimitMod));
        carry = product / kLimitMod;
    }
    rv->set_segment(xstart, static_cast<uint32_t>(carry));
    
    for (int64_t i = xstart - 1; i >= 0; i--) {
        carry = 0;
        for (int64_t j = ystart, k = ystart + 1 + i; j >= 0; j--, k--) {
            auto l = static_cast<uint64_t>(y->segment(j)),
                 r = static_cast<uint64_t>(x->segment(i)),
                 v = static_cast<uint64_t>(rv->segment(k));
            
            uint64_t product = l * r + v + carry;
            rv->set_segment(k, static_cast<uint32_t>(product % kLimitMod));
            carry = product / kLimitMod;
        }
        rv->set_segment(i, static_cast<uint32_t>(carry));
    }
    return static_cast<uint32_t>(carry);
}


/*static*/ void Decimal::RawDivSlow(const Decimal *lhs, const Decimal *rhs,
                                    Decimal *tmp, Decimal *rv, Decimal *rem) {
    size_t hil = lhs->highest_digital();
    DCHECK_LT(hil, lhs->digitals_size());
    size_t hir = rhs->highest_digital();
    DCHECK_LT(hir, rhs->digitals_size());
    
    DCHECK_GE(hil, hir);
    ::memset(rv->segments(), 0, rv->segments_size() * 4);
    //::memset(rem->segments(), 0, rem->segments_size() * 4);
    ::memcpy(rem->segments(), lhs->segments(), lhs->segments_size() * 4);
    
    // 999999999 : 8
    // 000012345 : 4
    //
    // 999999999 / 123450000 = 8 * 10000, 12399999
    //  12399999 / 12345000  = 1 * 1000,  54999
    //     54999 / 12345     = 4 * 1,     5619
    //                       = 81004,     5619
    //const Decimal *d0 = lhs;
    const Decimal *mid = rhs;
    int64_t m = 0;
    do {
        hil = rem->highest_digital();
        hir = rhs->highest_digital();
        //DCHECK_GE(hil, hir);
        m = hil - hir;
        if (m < 0) {
            break;
        } else if (m > 0) {
            ::memset(tmp->segments(), 0, tmp->segments_size() * 4);
            for (int64_t i = rhs->highest_digital(); i >= 0; i--) {
                size_t sid = tmp->digital_to_segment(i + m);
                uint32_t s = tmp->segment(sid);
                s += rhs->digital(i) * kPowExp[(i + m) % 9];
                tmp->set_segment(sid, s);
            }
            mid = tmp;
        } else {
            mid = rhs;
        }
//        printf("rem:%s\n", rem->ToString().c_str());
//        printf("mid:%s\n", mid->ToString().c_str());

        DCHECK_EQ(rem->highest_digital(), mid->highest_digital());
        int k = 0;
        while (rem->AbsCompare(mid) > 0) {
            RawSub(rem, mid, rem);
            k++;
//            printf("-rem:%s, mid:%s\n", rem->ToString().c_str(), mid->ToString().c_str());
        }
        DCHECK_LT(k, 10);
//        printf("+rem:%s\n", rem->ToString().c_str());
        
        size_t sid = rv->digital_to_segment(m);
        uint32_t s = rv->segment(sid);
        s += k * kPowExp[m % 9];
        rv->set_segment(sid, s);
        
        //d0 = rem;
    } while (m > 0);
}
    
/*static*/ Decimal *Decimal::RawDiv(const Decimal *lhs, const Decimal *rhs,
                                    Decimal *rv, Decimal *rem, base::Arena *arena) {
    DCHECK_EQ(rem->segments_size(), lhs->segments_size());
//for (size_t i = 0; i <)
//    ::memcpy(rem->prepared() + kHeaderSize, lhs->segments(),
//             lhs->segments_size() * 4);
    
    size_t rem_len = rem->segments_size();
    
    int shift = base::Bits::CountLeadingZeros32(rhs->segment(0));
    if (shift > 0) {
        auto tmp = NewWithRawData(arena, rhs->data(), rhs->size());
        tmp->PrimitiveShl(shift);
        rhs = tmp;
        rem = rem->Shl(shift, arena);
    }
    
    if (rem_len == rem->segments_size()) {
        // TODO:
        auto old = rem;
        rem = NewUninitialized(arena, old->digitals_size() + 9);
        DCHECK_EQ(rem->segments_size(), old->segments_size() + 1);
        rem->set_segment(0, 0);
        for (size_t i = 0; i < old->segments_size(); ++i) {
            rem->set_segment(i + 1, old->segment(i));
        }
    }
    
    size_t limit = rem_len - rhs->segments_size() + 1;
    uint64_t dh = static_cast<uint64_t>(rhs->segment(0));
    uint32_t dl = rhs->segment(1);
    uint32_t qword[2] = {0, 0};
    
    static const uint32_t kLimit = 900000000;
    
    // D2 Initialize j
    for (size_t j = 0; j < limit; ++j) {
        // D3 Calculate qhat
        // estimate qhat
        
        uint32_t qhat = 0;
        uint32_t qrem = 0;
        bool skip_correction = false;
        uint32_t nh = rem->segment(j);
        uint32_t nh2 = nh + kLimit;
        uint32_t nm = rem->segment(j + 1);
        
        if (nh == dh) {
            qhat = 999999999;
            qrem = nh + nm;
            skip_correction = qrem + kLimit < nh2;
        } else {
            uint64_t n_chunk = (((static_cast<uint64_t>(nh) * kLimitMod) +
                                  static_cast<uint64_t>(nm)));
            if (n_chunk >= 0) {
                qhat = static_cast<uint32_t>(n_chunk / dh);
                qrem = static_cast<uint32_t>(n_chunk - qhat * dh);
            } else {
                RawDivWord(n_chunk, dh, qword);
                qhat = qword[0];
                qrem = qword[1];
            }
            
            if (qhat == 0) {
                continue;
            }
            
            if (!skip_correction) { // Correct qhat
                uint64_t nl = static_cast<uint64_t>(rem->segment(j + 2));
                uint64_t rs = static_cast<uint64_t>(qrem) * kLimitMod + nl;
                uint64_t est_product = static_cast<uint64_t>(dl) * static_cast<uint64_t>(qhat);
                
                if (est_product > rs) {
                    qhat--;
                    qrem = static_cast<uint32_t>(static_cast<uint64_t>(qrem) + dh);
                    if (static_cast<uint64_t>(qrem) >= dh) {
                        est_product -= static_cast<uint64_t>(dl);
                        rs = static_cast<uint64_t>(qrem) * kLimitMod + nl;
                        if (est_product > rs) {
                            qhat--;
                        }
                    }
                }
            }
            
            // D4 Multiply and subtract
            rem->set_segment(j, 0);
            uint32_t borrow = MulSub(rhs, rem, qhat, rhs->segments_size(), j);
            
            // D5 Test remainder
            if (borrow + kLimit > nh2) {
                // D6 Add back
                DivAdd(rhs, rem, j + 1);
                qhat--;
            }
        }
        
        // Store the quotient digit
        rv->set_segment(j, qhat);
    } // D7 loop on j
    
    if (shift > 0) {
        rem = rem->Shr(shift, arena);
    }
    return rem;
}
    
// int divadd(int[] a, int[] result, int offset)
/*static*/ uint32_t Decimal::DivAdd(const Decimal *a, Decimal *rv,
                                    size_t offset) {
    uint64_t carry = 0;
    int64_t i = a->segments_size();
    while (i-- > 0) {
        uint64_t sum = static_cast<uint64_t>(a->segment(i)) +
                       static_cast<uint64_t>(rv->segment(i + offset)) + carry;
        rv->set_segment(i + offset, static_cast<uint32_t>(sum % kLimitMod));
        carry = sum / kLimitMod;
    }
    return static_cast<uint32_t>(carry);
}

// int mulsub(int[] q, int[] a, int x, int len, int offset)
/*static*/ uint32_t Decimal::MulSub(const Decimal *a, Decimal *q, uint32_t x,
                                    size_t len, size_t offset) {
    int64_t carry = 0;
    offset += len;
    int64_t i = len;
    while (i-- > 0) {
        int64_t product = static_cast<int64_t>(a->segment(i)) * x + carry;
        int64_t diff = static_cast<int64_t>(q->segment(offset)) - product;
        
        q->set_segment(offset--,
                       static_cast<uint32_t>(diff < 0 ? kLimitMod + diff : diff));
        
        carry = (product / kLimitMod) + (diff > 0 ? 1 : 0);
        //carry = (diff < 0) ? -diff : 0;
    }
    return static_cast<uint32_t>(carry);
}
    
/*static*/ uint32_t Decimal::RawDivWord(const Decimal *lhs, uint32_t rhs,
                                        Decimal *rv) {
    uint64_t divisor = static_cast<uint64_t>(rhs);
    if (lhs->segments_size() == 1) {
        uint64_t dividend = static_cast<uint64_t>(lhs->segment(0));
        uint32_t q = static_cast<uint32_t>(dividend / divisor);
        uint32_t r = static_cast<uint32_t>(dividend - q * divisor);
        
        for (size_t i = 0; i < rv->segments_size(); ++i) {
            rv->set_segment(i, 0);
        }
        rv->set_segment(0, q);
        return r;
    }
    
    int shift = base::Bits::CountLeadingZeros32(rhs);
    uint32_t r = lhs->segment(0);
    uint64_t rem = static_cast<uint64_t>(r);
    if (rem < divisor) {
        rv->set_segment(0, 0);
    } else {
        uint32_t d = static_cast<uint32_t>(rem / divisor);
        rv->set_segment(0, d);
        r = static_cast<uint32_t>(rem - (d * divisor));
        rem = static_cast<uint64_t>(r);
    }
    
    
    uint32_t qword[2] = {0, 0};
    for (int64_t i = 1; i < lhs->segments_size(); ++i) {
        int64_t estimate = (rem * kLimitMod) +
                            static_cast<int64_t>(lhs->segment(i));
        if (estimate >= 0) {
            qword[0] = static_cast<uint32_t>(estimate / divisor);
            qword[1] = static_cast<uint32_t>(estimate - qword[0] * divisor);
        } else {
            RawDivWord(estimate, divisor, qword);
        }
        rv->set_segment(i, qword[0]);
        r = qword[1];
        rem = static_cast<uint64_t>(r);
    }
    
    return shift > 0 ? r % divisor : r;
}
    
/*static*/ void Decimal::RawDivWord(uint64_t n, uint64_t d, uint32_t rv[2]) {
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
    
/*static*/ uint32_t Decimal::RawShl(const Decimal *lhs, uint32_t n,
                                    Decimal *rv) {
    uint64_t carry = 0;
    for (size_t i = 0, m = lhs->segments_size(); i < m; ++i) {
        uint64_t b = lhs->segment(i);
        uint64_t s = b << n;
        carry = s / kLimitMod;
        rv->set_segment(i, s % kLimitMod);
        if (i > 0) {
            int64_t j = i;
            while (j-- > 0) {
                s = rv->segment(j) + carry;
                rv->set_segment(j, static_cast<uint32_t>(s % kLimitMod));
                carry = s / kLimitMod;
                if (carry == 0) {
                    break;
                }
            }
        }
    }
    return static_cast<uint32_t>(carry);
}

/*static*/ uint32_t Decimal::RawShr(const Decimal *lhs, uint32_t n,
                                    Decimal *rv) {
/*
 552           int[] val = value;
 553           int n2 = 32 - n;
 554           for (int i=offset+intLen-1, c=val[i]; i>offset; i--) {
 555               int b = c;
 556               c = val[i-1];
 557               val[i] = (c << n2) | (b >>> n);
 558           }
 559           val[offset] >>>= n;
 */
    
    // FIXME: Use fast implements
    return RawDivWord(lhs, 1u << n, rv);
}
    
int Decimal::Compare(const Decimal *rhs) const {
    if (negative() == rhs->negative()) {
        int rv = AbsCompare(rhs);
        return negative() ? -rv : rv;
    }
    return negative() ? -1 : 1;
}
    
int Decimal::AbsCompare(const Decimal *rhs) const {    
    int rv = 0;
    if (segments_size() == rhs->segments_size() &&
        exp() == rhs->exp()) {
        for (size_t i = 0; i < segments_size(); ++i) {
            if (segment(i) < rhs->segment(i)) {
                rv = -1;
                break;
            } else if (segment(i) > rhs->segment(i)) {
                rv = 1;
                break;
            }
        }
    } else {
        //   j = 9, k = 6
        //   111.1100000 -> 10
        // 00011.11000   -> 8
        const int delta_exp = exp() - rhs->exp();
        const int delta_lhs = delta_exp > 0 ? 0 : +delta_exp;
        const int delta_rhs = delta_exp > 0 ? -delta_exp : 0;

        int64_t i = std::max(digitals_size(), rhs->digitals_size());
        while (i-- > 0) {
            int ld = GetDigitalOrZero(i + delta_lhs);
            int rd = rhs->GetDigitalOrZero(i + delta_rhs);

            if (ld < rd) {
                rv = -1;
                break;
            } else if (ld > rd) {
                rv = 1;
                break;
            }
        }
    }
    return rv;
}

std::string Decimal::ToString() const {
    size_t n_zero = 0;

    std::string buf;
    int prefix_zero = 0;
    for (size_t i = 0; i < segments_size(); ++i) {
        uint32_t s = segment(i);
        if (s == 0) {
            n_zero++;
        }
        for (size_t j = 0; j < 9; ++j) {
            char c = '0' + (s / kPowExp[8 - j]) % 10;
            if (c == '0') {
                if (prefix_zero == 0) {
                    prefix_zero = 1;
                }
            } else {
                prefix_zero = 2;
            }
            if (prefix_zero != 1) {
                buf.append(1, c);
            }
        }
    }
    if (n_zero == segments_size()) {
        return "0";
    }
    if (exp() > 0) {
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
        buf.insert(0, "-");
    }
    return buf;
}
    
Decimal *Decimal::Extend(int d, int m, base::Arena *arena) const {
    DCHECK_LE(d, kMaxDigitalSize); DCHECK_GT(d, 0);
    DCHECK_LE(m, kMaxExp);         DCHECK_GE(m, 0);
    DCHECK_LT(m, d);

    Decimal *dec = NewUninitialized(arena, d);
    if (!dec) {
        return nullptr;
    }
    size_t n_segment = dec->segments_size();
    // (18,9) 123456789.987654321
    // (18,3) 123456789.987
    // (9, 3)    456789.987
    // (9, 0)    456789
    // (10, 3) 00456789.987
    // (10, 4)  0456789.9870
    
    if (m <= exp()) {
        int64_t k = n_segment;
        size_t n = digitals_size() < d ? digitals_size() : d;
        int r = 0;
        uint32_t s = 0;
        for (size_t i = 0; i < n; ++i) {
            if (i + exp() - m >= digitals_size()) {
                break;
            }
            s += digital(i + exp() - m) * kPowExp[r++];
            if (r >= 9) {
                dec->set_segment(--k, s);
                s = 0;
                r = 0;
            }
        }
        if (r > 0) {
            dec->set_segment(--k, s);
        }
        while (k-- > 0) { // fill zero;
            dec->set_segment(k, 0);
        }
    } else if (m > exp()) {
        int64_t k = n_segment;
        int b = m - exp();
        while (k > n_segment - b / 9) {
            dec->set_segment(--k, 0);
        }
        int64_t n = digitals_size() < d ? digitals_size() : d;
        int r = b % 9;
        uint32_t s = 0;
        for (int64_t i = 0; i < n; ++i) {
            s += digital(i) * kPowExp[r++];
            if (r >= 9) {
                dec->set_segment(--k, s);
                s = 0;
                r = 0;
            }
        }
        if (r > 0) {
            dec->set_segment(--k, s);
        }
        while (k > 0) { // fill zero;
            dec->set_segment(--k, 0);
        }
    }

    dec->set_negative_and_exp(negative(), m);
    return dec;
}

/*static*/ Decimal *Decimal::New(base::Arena *arena, const char *s, size_t n) {
    Decimal *dec = nullptr;
    size_t n_segment = 0;
    bool negative = false;
    
    int rv = base::Slice::LikeNumber(s, n);
    switch (rv) {
        case 'o':
            // TODO:
            break;
        
        case 'd':
        case 's': {
            if (rv == 's') {
                negative = (s[0] == '-') ? true : false;
                n--;
                s++;
            } else {
                negative = false;
            }
            dec = NewUninitialized(arena, n);
            if (!dec) {
                return nullptr;
            }
            n_segment = dec->segments_size();
            uint32_t segment = 0;
            int r = 0;
            size_t k = n_segment;
            for (int64_t i = n - 1; i >= 0; --i) {
                segment += ((s[i] - '0') * kPowExp[r++]);
                if (r >= 9) {
                    dec->set_segment(--k, segment);
                    segment = 0;
                    r = 0;
                }
            }
            if (r > 0) {
                dec->set_segment(--k, segment);
            }
            dec->set_negative_and_exp(negative, 0);
        } break;
            
        case 'h':
            // TODO:
            break;
            
        case 'f': {
            if (s[0] == '-' || s[0] == '+') {
                negative = (s[0] == '-') ? true : false;
                n--;
                s++;
            } else {
                negative = false;
            }
            dec = NewUninitialized(arena, n - 1);
            if (!dec) {
                return nullptr;
            }
            n_segment = dec->segments_size();
            uint32_t segment = 0;
            int r = 0;
            size_t exp = 0, k = n_segment;
            for (int64_t i = n - 1; i >= 0; --i) {
                if (s[i] == '.') {
                    exp = n - i - 1;
                    continue;
                }
                segment += ((s[i] - '0') * kPowExp[r++]);
                if (r >= 9) {
                    dec->set_segment(--k, segment);
                    segment = 0;
                    r = 0;
                }
            }
            if (r > 0) {
                dec->set_segment(--k, segment);
            }
            dec->set_negative_and_exp(negative, static_cast<int>(exp));
        } break;
            
        default:
            break;
    }
    return dec;
}

/*static*/ Decimal *Decimal::NewI64(base::Arena *arena, int64_t value) {
    bool negative = value < 0;
    if (negative) {
        value = -value; 
    }
    if (value > 999999999999999999ull) {
        return nullptr; // overflow
    }

    Decimal *dec = nullptr;
    if (value > 999999999ull) {
        dec = NewUninitialized(arena, 18);
        dec->set_segment(0, static_cast<uint32_t>(value / 1000000000ull));
        dec->set_segment(1, static_cast<uint32_t>(value % 1000000000ull));
    } else {
        if (value <= kMaxConstVal) {
            return negative ? kConstants[kConstZeroIdx - value]
                            : kConstants[kConstZeroIdx + value];
        }
        dec = NewUninitialized(arena, 9);
        dec->set_segment(0, static_cast<uint32_t>(value));
    }

    dec->set_negative_and_exp(negative, 0);
    return dec;
}

/*static*/ Decimal *Decimal::NewU64(base::Arena *arena, uint64_t value) {
    if (value > 999999999999999999ull) {
        return nullptr; // overflow
    }
    Decimal *dec = nullptr;
    if (value > 999999999ull) {
        dec = NewUninitialized(arena, 18);
        dec->set_segment(0, static_cast<uint32_t>(value / 1000000000ull));
        dec->set_segment(1, static_cast<uint32_t>(value % 1000000000ull));
    } else {
        if (value <= kMaxConstVal) {
            return kConstants[kConstZeroIdx + value];
        }
        dec = NewUninitialized(arena, 9);
        dec->set_segment(0, static_cast<uint32_t>(value));
    }
    dec->set_negative_and_exp(false, 0);
    return dec;
}

} // namespace sql

} // namespace mai
