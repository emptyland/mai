#ifndef MAI_BASE_SLICE_H_
#define MAI_BASE_SLICE_H_

#include "base/varint-encoding.h"
#include "base/allocators.h"
#include "glog/logging.h"
#include <string_view>

namespace mai {
    
namespace base {
    
struct Slice {
    
    static std::string_view GetU32(uint32_t value, ScopedMemory *scope) {
        char *p = static_cast<char *>(scope->New(sizeof(value)));
        *reinterpret_cast<uint32_t *>(p) = value;
        return std::string_view(p, sizeof(value));
    }
    
    static std::string_view GetU64(uint64_t value, ScopedMemory *scope) {
        char *p = static_cast<char *>(scope->New(sizeof(value)));
        *reinterpret_cast<uint64_t *>(p) = value;
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
    
    static std::string_view GetV32(uint32_t value, ScopedMemory *scope) {
        char *p = static_cast<char *>(scope->New(Varint32::kMaxLen));
        size_t n = Varint32::Encode(p, value);
        return std::string_view(p, n);
    }
    
    static std::string_view GetV64(uint64_t value, ScopedMemory *scope) {
        char *p = static_cast<char *>(scope->New(Varint64::kMaxLen));
        size_t n = Varint64::Encode(p, value);
        return std::string_view(p, n);
    }
    
    static uint32_t SetU32(std::string_view slice) {
        DCHECK_EQ(sizeof(uint32_t), slice.size());
        return *reinterpret_cast<const uint32_t *>(slice.data());
    }
    
    static uint64_t SetU64(std::string_view slice) {
        DCHECK_EQ(sizeof(uint64_t), slice.size());
        return *reinterpret_cast<const uint64_t *>(slice.data());
    }
    
    static std::string ToReadable(std::string_view raw);
}; // struct Slice
    
} // namespace base
    
} // namespace mai


#endif // MAI_BASE_SLICE_H_
