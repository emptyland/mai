#include "base/hash.h"

namespace mai {
    
namespace base {
    
const hash_func_t Hash::kBloomFilterHashs[Hash::kNumberBloomFilterHashs] = {
    &Hash::Js, &Hash::Rs, &Hash::Bkdr, &Hash::Sdbm, &Hash::Elf,
};

/*static*/ uint32_t Hash::Js(const char *p, size_t n) {
    int hash = 1315423911;
    for (auto s = p; s < p + n; s++) {
        hash ^= ((hash << 5) + (*s) + (hash >> 2));
    }
    return hash;
}
    
/*static*/ uint32_t Hash::Sdbm(const char *p, size_t n) {
    uint32_t hash = 0;
    for (auto s = p; s < p + n; s++) {
        // equivalent to:
        hash = 65599 * hash + *s;
        hash = *s + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

/*static*/ uint32_t Hash::Rs(const char *p, size_t n) {
    uint32_t b = 378551;
    uint32_t a = 63689;
    uint32_t hash = 0;
    for (auto s = p; s < p + n; s++) {
        hash = hash * a + *s; a *= b;
    }
    return hash;
}

/*static*/ uint32_t Hash::Elf(const char *p, size_t n) {
    uint32_t hash = 0;
    uint32_t x = 0;
    for (auto s = p; s < p + n; s++) {
        hash = (hash << 4) + *s;
        if ((x = hash & 0xF0000000L) != 0) {
            hash ^= (x >> 24);
            hash &= ~x;
        }
    }
    return hash;
}

/*static*/ uint32_t Hash::Bkdr(const char *p, size_t n) {
    uint32_t seed = 131; // 31 131 1313 13131 131313 etc..
    uint32_t hash = 0;
    for (auto s = p; s < p + n; s++) {
        hash = hash * seed + *s;
    }
    return hash;
}

/*static*/ uint32_t Hash::Crc32(const char *s, size_t n) {
    return ::crc32(0, s, n);
}
    
} // namespace base
    
} // namespace mai
