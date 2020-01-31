#ifndef MAI_LANG_VALUE_INL_H_
#define MAI_LANG_VALUE_INL_H_

#include "lang/value.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {

namespace lang {

inline bool Any::is_forward() const { return klass_ & 1; }

inline Class *Any::clazz() const {
    DCHECK(!is_forward());
    return reinterpret_cast<Class *>(klass_);
}

inline Any *Any::forward() const {
    DCHECK(is_forward());
    return reinterpret_cast<Any *>(klass_ & ~1);
}

inline uint32_t Any::tags() const { return tags_; }

inline bool CapturedValue::is_object_value() const { return padding_ & kObjectBit; }

inline bool CapturedValue::is_primitive_value() const { return !is_object_value(); }

inline const void *CapturedValue::value() const { return address(); }

inline void *CapturedValue::mutable_value() { return address(); }

} // namespace lang

} // namespace mai

#endif // MAI_LANG_VALUE_INL_H_
