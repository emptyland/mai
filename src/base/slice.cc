#include "base/slice.h"
#include <ctype.h>
#include <stdio.h>

namespace mai {
    
namespace base {
    
//static const int64_t kPow10Exps[19] = {
//    1LL,
//    10LL,
//    100LL,
//    1000LL,
//    10000LL,
//    100000LL,
//    1000000LL,
//    10000000LL,
//    100000000LL,
//    1000000000LL,
//    10000000000LL,
//    100000000000LL,
//    1000000000000LL,
//    10000000000000LL,
//    100000000000000LL,
//    1000000000000000LL,
//    10000000000000000LL,
//    100000000000000000LL,
//    1000000000000000000LL,
//};

/*static*/ std::string Slice::ToReadable(std::string_view raw) {
    static char hex_aplha_table[] = "0123456789abcdef";
    
    std::string s;
    for (char c : raw) {
        if (::isprint(c)) {
            s.append(1, c);
        } else {
            s.append("\\x");
            s.append(1, hex_aplha_table[(c & 0xf0) >> 4]);
            s.append(1, hex_aplha_table[c & 0x0f]);
        }
    }
    return s;
}
    
/*static*/ int Slice::ParseI64(const char *s, size_t n, int64_t *val) {
    int sign = s[0] == '-' ? -1 : 1;
    if (s[0] == '-' || s[0] == '+') {
        s++;
        n--;
    }
    if (n == 0) {
        return -1;
    }
    if (n > 19) {
        return 1;
    }
    
    uint64_t l = 0;
    for (size_t i = 0; i < n; ++i) {
        if (*s < '0' || *s > '9') {
            return -1;
        }
        uint64_t d = 0x7fffffffffffffffULL - l * 10;
        uint64_t e = (*s++ - '0');
        d += (sign < 0) ? 1 : 0;
        if (e > d) {
            return 1;
        }
        l = l * 10 + e;
    }

    *val = l * sign;
    return 0;
}
    
/*static*/ int Slice::ParseU64(const char *s, size_t n, uint64_t *val) {
    if (n == 0) {
        return -1;
    }
    if (n > 20) { /*MAX: 18446744073709551615*/
        return 1;
    }
    uint64_t l = 0;
    for (size_t i = 0; i < n; ++i) {
        if (*s < '0' || *s > '9') {
            return -1;
        }
        uint64_t d = 0xffffffffffffffffULL - l * 10;
        uint64_t e = (*s++ - '0');
        if (e > d) {
            return 1;
        }
        l = l * 10 + e;
    }
    *val = l;
    return 0;
}
    
/*static*/ int Slice::ParseH64(const char *s, size_t n, uint64_t *val) {
    if (n == 0) {
        return -1;
    }
    if (n > 16) { /*MAX: ffffffffffffffff*/
        return 1;
    }
    uint64_t l = 0;
    for (size_t i = 0; i < n; ++i) {
        uint64_t e = 0;
        if (*s >= '0' && *s <= '9') {
            e = *s - '0';
        } else if (*s >= 'a' && *s <= 'f') {
            e = *s - 'a' + 10;
        } else if (*s >= 'A' && *s <= 'F') {
            e = *s - 'A' + 10;
        } else {
            return -1;
        }
        l = (l << 4) | e;
        ++s;
    }
    *val = l;
    return 0;
}

/*static*/ int Slice::ParseO64(const char *s, size_t n, uint64_t *val) {
    if (n == 0) {
        return -1;
    }
    if (n > 22) { /*MAX: 1777777777777777777777*/
        return 1;
    }
    
    uint64_t l = 0;
    for (size_t i = 0; i < n; ++i) {
        if (*s < '0' || *s > '7') {
            return -1;
        }
        uint64_t d = 0xffffffffffffffffULL - l * 8;
        uint64_t e = (*s++ - '0');
        if (e > d) {
            return 1;
        }
        l = l * 8 + e;
    }
    *val = l;
    return 0;
}
    
/*static*/ int Slice::ParseI32(const char *s, size_t n, int32_t *val) {
    int sign = s[0] == '-' ? -1 : 1;
    if (s[0] == '-' || s[0] == '+') {
        s++;
        n--;
    }
    if (n == 0) {
        return -1;
    }
    if (n > 10) {
        return 1;
    }
    
    uint32_t l = 0;
    for (size_t i = 0; i < n; ++i) {
        if (*s < '0' || *s > '9') {
            return -1;
        }
        uint32_t d = 0x7fffffffU - l * 10;
        uint32_t e = (*s++ - '0');
        d += (sign < 0) ? 1 : 0;
        if (e > d) {
            return 1;
        }
        l = l * 10 + e;
    }
    
    *val = l * sign;
    return 0;
}

/*static*/ std::string Sprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::string str(Vsprintf(fmt, ap));
    va_end(ap);
    return str;
}

/*static*/ std::string Vsprintf(const char *fmt, va_list ap) {
    va_list copied;
    int len = 128, rv = len;
    std::unique_ptr<char[]> buf;
    do {
        len = rv + 128;
        buf.reset(new char[len]);
        va_copy(copied, ap);
        rv = ::vsnprintf(buf.get(), len, fmt, ap);
        va_copy(ap, copied);
    } while (rv > len);
    //buf[rv] = 0;
    return std::string(buf.get());
}
    
// return:
// 0 = not a number
// 'o' = octal
// 'd' = decimal
// 's' = signed decimal
// 'h' = hexadecimal
// 'f' = float
/*static*/ int Slice::LikeNumber(const char *s, size_t n) {
    enum State {
        kInit,
        kPrefixSign,
        kPrefixZero,
        kPrefix0x, // hex prefix
        kPrevDot,
        kPrefixExp,
        kPrefixExpSign,
        
        kOct,
        kDec,
        kDecSgined,
        kHex,
        kFloat,
        kExponent,
    };
    
    State state = kInit;
    const char *e = s + n;
    while (s < e) {
        const char c = *s++;
        
        if (c == '-' || c == '+') {
            if (state == kPrefixExp) {
                state = kPrefixExpSign;
            } else if (state == kInit) {
                state = kPrefixSign;
            } else {
                return 0;
            }
        } else if (c == '0') {
            if (state == kInit) {
                state = kPrefixZero;
            } else if (state == kPrefixSign) {
                state = kDecSgined;
            } else if (state == kPrefixZero) {
                state = kOct;
            } else if (state == kPrefix0x) {
                state = kHex;
            } else if (state == kPrevDot) {
                state = kFloat;
            } else if (state == kPrefixExp ||
                       state == kPrefixExpSign) {
                state = kExponent;
            } else if (state == kDec ||
                       state == kOct ||
                       state == kDecSgined ||
                       state == kHex ||
                       state == kFloat ||
                       state == kExponent) {
                // ignore
            } else {
                return 0;
            }
        } else if (c == 'x' || c == 'X') {
            if (state == kPrefixZero) {
                state = kPrefix0x;
            } else {
                return 0;
            }
        } else if (c > '0' && c <= '7') {
            if (state == kInit) {
                state = kDec;
            } else if (state == kPrefixZero) {
                state = kOct;
            } else if (state == kPrefix0x) {
                state = kHex;
            } else if (state == kPrefixSign) {
                state = kDecSgined;
            } else if (state == kPrevDot) {
                state = kFloat;
            } else if (state == kPrefixExp ||
                       state == kPrefixExpSign) {
                state = kExponent;
            } else if (state == kOct ||
                       state == kDecSgined ||
                       state == kDec ||
                       state == kHex ||
                       state == kFloat ||
                       state == kExponent) {
                // ignore
            } else {
                return 0;
            }
        } else if (c > '7' && c <= '9') {
            if (state == kInit) {
                state = kDec;
            } else if (state == kPrefix0x) {
                state = kHex;
            } else if (state == kPrefixSign) {
                state = kDecSgined;
            } else if (state == kPrevDot) {
                state = kFloat;
            } else if (state == kPrefixExp ||
                       state == kPrefixExpSign) {
                state = kExponent;
            } else if (state == kDecSgined ||
                       state == kDec ||
                       state == kHex ||
                       state == kFloat ||
                       state == kExponent) {
                // ignore
            } else {
                return 0;
            }
        } else if ((c >= 'a' && c <= 'f') ||
                   (c >= 'A' && c <= 'F')) {
            if (c == 'e' || c == 'E') {
                if (state == kDec ||
                    state == kDecSgined ||
                    state == kPrefixZero ||
                    state == kFloat) {
                    state = kPrefixExp;
                    continue;
                }
            }
            if (state == kPrefix0x) {
                state = kHex;
            } else if (state == kHex) {
                // ignore
            } else {
                return 0;
            }
        } else if (c == '.') {
            if (state == kInit) {
                state = kPrevDot;
            } else if (state == kPrefixSign ||
                       state == kDecSgined ||
                       state == kPrefixZero ||
                       state == kDec) {
                state = kPrevDot;
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    }
    
    switch (state) {
        case kOct:
            return 'o';
        case kDecSgined:
            return 's';
        case kPrefixZero: // Only one zero
        case kDec:
            return 'd';
        case kHex:
            return 'h';
        case kFloat:
            return 'f';
        case kExponent:
            return 'e';
        default:
            break;
    }
    return 0;
}
    
/*static*/ int Slice::ParseEscaped(const char *s, size_t n, std::string *rv) {
    const char *e = s + n;
    while (s < e) {
        const char c = *s++;
        if (c != '\\') {
            rv->append(1, c);
            continue;
        }
        
        // c == '\\'
        if (s >= e) {
            rv->append(1, '\\');
            break;
        }

        switch (*s++) {
            case 'a':
                rv->append(1, '\a');
                break;
            case 'b':
                rv->append(1, '\b');
                break;
            case 'f':
                rv->append(1, '\f');
                break;
            case 'n':
                rv->append(1, '\n');
                break;
            case 'r':
                rv->append(1, '\r');
                break;
            case 't':
                rv->append(1, '\t');
                break;
            case 'v':
                rv->append(1, '\v');
                break;
            case '\\':
                rv->append(1, '\\');
                break;
            case '\'':
                rv->append(1, '\'');
                break;
            case '\"':
                rv->append(1, '\"');
                break;
            case '0': {
                uint8_t v = 0;
                for (int i = 0; i < 3; ++i) {
                    char cc = *s;
                    if (s >= e) {
                        break;
                    }
                    if (cc >= '0' && cc <= '7') {
                        v = (v << 3) + (cc - '0');
                    } else {
                        break;
                    }
                    s++;
                }
                rv->append(1, static_cast<char>(v));
            } break;
                
            case 'x': {
                uint8_t v = 0;
                for (int i = 0; i < 2; ++i) {
                    char cc = *s;
                    if (s >= e) {
                        break;
                    }
                    if (cc >= '0' && cc <= '9') {
                        v = (v << 4) + (cc - '0');
                    } else if (cc >= 'a' && cc <= 'f') {
                        v = (v << 4) + (10 + cc - 'a');
                    } else if (cc >= 'A' && cc <= 'F') {
                        v = (v << 4) + (10 + cc - 'A');
                    } else {
                        break;
                    }
                    s++;
                }
                rv->append(1, static_cast<char>(v));
            } break;

            default:
                return -1;
        }
    }
    return 0;
}
    
template<class T>
inline void *RoundBytesFill(const T zag, void *chunk, size_t n) {
    static_assert(sizeof(zag) > 1 && sizeof(zag) <= 8, "T must be int16,32,64");
    static_assert(std::is_integral<T>::value, "T must be int16,32,64");
    
    DCHECK_GE(n, 0);
    auto result = chunk;
    auto round = n / sizeof(T);
    while (round--) {
        auto round_bits = static_cast<T *>(chunk);
        *round_bits = zag;
        chunk = static_cast<void *>(round_bits + 1);
    }
    
    auto zag_bytes = reinterpret_cast<const uint8_t *>(&zag);
    
    round = n % sizeof(T);
    while (round--) {
        auto bits8 = static_cast<uint8_t *>(chunk);
        *bits8 = *zag_bytes++;
        chunk = static_cast<void *>(bits8 + 1);
    }
    return result;
}

void *Round16BytesFill(const uint16_t zag, void *chunk, size_t n) {
    return RoundBytesFill<uint16_t>(zag, chunk, n);
}

void *Round32BytesFill(const uint32_t zag, void *chunk, size_t n) {
    return RoundBytesFill<uint32_t>(zag, chunk, n);
}

void *Round64BytesFill(const uint64_t zag, void *chunk, size_t n) {
    return RoundBytesFill<uint64_t>(zag, chunk, n);
}

/*virtual*/ AbstractPrinter::~AbstractPrinter() {
}

/*virtual*/ void AbstractPrinter::Printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    VPrintf(fmt, ap);
    va_end(ap);
}

} // namespace base
    
} // namespace mai
