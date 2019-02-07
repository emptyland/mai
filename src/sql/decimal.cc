#include "sql/decimal.h"
#include "base/slice.h"

namespace mai {

namespace sql {

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
    
int Decimal::Compare(const Decimal *rhs) const {
    DCHECK_EQ(digital_size(), rhs->digital_size());
    DCHECK_EQ(exp(), rhs->exp());
    
    // TODO:
    return 0;
}
    
std::string Decimal::ToString() const {
    size_t n_zero = 0;

    std::string buf;
    int prefix_zero = 0;
    for (size_t i = 0; i < segment_size(); ++i) {
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
    if (n_zero == segment_size()) {
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
    
    size_t n_segment = (d + 8) / 9;
    size_t size = n_segment * 4 + 4;
    Decimal *dec = static_cast<Decimal *>(NewPrepare(arena, size));
    if (!dec) {
        return nullptr;
    }
    // (18,9) 123456789.987654321
    // (18,3) 123456789.987
    // (9, 3)    456789.987
    // (9, 0)    456789
    // (10, 3) 00456789.987
    // (10, 4)  0456789.9870
    
    if (m <= exp()) {
        int64_t k = n_segment;
        size_t n = digital_size() < d ? digital_size() : d;
        int r = 0;
        uint32_t s = 0;
        for (size_t i = 0; i < n; ++i) {
            if (i + exp() - m >= digital_size()) {
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
        int64_t n = digital_size() < d ? digital_size() : d;
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

    dec->prepared()[0] = (negative() ? 0x00 : 0x80)
                       | static_cast<int8_t>(m);
    return dec;
}

/*static*/ Decimal *Decimal::New(base::Arena *arena, const char *s, size_t n) {
    Decimal *dec = nullptr;
    size_t size = 0, n_segment = 0;
    bool negative = false;
    
    int rv = base::Slice::LikeNumber(s, n);
    switch (rv) {
        case 'o':
            // TODO:
            break;
        
        case 'd':
        case 's':
            if (rv == 's') {
                negative = (s[0] == '-') ? true : false;
                n--;
                s++;
            } else {
                negative = false;
            }
            n_segment = (n + 8) / 9;
            size = n_segment * 4 + 4;
            dec = static_cast<Decimal *>(NewPrepare(arena, size));
            if (!dec) {
                return nullptr;
            }
            dec->prepared()[0] = negative ? 0x00 : 0x80;
            for (size_t i = 0; i < n_segment; ++i) {
                size_t b = i * 9;
                size_t e = std::min(b + 9, n);
                uint32_t segment = 0;
                for (size_t j = b; j < e; ++j) {
                    segment += ((s[e - j - 1] - '0') * kPowExp[j - b]);
                }
                dec->set_segment(i, segment);
            }
            break;
            
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
            n_segment = (n - 1 + 8) / 9;
            size = n_segment * 4 + 4;
            dec = static_cast<Decimal *>(NewPrepare(arena, size));
            if (!dec) {
                return nullptr;
            }
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
            dec->prepared()[0] = (negative ? 0x00 : 0x80) | static_cast<int8_t>(exp);
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
        dec = static_cast<Decimal *>(NewPrepare(arena, 4 + 8));
        dec->set_segment(0, static_cast<uint32_t>(value / 1000000000ull));
        dec->set_segment(1, static_cast<uint32_t>(value % 1000000000ull));
    } else {
        dec = static_cast<Decimal *>(NewPrepare(arena, 4 + 4));
        dec->set_segment(0, static_cast<uint32_t>(value));
    }

    dec->prepared()[0] = (negative ? 0x00 : 0x80);
    return dec;
}

/*static*/ Decimal *Decimal::NewU64(base::Arena *arena, uint64_t value) {
    if (value > 999999999999999999ull) {
        return nullptr; // overflow
    }
    Decimal *dec = nullptr;
    if (value > 999999999ull) {
        dec = static_cast<Decimal *>(NewPrepare(arena, 4 + 8));
        dec->set_segment(0, static_cast<uint32_t>(value / 1000000000ull));
        dec->set_segment(1, static_cast<uint32_t>(value % 1000000000ull));
    } else {
        dec = static_cast<Decimal *>(NewPrepare(arena, 4 + 4));
        dec->set_segment(0, static_cast<uint32_t>(value));
    }
    dec->prepared()[0] = 0x80;
    return dec;
}

} // namespace sql

} // namespace mai
