#ifndef MAI_CORE_INTERNAL_KEY_COMPARATOR_H_
#define MAI_CORE_INTERNAL_KEY_COMPARATOR_H_

#include "mai/comparator.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {

namespace core {
    
class InternalKeyComparator final : public Comparator {
public:
    InternalKeyComparator(const Comparator *ucmp)
        : ucmp_(DCHECK_NOTNULL(ucmp)) {}
    virtual ~InternalKeyComparator();
    
    virtual int Compare(std::string_view lhs,
                        std::string_view rhs) const override;
    virtual uint32_t Hash(std::string_view value) const override;
    virtual const char* Name() const override;
    virtual void FindShortestSeparator(std::string* start,
                                       std::string_view limit) const override;
    virtual void FindShortSuccessor(std::string* key) const override;
    
    DEF_PTR_GETTER_NOTNULL(const Comparator, ucmp);

    DISALLOW_IMPLICIT_CONSTRUCTORS(InternalKeyComparator);
private:
    const Comparator *const ucmp_;
}; // class InternalKeyComparator
    
} // namespace core

} // namespace mai


#endif // MAI_CORE_INTERNAL_KEY_COMPARATOR_H_
