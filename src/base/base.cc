#include "base/base.h"
#include "glog/logging.h"
#include <type_traits>

namespace mai {

template<class T>
inline void *RoundBytesFill(const T zag, void *chunk, size_t n) {
    static_assert(sizeof(zag) > 1 && sizeof(zag) <= 8, "T must be int16,32,64");
    static_assert(std::is_integral<T>::value, "T must be int16,32,64");

    DCHECK_GE(n, 0);
    auto result = chunk;
    auto round = n / sizeof(T);
    while (round--) {
        auto round_bits = static_cast<T *>(chunk);
        *round_bits = zag;
        chunk = static_cast<void *>(round_bits + 1);
    }

    auto zag_bytes = reinterpret_cast<const uint8_t *>(&zag);

    round = n % sizeof(T);
    while (round--) {
        auto bits8 = static_cast<uint8_t *>(chunk);
        *bits8 = *zag_bytes++;
        chunk = static_cast<void *>(bits8 + 1);
    }
    return result;
}

void *Round16BytesFill(const uint16_t zag, void *chunk, size_t n) {
    return RoundBytesFill<uint16_t>(zag, chunk, n);
}

void *Round32BytesFill(const uint32_t zag, void *chunk, size_t n) {
    return RoundBytesFill<uint32_t>(zag, chunk, n);
}

void *Round64BytesFill(const uint64_t zag, void *chunk, size_t n) {
    return RoundBytesFill<uint64_t>(zag, chunk, n);
}

} // namespace mai
