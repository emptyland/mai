#ifndef MAI_BASE_BIT_OPS_H_
#define MAI_BASE_BIT_OPS_H_

#include "base/base.h"
#include <stdint.h>

namespace mai {

namespace base {

struct Bits {

    // Fast find first zero, right to left
    // Base on binary searching
    static inline int FindFirstZero32(uint32_t x);

    /**
     * Find first one, right to left.
     */
    static inline int FindFirstOne32(uint32_t x);

    /**
     * Count left bit 0 from 32 bits integer.
     */
    static inline int CountLeadingZeros32(uint32_t x);

    /**
     * Count right bit 0 from 32 bits integer.
     */
    static inline int CountTrailingZeros32(uint32_t x);

    /**
     * Count bit 1 from 32 bits integer.
     */
    static inline int CountOne32(uint32_t x);

    /**
     * Count left bit 1 from 64 bits integer.
     */
    static inline int CountLeadingZeros64(uint64_t x);

    /**
     * Count right bit 1 from 64 bits integer.
     */
    static inline int CountTrailingZeros64(uint64_t x);

    DISALLOW_ALL_CONSTRUCTORS(Bits);
}; // struct Bits

/*static*/ inline int Bits::FindFirstZero32(uint32_t x) {
    static const int zval[] = {
        0, /* 0 */ 1, /* 1 */ 0, /* 2 */ 2, /* 3 */
        0, /* 4 */ 1, /* 5 */ 0, /* 6 */ 3, /* 7 */
        0, /* 8 */ 1, /* 9 */ 0, /* a */ 2, /* b */
        0, /* c */ 1, /* d */ 0, /* e */ 4, /* f */
    };

    int base = 0;
    if ((x & 0x0000FFFF) == 0x0000FFFFU) { base += 16; x >>= 16; }
    if ((x & 0x000000FF) == 0x000000FFU) { base +=  8; x >>=  8; }
    if ((x & 0x0000000F) == 0x0000000FU) { base +=  4; x >>=  4; }
    return base + zval[x & 0xFU];
}

/*static*/ inline int Bits::FindFirstOne32(uint32_t x) {
    static const int oval[16] = {
        4, /* 0 */ 0, /* 1 */ 1, /* 2 */ 0, /* 3 */
        2, /* 4 */ 0, /* 5 */ 1, /* 6 */ 0, /* 7 */
        3, /* 8 */ 0, /* 9 */ 1, /* A */ 0, /* B */
        2, /* C */ 0, /* D */ 1, /* E */ 0, /* F */
    };

    auto base = 0;
    if ((x & 0x0000FFFF) == 0) { base += 16; x >>= 16; }
    if ((x & 0x000000FF) == 0) { base += 8;  x >>=  8; }
    if ((x & 0x0000000F) == 0) { base += 4;  x >>=  4; }
    return base + oval[x & 0xF];
}

/*static*/ inline int Bits::CountLeadingZeros32(uint32_t x) {
    static const int zval[16] = {
        4, /* 0 */ 3, /* 1 */ 2, /* 2 */ 2, /* 3 */
        1, /* 4 */ 1, /* 5 */ 1, /* 6 */ 1, /* 7 */
        0, /* 8 */ 0, /* 9 */ 0, /* A */ 0, /* B */
        0, /* C */ 0, /* D */ 0, /* E */ 0  /* F */
    };

    int base = 0;
    if ((x & 0xFFFF0000) == 0) {base  = 16; x <<= 16;} else {base = 0;}
    if ((x & 0xFF000000) == 0) {base +=  8; x <<=  8;}
    if ((x & 0xF0000000) == 0) {base +=  4; x <<=  4;}
    return base + zval[x >> (32-4)];
}

/*static*/ inline int Bits::CountTrailingZeros32(uint32_t x) {
    if (x == 0) return 32;
    auto n = 0;
    if ((x & 0x0000FFFF) == 0) { n += 16; x >>= 16; }
    if ((x & 0x000000FF) == 0) { n +=  8; x >>=  8; }
    if ((x & 0x0000000F) == 0) { n +=  4; x >>=  4; }
    if ((x & 0x00000003) == 0) { n +=  2; x >>=  2; }
    if ((x & 0x00000001) == 0) { n +=  1; }
    return n;
}

/*static*/ inline int Bits::CountOne32(uint32_t x) {
    x = ((0xaaaaaaaa & x) >> 1) + (0x55555555 & x);
    x = ((0xcccccccc & x) >> 2) + (0x33333333 & x);
    x = ((0xf0f0f0f0 & x) >> 4) + (0x0f0f0f0f & x);
    x = ((0xff00ff00 & x) >> 8) + (0x00ff00ff & x);
    x = ((0xffff0000 & x) >>16) + (0x0000ffff & x);

    return static_cast<int>(x);
}

/*static*/ inline int Bits::CountLeadingZeros64(uint64_t x) {
    if ((x & 0xFFFFFFFF00000000ULL) == 0) {
        return 32 + CountLeadingZeros32(static_cast<uint32_t>(x));
    } else {
        x = (x & 0xFFFFFFFF00000000ULL) >> 32;
        return CountLeadingZeros32(static_cast<uint32_t>(x));
    }
}

/*static*/ inline int Bits::CountTrailingZeros64(uint64_t x) {
    if ((x & 0x00000000FFFFFFFFULL) == 0) {
        x = (x & 0x00000000FFFFFFFFULL) << 32;
        return 32 + CountTrailingZeros32(static_cast<uint32_t>(x));
    } else {
        return CountTrailingZeros32(static_cast<uint32_t>(x));
    }
}

} // namespace base

} // namespace mai

#endif // MAI_BASE_BIT_OPS_H_

