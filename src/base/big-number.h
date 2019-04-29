#ifndef MAI_BASE_BIG_NUMBER_H_
#define MAI_BASE_BIG_NUMBER_H_

#include "base/base.h"

namespace mai {
    
namespace base {
    
struct Big {
    static constexpr const int kMinRadix = 2;
    static constexpr const int kMaxRadix = 16;

    static int Compare(View<uint32_t> lhs, View<uint32_t> rhs);
    
    static void PrimitiveShl(MutView<uint32_t> vals, int n);
    static void PrimitiveShr(MutView<uint32_t> vals, int n);
    
    static uint32_t Add(View<uint32_t> lhs, View<uint32_t> rhs, MutView<uint32_t> rv);
    static uint32_t Sub(View<uint32_t> lhs, View<uint32_t> rhs, MutView<uint32_t> rv);

    static uint32_t BasicMul(View<uint32_t> x, View<uint32_t> y, MutView<uint32_t> rv);
    static void DivWord(uint64_t n, uint64_t d, uint32_t rv[2]);
    static uint32_t MulSub(MutView<uint32_t> q, MutView<uint32_t> a, uint32_t x,
                    int64_t offset);
    static uint32_t DivAdd(MutView<uint32_t> a, MutView<uint32_t> r, size_t offset);
    static uint32_t DivWord(View<uint32_t> lhs, uint32_t rhs, MutView<uint32_t> rv);
    
    static size_t GetNumberOfDigitals(size_t bits, int radix);
    static size_t GetNumberOfBits(size_t digitals_count, int radix);
    
    static uint32_t Char2Digital(int c);

    DISALLOW_ALL_CONSTRUCTORS(Big);
}; // struct Big

    
} // namespace base
    
} // namespace mai


#endif // MAI_BASE_BIG_NUMBER_H_
