#ifndef MAI_BASE_HASH_H_
#define MAI_BASE_HASH_H_

#include <stddef.h>
#include <stdint.h>

extern "C" uint32_t crc32(uint32_t crc, const void *buf, size_t size);

namespace mai {
    
namespace base {
    
typedef uint32_t (*hash_func_t) (const char *, size_t);

struct Hash {
    static const int kNumberBloomFilterHashs = 5;
    static const hash_func_t kBloomFilterHashs[kNumberBloomFilterHashs];
    
    static uint32_t Js(const char *s, size_t n);
    static uint32_t Sdbm(const char *s, size_t n);
    static uint32_t Rs(const char *s, size_t n);
    static uint32_t Elf(const char *s, size_t n);
    static uint32_t Bkdr(const char *s, size_t n);
    static uint32_t Crc32(const char *s, size_t n);
    
}; // struct Hash

} // namespace base
    
} // namespace mai

#endif // MAI_BASE_HASH_H_
