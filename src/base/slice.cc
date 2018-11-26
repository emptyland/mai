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

} // namespace base
    
} // namespace mai
