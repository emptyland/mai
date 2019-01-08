#include "base/arena-utils.h"

namespace mai {
    
namespace base {
    
static inline void Encode16(void *x, uint16_t val) {
    uint8_t *buf = static_cast<uint8_t *>(x);
    buf[0] = static_cast<uint8_t>((0xff00 & val) >> 8);
    buf[1] = static_cast<uint8_t>((0x00ff & val));
}

static inline void Encode32(void *x, uint32_t val) {
    uint8_t *buf = static_cast<uint8_t *>(x);
    buf[0] = static_cast<uint8_t>((0xff000000 & val) >> 24);
    buf[1] = static_cast<uint8_t>((0x00ff0000 & val) >> 16);
    buf[2] = static_cast<uint8_t>((0x0000ff00 & val) >> 8);
    buf[3] = static_cast<uint8_t>((0x000000ff & val));
}
    
/*static*/ ArenaString *ArenaString::New(base::Arena *arena, const char *s,
                                         size_t n) {
    size_t size = sizeof(ArenaString);
    Kind kind = GetKind(n);
    switch (kind) {
        case kSmall:
            size += (n + 1);
            break;
        case kLarge:
            size += (2 + n + 1);
            break;
        case kOverflow:
            return nullptr;
    }
    void *chunk = arena->Allocate(size);
    return new (chunk) ArenaString(s, n, kind);
}
    
ArenaString::ArenaString(const char *s, size_t n, Kind kind)
    : hash_val_(Hash::Js(s, n)) {
    DCHECK_NE(kOverflow, kind);
    
    char *x = nullptr;
    if (kind == kSmall) {
        uint16_t raw_size = static_cast<uint16_t>(n & 0x7fffu);
        Encode16(buf_, raw_size);
        x = reinterpret_cast<char *>(this + 1);
    } else if (kind == kLarge) {
        uint32_t raw_size = static_cast<uint32_t>(n & 0x3fffffffu);
        Encode32(buf_, raw_size);
        buf_[0] |= 0x80;
        x = reinterpret_cast<char *>(this + 1) + 2;
    }
    ::memcpy(x, s, n);
    x[n] = '\0';
}
    
} // namespace base
    
} // namespace mai
