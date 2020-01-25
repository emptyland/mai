#ifndef MAI_LANG_VALUE_INL_H_
#define MAI_LANG_VALUE_INL_H_

#include "lang/value.h"
#include "base/base.h"

namespace mai {

namespace lang {

inline bool Any::is_forward() const { return forward_ & 1; }

inline Class *Any::clazz() const {
    DCHECK(!is_forward());
    return reinterpret_cast<Class *>(forward_);
}

inline Any *Any::forward() const {
    DCHECK(is_forward());
    return reinterpret_cast<Any *>(forward_ & ~1);
}

inline uint32_t Any::tags() const { return tags_; }

} // namespace lang

} // namespace mai

#endif // MAI_LANG_VALUE_INL_H_
