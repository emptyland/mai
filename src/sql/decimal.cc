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
    
std::string Decimal::ToString() const {
    std::string buf;
    if (negative()) {
        buf.append("-");
    }
    int prefix_zero = 0;
    for (size_t i = 0; i < segment_size(); ++i) {
        uint32_t s = segment(i);
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
    if (buf.empty() && prefix_zero == 1) {
        buf.append(1, '0');
    }
    if (exp() > 0) {
        buf.insert(buf.size() - exp(), ".");
    }
    return buf;
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
            size = n_segment * 4 + 1;
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
            size = n_segment * 4 + 1;
            dec = static_cast<Decimal *>(NewPrepare(arena, size));
            if (!dec) {
                return nullptr;
            }
            uint32_t segment = 0;
            int r = 0;
            size_t exp = 0, si = n_segment;
            for (int64_t i = n - 1; i >= 0; --i) {
                if (s[i] == '.') {
                    exp = n - i - 1;
                    continue;
                }
                segment += ((s[i] - '0') * kPowExp[r++]);
                if (r >= 9) {
                    dec->set_segment(--si, segment);
                    segment = 0;
                    r = 0;
                }
            }
            if (r > 0) {
                dec->set_segment(--si, segment);
            }
            dec->prepared()[0] = (negative ? 0x00 : 0x80) | static_cast<int8_t>(exp);
        } break;
            
        default:
            break;
    }
    return dec;
}

/*static*/ Decimal *Decimal::NewI64(base::Arena *arena, int64_t value) {
    return nullptr;
}

/*static*/ Decimal *Decimal::NewU64(base::Arena *arena, uint64_t value) {
    return nullptr;
}

} // namespace sql

} // namespace mai
