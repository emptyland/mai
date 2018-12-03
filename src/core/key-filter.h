#ifndef MAI_CORE_KEY_FILTER_H_
#define MAI_CORE_KEY_FILTER_H_

#include "reference-count.h"
#include "base/base.h"
#include <string>
#include <string_view>

namespace mai {
    
namespace core {
    
class KeyFilter : public base::ReferenceCountable {
public:
    KeyFilter() {}
    virtual ~KeyFilter() {}
    
    virtual bool MayExists(std::string_view key) const = 0;
    
    virtual bool EnsureNotExists(std::string_view key) const {
        return !MayExists(key);
    }
    
    virtual size_t ApproximateCount() const = 0;
    
    virtual size_t memory_usage() const = 0;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(KeyFilter);
}; // class KeyFilter
    
} // namespace core
    
} // namespace mai


#endif // MAI_CORE_KEY_FILTER_H_
