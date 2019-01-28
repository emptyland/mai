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
    
/*static*/ bool Slice::LikeFloating(const char *s, size_t n) {
    // 0 : begin
    // 1 : signe -/+
    // 2 : digit [0-9]
    // 3 : dot   .
    // 4 : float
    int state = 0;
    for (size_t i = 0; i < n; ++i) {
        if (s[i] == '-' || s[i] == '+') {
            if (state != 0) {
                return false;
            }
            state = 1;
        } else if (::isdigit(s[i])) {
            if (state == 3 || state == 4) {
                state = 4;
            } else {
                state = 2;
            }
        } else if (s[i] == '.') {
            if (state == 0 || state == 1 || state == 2) {
                state = 3;
            } else {
                return false;
            }
        } else {
            return false;
        }
    }
    return state == 4;
}

} // namespace base
    
} // namespace mai
