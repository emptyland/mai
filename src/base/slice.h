#ifndef MAI_BASE_SLICE_H_
#define MAI_BASE_SLICE_H_

#include "base/allocators.h"
#include <string_view>

namespace mai {
    
namespace base {
    
struct Slice {
    
    static std::string_view GetU32(uint32_t value, ScopedMemory *scope) {
        char *p = static_cast<char *>(scope->New(sizeof(value)));
        *reinterpret_cast<uint32_t *>(p) = value;
        return std::string_view(p, sizeof(value));
    }
    
    template<class T>
    static std::string_view GetBlock(T *block) {
        return std::string_view(reinterpret_cast<const char *>(block),
                                sizeof(*block));
    }
    
    static std::string_view GetPad(size_t size, ScopedMemory *scope) {
        char *p = static_cast<char *>(scope->New(size));
        ::memset(p, 0, size);
        return std::string_view(p, size);
    }
    
}; // struct Slice
    
} // namespace base
    
} // namespace mai


#endif // MAI_BASE_SLICE_H_
