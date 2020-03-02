#pragma once
#ifndef MAI_LANG_STABLE_SPACE_BUILDER_H_
#define MAI_LANG_STABLE_SPACE_BUILDER_H_

#include "lang/mm.h"

namespace mai {

namespace lang {

class Any;

class GlobalSpaceBuilder {
public:
    static constexpr size_t kAligmentSize = 2;
    
    GlobalSpaceBuilder();

    ~GlobalSpaceBuilder() {
        delete[] spans_;
        delete[] bitmap_;
    }
    
    int Reserve(PlainType type) {
        if (type == kRef) {
            return ReserveRef();
        } else {
            return Reserve(&r_, static_cast<size_t>(type), false/*isref*/);
        }
    }

    int ReserveRef() { return Reserve(&r_, kPointerSize, true/*isref*/); }
    
    int AppendI32(int32_t val) { return Append(val); }
    int AppendU32(uint32_t val) { return Append(val); }
    int AppendI64(int64_t val) { return Append(val); }
    int AppendU64(uint64_t val) { return Append(val); }
    int AppendF32(float val) { return Append(val); }
    int AppendF64(double val) { return Append(val); }
    int AppendAny(Any *val) {
        int location = Reserve(&r_, sizeof(val), true/*isref*/);
        *reinterpret_cast<Any **>(address(location)) = val;
        return location;
    }
    
    Span32 *TakeSpans() {
        Span32 *rv = spans_;
        spans_ = nullptr;
        return rv;
    }

    uint32_t *TakeBitmap() {
        uint32_t *rv = bitmap_;
        bitmap_ = nullptr;
        return rv;
    }
    
    DEF_VAL_GETTER(size_t, capacity);
    DEF_VAL_GETTER(size_t, length);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(GlobalSpaceBuilder);
private:
    struct SpanState {
        size_t used;
        size_t index;
    }; // struct SpanState
    
    template<class T>
    inline int Append(T value) {
        int location = Reserve(&p_, sizeof(value), false/*isref*/);
        *reinterpret_cast<T *>(address(location)) = value;
        return location;
    }
    
    int Reserve(SpanState *state, size_t size, bool isref);
    
    void CheckCapacity();
    
    Address address(int pos) {
        DCHECK_GE(pos, 0);
        DCHECK_LT(pos, length_ * sizeof(Span32));
        return reinterpret_cast<Address>(spans_) + pos;
    }
    
    Span32 *spans_;
    uint32_t *bitmap_;
    size_t capacity_;
    size_t length_ = 0;
    SpanState p_; // primitive type
    SpanState r_; // reference type
}; // class GlobalSpaceBuilder

} // namespace lang

} // namespace mai

#endif // MAI_LANG_STABLE_SPACE_BUILDER_H_
