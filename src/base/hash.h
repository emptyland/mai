#ifndef MAI_BASE_HASH_H_
#define MAI_BASE_HASH_H_

#include <stddef.h>
#include <stdint.h>

extern "C" uint32_t crc32(uint32_t crc, const void *buf, size_t size);

namespace mai {
    
namespace base {

struct Hash {
    
    static uint32_t Js(const char *s, size_t n);
    
    static uint32_t Crc32(const char *s, size_t n);
    
}; // struct Hash

} // namespace base
    
} // namespace mai

#endif // MAI_BASE_HASH_H_
