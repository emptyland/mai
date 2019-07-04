#include "base/arena-utils.h"

namespace mai {
    
namespace base {
    
static const char empty_arean_string_stub[sizeof(ArenaString)] = {0};
    
/*static*/ const ArenaString *const ArenaString::kEmpty =
    reinterpret_cast<const ArenaString *>(empty_arean_string_stub);
    
/*static*/ ArenaString *ArenaString::New(base::Arena *arena, const char *s,
                                         size_t n) {
    void *chunk = arena->Allocate(n + 1 + sizeof(ArenaString));
    if (!chunk) {
        return nullptr;
    }
    return new (chunk) ArenaString(s, n);
}
    
ArenaString::ArenaString(const char *s, size_t n)
    : hash_val_(Hash::Js(s, n))
    , len_(static_cast<uint32_t>(n)) {
    ::memcpy(buf_, s, n);
    buf_[n] = '\0';
}
    
ArenaString::ArenaString(size_t n)
    : hash_val_(0)
    , len_(static_cast<uint32_t>(n)) {
    buf_[n] = '\0';
}
    
} // namespace base
    
} // namespace mai
