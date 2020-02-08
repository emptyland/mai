#pragma once
#ifndef MAI_LANG_VALUE_INL_H_
#define MAI_LANG_VALUE_INL_H_

#include "lang/value.h"
#include "lang/metadata.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {

namespace lang {

// Closure's captured-value
class CapturedValue : public AbstractValue {
public:
    // internal functions:
    inline bool is_object_value() const { return padding_ & kObjectBit; }

    inline bool is_primitive_value() const { return !is_object_value(); }

    inline const void *value() const { return address(); }

    inline void *mutable_value() { return address(); }
    
    friend class Machine;
private:
    static constexpr uint32_t kObjectBit = 1;

    // Create a primitive captured-value
    CapturedValue(const Class *clazz, const void *value, size_t n, uint32_t tags)
        : AbstractValue(clazz, value, n, tags) {
        padding_ = 0;
    }
    
    // Create a object captured-value
    CapturedValue(const Class *clazz, Any *value, uint32_t tags)
        : AbstractValue(clazz, &value, sizeof(value), tags) {
        padding_ = kObjectBit;
    }

    // Create a empty captured-value
    CapturedValue(const Class *clazz, bool is_object, uint32_t tags)
        : AbstractValue(clazz, tags) {
        padding_ = is_object ? kObjectBit : 0;
    }
}; // class CapturedValue



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

inline bool Any::QuicklyIs(uint32_t type_id) const { return clazz()->id() == type_id; }

template<class T>
inline void Array<T, true>::quickly_set_nobarrier(size_t i, T value) {
    DCHECK_GE(i, 0);
    DCHECK_LT(i, length_);
    elems_[i] = value;
}

inline bool Closure::is_cxx_function() const { return (tags_ & kClosureMask) == kCxxFunction; }

inline bool Closure::is_mai_function() const { return (tags_ & kClosureMask) == kMaiFunction; }

inline Code *Closure::code() const {
    DCHECK(is_cxx_function());
    return DCHECK_NOTNULL(cxx_fn_);
}

inline Function *Closure::function() const {
    DCHECK(is_mai_function());
    return DCHECK_NOTNULL(mai_fn_);
}

inline Array<String *> *Throwable::stacktrace() const { return stacktrace_; }

inline String *Panic::quickly_message() const { return message_; }

} // namespace lang

} // namespace mai

#endif // MAI_LANG_VALUE_INL_H_
