#include "base/slice.h"
#include <ctype.h>
#include <stdio.h>

namespace mai {
    
namespace base {

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

/*static*/ std::string Slice::Sprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::string str(Vsprintf(fmt, ap));
    va_end(ap);
    return str;
}

/*static*/ std::string Slice::Vsprintf(const char *fmt, va_list ap) {
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
        
        kOct,
        kDec,
        kDecSgined,
        kHex,
        kFloat,
    };
    
    State state = kInit;
    const char *e = s + n;
    while (s < e) {
        const char c = *s++;
        
        if (c == '-' || c == '+') {
            if (state != kInit) {
                return 0;
            }
            state = kPrefixSign;
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
            } else if (state == kDec ||
                       state == kOct ||
                       state == kDecSgined ||
                       state == kHex ||
                       state == kFloat) {
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
            } else if (state == kOct ||
                       state == kDecSgined ||
                       state == kDec ||
                       state == kHex ||
                       state == kFloat) {
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
            } else if (state == kDecSgined ||
                       state == kDec ||
                       state == kHex ||
                       state == kFloat) {
                // ignore
            } else {
                return 0;
            }
        } else if ((c >= 'a' && c <= 'f') ||
                   (c >= 'A' && c <= 'F')) {
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
        default:
            break;
    }
    return 0;
}

} // namespace base
    
} // namespace mai
