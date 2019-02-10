#include "sql/decimal.h"
#include "base/slice.h"

namespace mai {

namespace sql {

static uint8_t zero_decimal_stub[sizeof(Decimal) + Decimal::kHeaderSize] = {
    0, 0, 0, 0, // hash_val
    0, static_cast<uint8_t>(Decimal::kHeaderSize), 0, 0, // size
    0x80, 0, 0, 0, // header
};

static uint8_t  one_decimal_stub[sizeof(Decimal) + Decimal::kHeaderSize + 4] = {
    0, 0, 0, 0, // hash_val
    0, static_cast<uint8_t>(Decimal::kHeaderSize) + 4, 0, 0, // size
    0x80, 0, 0, 0, // header
    0, 0, 0, 1, // one
};

static
uint8_t minus_one_decimal_stub[sizeof(Decimal) + Decimal::kHeaderSize + 4] = {
    0, 0, 0, 0, // hash_val
    0, static_cast<uint8_t>(Decimal::kHeaderSize) + 4, 0, 0, // size
    0, 0, 0, 0, // header
    0, 0, 0, 1, // one
};
    
/*static*/
Decimal *const Decimal::kZero = reinterpret_cast<Decimal *>(zero_decimal_stub);
    
/*static*/
Decimal *const Decimal::kOne  = reinterpret_cast<Decimal *>(one_decimal_stub);
    
/*static*/
Decimal *const Decimal::kMinusOne
    = reinterpret_cast<Decimal *>(minus_one_decimal_stub);
    
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
    rv->set_negative_and_exp(negative() != rhs->negative(), exp());
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
    
    int rv = AbsCompare(rhs);
    if (rv < 0) {
        return {kZero, NewWithRawData(arena, data(), size())};
    }
    if (rv == 0) {
        return (negative() == rhs->negative())
            ? std::make_tuple(kOne, kZero) : std::make_tuple(kMinusOne, kZero);
    }

    // TODO:
    return {nullptr, nullptr};
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
        
        carry = (val < 0) ? -val : 0;
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
    
/*static*/ uint32_t Decimal::RawShl(const Decimal *lhs, uint32_t n,
                                    Decimal *rv) {
    // TODO:
    return 0;
}

/*static*/ uint32_t Decimal::RawShr(const Decimal *lhs, uint32_t n,
                                    Decimal *rv) {
    // TODO:
    return 0;
}
    
int Decimal::Compare(const Decimal *rhs) const {
    DCHECK_EQ(digitals_size(), rhs->digitals_size());
    DCHECK_EQ(exp(), rhs->exp());

    if (negative() == rhs->negative()) {
        int rv = AbsCompare(rhs);
        return negative() ? -rv : rv;
    }
    return negative() ? -1 : 1;
}
    
int Decimal::AbsCompare(const Decimal *rhs) const {
    DCHECK_EQ(digitals_size(), rhs->digitals_size());
    DCHECK_EQ(exp(), rhs->exp());
    
    int rv = 0;
    for (size_t i = 0; i < segments_size(); ++i) {
        if (segment(i) < rhs->segment(i)) {
            rv = -1;
            break;
        } else if (segment(i) > rhs->segment(i)) {
            rv = 1;
            break;
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
        if (value == 0) {
            return kZero;
        } else if (value == 1) {
            return kOne;
        } else if (value == -1) {
            return kMinusOne;
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
        if (value == 0) {
            return kZero;
        } else if (value == 1) {
            return kOne;
        }
        dec = NewUninitialized(arena, 9);
        dec->set_segment(0, static_cast<uint32_t>(value));
    }
    dec->set_negative_and_exp(false, 0);
    return dec;
}

} // namespace sql

} // namespace mai
