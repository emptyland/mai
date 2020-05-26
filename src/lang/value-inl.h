#pragma once
#ifndef MAI_LANG_VALUE_INL_H_
#define MAI_LANG_VALUE_INL_H_

#include "lang/value.h"
#include "lang/metadata.h"
#include "lang/object-visitor.h"
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

// The stub class for user-defined-object
// Support some useful method.
class Object : public Any {
public:
    void Iterate(ObjectVisitor *visitor);

    String *ToString() const;
private:
    Address GetFieldAddress(const Field *field) {
        return reinterpret_cast<Address>(this) + field->offset();
    }
}; // class Object
static_assert(sizeof(Any) == sizeof(Object), "Incorrect Object size");

inline bool Any::is_forward() const { return klass_ & 1; }

inline bool Any::is_directly() const { return !is_forward(); }

inline Class *Any::clazz() const {
    DCHECK(!is_forward());
    return reinterpret_cast<Class *>(klass_);
}

inline void Any::set_clazz(const Class *clazz) {
    DCHECK(!is_forward());
    klass_ = reinterpret_cast<uintptr_t>(DCHECK_NOTNULL(clazz));
}

inline Any *Any::forward() const {
    return is_forward() ? reinterpret_cast<Any *>(klass_ & ~1) : nullptr;
}

inline void Any::set_forward(Any *addr) { set_forward_address(reinterpret_cast<Address>(addr)); }

inline void Any::set_forward_address(Address addr) {
    uintptr_t word = reinterpret_cast<uintptr_t>(addr);
    DCHECK((word & 1) == 0);
    klass_ = word | 1;
}

inline int Any::color() const { return tags_ & kColorMask; }

inline void Any::set_color(int color) { tags_ = (tags_ & ~kColorMask) | color; }

inline uint32_t Any::tags() const { return tags_; }

inline bool Any::QuicklyIs(uint32_t type_id) const { return clazz()->id() == type_id; }

template<class T>
inline T Any::UnsafeGetField(const Field *field) const {
    const void *addr = reinterpret_cast<const uint8_t *>(this) + field->offset();
    return *static_cast<T const*>(addr);
}

template<class T>
inline T *Any::UnsafeAccess(const Field *field) {
    void *addr = reinterpret_cast<Address>(this) + field->offset();
    return static_cast<T *>(addr);
}

template<class T>
inline void Any::UnsafeSetField(const Field *field, T value) {
    void *addr = reinterpret_cast<Address>(this) + field->offset();
    *static_cast<T *>(addr) = value;
    // TODO Barriar
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
inline void Array<T, R>::QuicklyAppendNoResize(T data) {
    DCHECK_LE(length_ + 1, capacity_);
    elems_[length_++] = data;
}

template<class T, bool R>
inline void Array<T, R>::QuicklyAppendNoResize(const T *data, size_t n) {
    DCHECK_LT(length_ + n, capacity_);
    ::memcpy(&elems_[length_], data, n);
    length_ += n;
}

template<class T, bool R>
inline T *Array<T, R>::QuicklyAdvanceNoResize(size_t n) {
    DCHECK_LT(length_ + n, capacity_);
    T *base = &elems_[length_];
    length_ += n;
    return base;
}

template<class T>
inline void Array<T, true>::quickly_set(size_t i, T value) {
    quickly_set_nobarrier(i, value);
    WriteBarrier(reinterpret_cast<Any **>(&elems_[i]));
}

template<class T>
inline void Array<T, true>::quickly_set_nobarrier(size_t i, T value) {
    DCHECK_GE(i, 0);
    DCHECK_LT(i, length_);
    elems_[i] = value;
}

template<class T>
inline void Array<T, true>::QuicklySetAll(size_t i, T *value, size_t n) {
    DCHECK_GE(i, 0);
    DCHECK_LE(i + n, length_);
    ::memcpy(&elems_[i], value, sizeof(T) * n);
    WriteBarrier(reinterpret_cast<Any **>(&elems_[i]), n);
}

template<class T>
inline void Array<T, true>::QuicklyAppend(T value) {
    DCHECK_LE(length_ + 1, capacity_);
    elems_[length_] = value;
    WriteBarrier(reinterpret_cast<Any **>(&elems_[length_]));
    length_++;
}

template<class T>
inline T Array<T, true>::quickly_get(size_t i) const {
    DCHECK_GE(i, 0);
    DCHECK_LT(i, length_);
    return elems_[i];
}

template<class T>
inline void Array<T, true>::Iterate(ObjectVisitor *visitor) {
    visitor->VisitPointers(this, &elems_[0], &elems_[length_]);
}

template<class K>
template<class V>
inline ImplementMap<K> *ImplementMap<K>::UnsafePut(K key, V value) {
    ImplementMap<K> *dummy = nullptr;
    Entry *room = FindOrMakeRoom(key, &dummy);
    if (!room) {
        return nullptr;
    }
    room->key = key;
    if (ElementTraits<K>::kIsReferenceType) {
        dummy->WriteBarrier(reinterpret_cast<Any **>(&room->key));
    }
    *reinterpret_cast<V *>(&room->value) = value;
    if (IsValueReferenceType()) {
        dummy->WriteBarrier(reinterpret_cast<Any **>(&room->value));
    }
    return dummy;
}

template<class K>
template<class V>
inline V ImplementMap<K>::UnsafeGet(K key) {
    Entry *room = FindForGet(key);
    return !room ? V(0) : *reinterpret_cast<V *>(&room->value);
}

template<class K>
inline void ImplementMap<K>::UnsafeSet(K key, uintptr_t value) {
    if (Entry *room = FindForPut(key)) {
        room->key = key;
        if (ElementTraits<K>::kIsReferenceType) {
            WriteBarrier(reinterpret_cast<Any **>(&room->key));
        }
        room->value = value;
        if (IsValueReferenceType()) {
            WriteBarrier(reinterpret_cast<Any **>(&room->value));
        }
    }
}

template<class K, class V>
inline void Map<K, V, false, true>::Iterate(ObjectVisitor *visitor) {
    typename ImplementMap<K>::Iterator iter(this);
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        //printf("value: %p\n", iter->value);
        visitor->VisitPointer(this, reinterpret_cast<Any **>(&iter->value));
    }
}

template<class K, class V>
inline void Map<K, V, true, true>::Iterate(ObjectVisitor *visitor) {
    typename ImplementMap<K>::Iterator iter(this);
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        visitor->VisitPointer(this, reinterpret_cast<Any **>(&iter->key));
        visitor->VisitPointer(this, reinterpret_cast<Any **>(&iter->value));
    }
}

template<class K, class V>
inline void Map<K, V, true, false>::Iterate(ObjectVisitor *visitor) {
    typename ImplementMap<K>::Iterator iter(this);
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        visitor->VisitPointer(this, reinterpret_cast<Any **>(&iter->key));
    }
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

inline void Closure::Iterate(ObjectVisitor *visitor) {
    visitor->VisitPointers(this, reinterpret_cast<Any **>(&captured_var_[0]),
                           reinterpret_cast<Any **>(&captured_var_[captured_var_size_]));
}

inline Array<String *> *Throwable::stacktrace() const { return stacktrace_; }

inline void Throwable::QuickSetStacktrace(Array<String *> *stacktrace) {
    stacktrace_ = stacktrace;
    WriteBarrier(reinterpret_cast<Any **>(&stacktrace_));
}

inline String *Panic::quickly_message() const { return message_; }

inline String *IncrementalStringBuilder::QuickBuild() const { return Finish(); }

} // namespace lang

} // namespace mai

#endif // MAI_LANG_VALUE_INL_H_
