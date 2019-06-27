#ifndef MAI_BASE_SLICE_H_
#define MAI_BASE_SLICE_H_

#include "base/varint-encoding.h"
#include "base/allocators.h"
#include "glog/logging.h"
#include <stdarg.h>
#include <string_view>

namespace mai {
    
namespace base {
    
class Slice final {
public:
    static std::string_view GetByte(char value, ScopedMemory *scope) {
        char *p = static_cast<char *>(scope->Allocate(sizeof(value)));
        *p = value;
        return std::string_view(p, sizeof(value));
    }
    
    static std::string_view GetU16(uint16_t value, ScopedMemory *scope) {
        char *p = static_cast<char *>(scope->Allocate(sizeof(value)));
        *reinterpret_cast<uint16_t *>(p) = value;
        return std::string_view(p, sizeof(value));
    }
    
    static std::string_view GetU32(uint32_t value, ScopedMemory *scope) {
        char *p = static_cast<char *>(scope->Allocate(sizeof(value)));
        *reinterpret_cast<uint32_t *>(p) = value;
        return std::string_view(p, sizeof(value));
    }
    
    static std::string_view GetU64(uint64_t value, ScopedMemory *scope) {
        char *p = static_cast<char *>(scope->Allocate(sizeof(value)));
        *reinterpret_cast<uint64_t *>(p) = value;
        return std::string_view(p, sizeof(value));
    }
    
    static std::string_view GetString(std::string_view value, ScopedMemory *scope) {
        const size_t size = Varint64::Sizeof(value.size()) + value.size();
        char *const buf = static_cast<char *>(scope->Allocate(size));
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
        char *p = static_cast<char *>(scope->Allocate(size));
        ::memset(p, 0, size);
        return std::string_view(p, size);
    }
    
    static std::string_view GetV32(uint32_t value, ScopedMemory *scope) {
        char *p = static_cast<char *>(scope->Allocate(Varint32::kMaxLen));
        size_t n = Varint32::Encode(p, value);
        return std::string_view(p, n);
    }
    
    static std::string_view GetV64(uint64_t value, ScopedMemory *scope) {
        char *p = static_cast<char *>(scope->Allocate(Varint64::kMaxLen));
        size_t n = Varint64::Encode(p, value);
        return std::string_view(p, n);
    }
    
    static void WriteFixed32(std::string *buf, uint32_t value) {
        buf->append(reinterpret_cast<const char *>(&value), sizeof(value));
    }
    
    static void WriteFixed64(std::string *buf, uint64_t value) {
        buf->append(reinterpret_cast<const char *>(&value), sizeof(value));
    }
    
    static void WriteVarint32(std::string *buf, uint32_t value) {
        char tmp[Varint32::kMaxLen];
        auto n = Varint32::Encode(tmp, value);
        buf->append(tmp, n);
    }

    static void WriteVarint64(std::string *buf, uint64_t value) {
        char tmp[Varint64::kMaxLen];
        auto n = Varint64::Encode(tmp, value);
        buf->append(tmp, n);
    }
    
    static void WriteString(std::string *buf, std::string_view str) {
        WriteVarint64(buf, str.size());
        buf->append(str);
    }
    
    static uint16_t SetFixed16(std::string_view slice) {
        DCHECK_EQ(sizeof(uint16_t), slice.size());
        return *reinterpret_cast<const uint16_t *>(slice.data());
    }
    
    static uint32_t SetFixed32(std::string_view slice) {
        DCHECK_EQ(sizeof(uint32_t), slice.size());
        return *reinterpret_cast<const uint32_t *>(slice.data());
    }
    
    static uint64_t SetFixed64(std::string_view slice) {
        DCHECK_EQ(sizeof(uint64_t), slice.size());
        return *reinterpret_cast<const uint64_t *>(slice.data());
    }
    
    static std::string ToReadable(std::string_view raw);
    
    // return:
    // 0 = not a number
    // 'o' = octal
    // 'd' = decimal
    // 's' = signed decimal
    // 'h' = hexadecimal
    // 'f' = float
    // 'e' = float with exp
    static int LikeNumber(const char *s) { return LikeNumber(s, !s ? 0 : strlen(s)); }
    
    static int LikeNumber(const char *s, size_t n);
    
    // return:
    //  0 = parse ok
    // > 0 = overflow
    // < 0 = bad char
    static int ParseI64(const char *s, int64_t *val) {
        return ParseI64(s, !s ? 0 : strlen(s), val);
    }
    
    static int ParseI64(const char *s, size_t n, int64_t *val);
    
    // return:
    //  0 = parse ok
    // > 0 = overflow
    // < 0 = bad char
    static int ParseU64(const char *s, uint64_t *val) {
        return ParseU64(s, !s ? 0 : strlen(s), val);
    }
    
    static int ParseU64(const char *s, size_t n, uint64_t *val);
    
    // return:
    //  0 = parse ok
    // > 0 = overflow
    // < 0 = bad char
    static int ParseH64(const char *s, uint64_t *val) {
        return ParseH64(s, !s ? 0 : strlen(s), val);
    }
    
    static int ParseH64(const char *s, size_t n, uint64_t *val);
    
    // return:
    //  0 = parse ok
    // > 0 = overflow
    // < 0 = bad char
    static int ParseO64(const char *s, uint64_t *val) {
        return ParseO64(s, !s ? 0 : strlen(s), val);
    }
    
    static int ParseO64(const char *s, size_t n, uint64_t *val);

    // return:
    //  0 = parse ok
    // > 0 = overflow
    // < 0 = bad char
    static int ParseI32(const char *s, int32_t *val) {
        return ParseI32(s, !s ? 0 : strlen(s), val);
    }
    
    static int ParseI32(const char *s, size_t n, int32_t *val);
    
    // return:
    // 0 = not a date/date-time/time
    // 'c' = date-time : 0000-00-00 00:00:00
    // 'd' = date      : 0000-00-00
    // 't' = time      : 00:00:00
    static int LikeDateTime(const char *s) {
        return LikeDateTime(s, !s ? 0 : ::strlen(s));
    }
    
    static int LikeDateTime(const char *s, size_t n) { return 0; }
    
    // escape char:
    // \a
    // \b
    // \f
    // \n
    // \r
    // \t
    // \v
    // \\ -> \
    // \' -> '
    // \" -> "
    // \dddd - oct
    // \xhh -> hex
    // \uuuuu -> unicode
    static int ParseEscaped(const char *s, std::string *rv) {
        return ParseEscaped(s, !s ? 0 : ::strlen(s), rv);
    }
    
    static int ParseEscaped(const char *s, size_t n, std::string *rv);
    
    DISALLOW_ALL_CONSTRUCTORS(Slice);
}; // struct Slice

class BufferReader final {
public:
    explicit BufferReader(std::string_view buf) : buf_(buf) {}
    BufferReader(const char *s, size_t n) : buf_(s, n) {}
    
    DEF_VAL_GETTER(size_t, position);
    
    char ReadByte() {
        DCHECK(!Eof());
        return buf_[position_++];
    }
    
    uint32_t ReadFixed32() {
        DCHECK(!Eof());
        std::string_view result = buf_.substr(position_, 4);
        position_ += 4;
        return Slice::SetFixed32(result);
    }
    
    uint64_t ReadFixed64() {
        DCHECK(!Eof());
        std::string_view result = buf_.substr(position_, 8);
        position_ += 8;
        return Slice::SetFixed64(result);
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
        if (len == 0) {
            return "";
        }
        return ReadString(len);
    }
    
    std::string_view ReadString(uint64_t len) {
        DCHECK(!Eof());
        std::string_view result = buf_.substr(position_, len);
        position_ += len;
        return result;
    }
    
    bool Eof() { return position_ >= buf_.length(); }
    
    std::string_view Remaining() const {
        std::string_view buf(buf_);
        buf.remove_prefix(position_);
        return buf;
    }
    
private:
    size_t position_ = 0;
    const std::string_view buf_;
}; // class BufferReader
    
__attribute__ (( __format__ (__printf__, 1, 2)))
std::string Sprintf(const char *fmt, ...);

std::string Vsprintf(const char *fmt, va_list ap);
    
// Round bytes filling
// For int16, 32, 64 filling:
void *Round16BytesFill(const uint16_t zag, void *chunk, size_t n);
void *Round32BytesFill(const uint32_t zag, void *chunk, size_t n);
void *Round64BytesFill(const uint64_t zag, void *chunk, size_t n);
    
} // namespace base
    
} // namespace mai


#endif // MAI_BASE_SLICE_H_
