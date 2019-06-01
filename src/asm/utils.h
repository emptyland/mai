#ifndef MAI_ASM_UTILS_H_
#define MAI_ASM_UTILS_H_

#include "base/base.h"
#include <stddef.h>
#include <type_traits>

namespace mai {
    
namespace arch {
    
template<class T, class O = ptrdiff_t>
class ObjectTemplate {
public:
    using type = T;
    using offset_type = O;
    
    template<class F>
    offset_type Offset(F T::*const field) {
        static_assert(sizeof(field) == kPointerSize, "incorrect size");
        return static_cast<offset_type>(bit_cast<Address>(field) - static_cast<Address>(nullptr));
    }
    
    template<class F>
    Address Method(F T::*const method) { return MethodAddress(method); }
    
    template<class E, class F>
    offset_type Between(E T::*const f0, F T::*const f1) {
        static_assert(sizeof(f0) == kPointerSize && sizeof(f1) == kPointerSize, "incorrect size");
        return static_cast<offset_type>(bit_cast<Address>(f0) - bit_cast<Address>(f1));
    }
    
    template<class F>
    static constexpr offset_type OffsetOf(F T::*const field) {
        static_assert(sizeof(field) == kPointerSize, "incorrect size");
        return static_cast<offset_type>(bit_cast<Address>(field) - static_cast<Address>(nullptr));
    }
    
    template<class F>
    static constexpr Address MethodAddress(F T::* method) {
        static_assert(std::is_function<F>::value, "not a method");
        auto bundle = reinterpret_cast<Address *>(&method);
        return bundle[0];
    }
    
}; //template<class T, class O> class ObjectTemplate
    
} // namespace arch
    
} // namespace mai


#endif // MAI_ASM_UTILS_H_
