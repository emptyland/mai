#include "nyaa/object-factory.h"
#include "nyaa/nyaa-values.h"
#include "base/slice.h"

namespace mai {
    
namespace nyaa {
    

NyString *ObjectFactory::Sprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    auto rv = Vsprintf(fmt, ap);
    va_end(ap);
    return rv;
}

NyString *ObjectFactory::Vsprintf(const char *fmt, va_list ap) {
    std::string s(base::Vsprintf(fmt, ap));
    return NewString(s.data(), s.length(), false);
}
    
NyMap *ObjectFactory::NewMap(uint32_t capacity, uint32_t seed, bool linear, bool old) {
    NyObject *maybe = nullptr;
    if (linear) {
        maybe = NewArray(capacity, nullptr /*base*/, old);
    } else {
        maybe = NewTable(capacity, seed, nullptr /*base*/, old);
    }
    if (!maybe) {
        return nullptr;
    }
    return NewMap(maybe, old);
}
    
} // namespace nyaa
    
} // namespace mai
