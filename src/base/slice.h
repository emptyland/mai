#ifndef MAI_BASE_SLICE_H_
#define MAI_BASE_SLICE_H_

#include "base/varint-encoding.h"
#include "base/allocators.h"
#include "glog/logging.h"
#include <stdarg.h>
#include <string_view>

namespace mai {
    
namespace base {
    
struct Slice {
    
    static std::string_view GetByte(char value, ScopedMemory *scope) {
        char *p = static_cast<char *>(scope->New(sizeof(value)));
        *p = value;
        return std::string_view(p, sizeof(value));
    }
    
    static std::string_view GetU16(uint16_t value, ScopedMemory *scope) {
        char *p = static_cast<char *>(scope->New(sizeof(value)));
        *reinterpret_cast<uint16_t *>(p) = value;
        return std::string_view(p, sizeof(value));
    }
    
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
    
    static std::string_view GetString(std::string_view value, ScopedMemory *scope) {
        const size_t size = Varint64::Sizeof(value.size()) + value.size();
        char *const buf = static_cast<char *>(scope->New(size));
        char *s = buf + Varint64::Encode(buf, value.size());
        ::memcpy(s, value.data(), value.size());
        return std::string_view(buf, size);
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
    
    static uint16_t SetU16(std::string_view slice) {
        DCHECK_EQ(sizeof(uint16_t), slice.size());
        return *reinterpret_cast<const uint16_t *>(slice.data());
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
    
    __attribute__ (( __format__ (__printf__, 1, 2)))
    static std::string Sprintf(const char *fmt, ...);
    
    static std::string Vsprintf(const char *fmt, va_list ap);
    
    DISALLOW_ALL_CONSTRUCTORS(Slice);
}; // struct Slice

class BufferReader final {
public:
    BufferReader(std::string_view buf) : buf_(buf) {}
    
    DEF_VAL_GETTER(size_t, position);
    
    char ReadByte() {
        DCHECK(!Eof());
        return buf_[position_++];
    }
    
    uint32_t ReadFixed32() {
        DCHECK(!Eof());
        std::string_view result = buf_.substr(position_, 4);
        position_ += 4;
        return Slice::SetU32(result);
    }
    
    uint64_t ReadFixed64() {
        DCHECK(!Eof());
        std::string_view result = buf_.substr(position_, 8);
        position_ += 8;
        return Slice::SetU64(result);
    }
    
    uint32_t ReadVarint32() {
        size_t varint_len;
        DCHECK(!Eof());
        uint32_t value = Varint32::Decode(&buf_[position_], &varint_len);
        position_ += varint_len;
        return value;
    }
    
    uint64_t ReadVarint64() {
        size_t varint_len;
        DCHECK(!Eof());
        uint64_t value = Varint64::Decode(&buf_[position_], &varint_len);
        position_ += varint_len;
        return value;
    }
    
    std::string_view ReadString() {
        uint64_t len = ReadVarint64();
        return ReadString(len);
    }
    
    std::string_view ReadString(uint64_t len) {
        DCHECK(!Eof());
        std::string_view result = buf_.substr(position_, len);
        position_ += len;
        return result;
    }
    
    bool Eof() { return position_ >= buf_.length(); }
    
private:
    size_t position_ = 0;
    const std::string_view buf_;
}; // class BufferReader
    
} // namespace base
    
} // namespace mai


#endif // MAI_BASE_SLICE_H_
