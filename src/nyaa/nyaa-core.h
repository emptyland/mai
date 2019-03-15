#ifndef MAI_NYAA_NYAA_CORE_H_
#define MAI_NYAA_NYAA_CORE_H_

#include "base/base.h"

namespace mai {
    
namespace nyaa {
    
class Nyaa;
    
class NyaaCore final {
public:
    NyaaCore(Nyaa *stub) {}
    ~NyaaCore() {}
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyaaCore);
private:
    Nyaa *stub_;
};
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_NYAA_CORE_H_
