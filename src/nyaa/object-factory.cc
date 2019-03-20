#include "nyaa/object-factory.h"
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
    
} // namespace nyaa
    
} // namespace mai
