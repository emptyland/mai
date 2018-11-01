#include "base/hash.h"

namespace mai {
    
namespace base {

/*static*/ uint32_t Hash::Js(const char *p, size_t n) {
    int hash = 1315423911;
    for (auto s = p; s < p + n; s++) {
        hash ^= ((hash << 5) + (*s) + (hash >> 2));
    }
    return hash;
}

/*static*/ uint32_t Hash::Crc32(const char *s, size_t n) {
    return ::crc32(0, s, n);
}
    
} // namespace base
    
} // namespace mai
