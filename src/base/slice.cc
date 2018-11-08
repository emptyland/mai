#include "base/slice.h"
#include <ctype.h>

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

} // namespace base
    
} // namespace mai
