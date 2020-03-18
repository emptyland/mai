#pragma once
#ifndef MAI_LANG_VALUE_INL_H_
#define MAI_LANG_VALUE_INL_H_

#include "lang/value.h"
#include "lang/metadata.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {

namespace lang {

class Channel;

// Closure's captured-value
class CapturedValue : public AbstractValue {
public:
    bool is_object_value() const { return padding_ & kObjectBit; }

    bool is_primitive_value() const { return !is_object_value(); }

    const void *value() const { return address(); }

    void *mutable_value() { return address(); }
    
    template<class T>
    inline T unsafe_typed_value() const { return *static_cast<const T *>(address()); }
    
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
        WriteBarrier(reinterpret_cast<Any **>(address()));
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

inline void Any::set_clazz(const Class *clazz) {
    DCHECK(!is_forward());
    klass_ = reinterpret_cast<uintptr_t>(DCHECK_NOTNULL(clazz));
}

inline Any *Any::forward() const {
    DCHECK(is_forward());
    return reinterpret_cast<Any *>(klass_ & ~1);
}

inline uint32_t Any::tags() const { return tags_; }

inline bool Any::QuicklyIs(uint32_t type_id) const { return clazz()->id() == type_id; }

template<class T>
inline T Any::UnsafeGetField(const Field *field) const {
    const void *addr = reinterpret_cast<const uint8_t *>(this) + field->offset();
    return *static_cast<T const*>(addr);
}

template<class T, bool R>
inline T Array<T, R>::quickly_get(size_t i) const {
    DCHECK_GE(i, 0);
    DCHECK_LT(i, length_);
    return elems_[i];
}

template<class T, bool R>
inline void Array<T, R>::quickly_set(size_t i, T value) {
    DCHECK_GE(i, 0);
    DCHECK_LT(i, length_);
    elems_[i] = value;
}

template<class T, bool R>
inline void Array<T, R>::quickly_set_length(size_t length) {
    DCHECK_LT(length_, capacity_);
    length_ = static_cast<uint32_t>(length);
}

template<class T, bool R>
inline void Array<T, R>::QuicklyAppendNoResize(const T *data, size_t n) {
    DCHECK_LT(length_ + n, capacity_);
    ::memcpy(&elems_[length_], data, n);
    length_ += n;
}

template<class T, bool R>
inline T *Array<T, R>::QuicklyAppendNoResize(size_t n) {
    DCHECK_LT(length_ + n, capacity_);
    T *base = &elems_[length_];
    length_ += n;
    return base;
}

template<class T>
inline void Array<T, true>::quickly_set_nobarrier(size_t i, T value) {
    DCHECK_GE(i, 0);
    DCHECK_LT(i, length_);
    elems_[i] = value;
}

template<class T>
inline T Array<T, true>::quickly_get(size_t i) const {
    DCHECK_GE(i, 0);
    DCHECK_LT(i, length_);
    return elems_[i];
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

inline void Closure::SetCapturedVar(uint32_t i, CapturedValue *value) {
    set_captured_var_no_barrier(i, value);
    WriteBarrier(reinterpret_cast<Any **>(&captured_var_[i].value));
}

inline void Closure::set_captured_var_no_barrier(uint32_t i, CapturedValue *value) {
    DCHECK_LT(i, captured_var_size_);
    captured_var_[i].value = DCHECK_NOTNULL(value);
}

inline CapturedValue *Closure::captured_var(uint32_t i) const {
    DCHECK_LT(i, captured_var_size_);
    return DCHECK_NOTNULL(captured_var_[i].value);
}

inline Array<String *> *Throwable::stacktrace() const { return stacktrace_; }

inline void Throwable::QuickSetStacktrace(Array<String *> *stacktrace) {
    stacktrace_ = stacktrace;
    WriteBarrier(reinterpret_cast<Any **>(&stacktrace_));
}

inline String *Panic::quickly_message() const { return message_; }

inline String *Exception::quickly_message() const { return message_; }

inline Exception *Exception::quickly_cause() const { return cause_; }

inline String *IncrementalStringBuilder::QuickBuild() const { return Finish(); }

} // namespace lang

} // namespace mai

#endif // MAI_LANG_VALUE_INL_H_
