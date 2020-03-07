#pragma once
#ifndef MAI_LANG_STABLE_SPACE_BUILDER_H_
#define MAI_LANG_STABLE_SPACE_BUILDER_H_

#include "lang/stack-frame.h"
#include "lang/value-inl.h"
#include "lang/mm.h"
#include "lang/type-defs.h"
#include "base/hash.h"
#include <type_traits>
#include <unordered_map>

namespace mai {

namespace lang {

class StackSpaceScope;

class StackSpaceAllocator {
public:
    using Scope = StackSpaceScope;
    static constexpr size_t kOffsetGranularity = kStackOffsetGranularity;
    static constexpr size_t kSizeGranularity = 4;
    static constexpr size_t kSpanSize = sizeof(Span16);
    static constexpr size_t kSlotBase = BytecodeStackFrame::kOffsetHeaderSize;
    
    struct Level {
        int p;
        int r;
    }; // struct Level
    
    StackSpaceAllocator() = default;
    
    DEF_VAL_PROP_RW(Level, level);
    DEF_VAL_GETTER(std::vector<uint32_t>, bitmap);
    DEF_VAL_GETTER(size_t, max_spans);

    void set_level(int p_level, int r_level) {
        level_.p = p_level;
        level_.r = r_level;
    }
    
    int Reserve(size_t size);

    int ReserveRef();
    
    void Fallback(int index, size_t size) {
        DCHECK_EQ(level_.p, index - kSlotBase);
        level_.p -= RoundUp(size, kSizeGranularity);
        while (Test(level_.p) && level_.p > 0) {
            DCHECK_EQ(0, level_.p % kSpanSize);
            level_.p -= kSpanSize;
        }
    }
    
    void FallbackRef(int index) {
        DCHECK_EQ(level_.r, index - kSlotBase);
        level_.r -= kPointerSize;
        while (!Test(level_.r) && level_.r > 0) {
            DCHECK_EQ(0, level_.r % kSpanSize);
            level_.r -= kSpanSize;
        }
    }
    
    uint32_t GetMaxStackSize() const {
        return static_cast<uint32_t>(max_spans_ * kSpanSize + kSlotBase);
    }
    
    friend class StackSpaceScope;
    DISALLOW_IMPLICIT_CONSTRUCTORS(StackSpaceAllocator);
private:
    void AdvanceSpan(bool isref);
    
    bool Test(int level) const {
        size_t index = level / kSpanSize;
        return bitmap_[index / 32] & (1u << (index % 32));
    }
    
    std::vector<uint32_t> bitmap_;
    Level level_{0, 0};
    size_t max_spans_ = 0;
}; // class StackSpaceAllocator


class StackSpaceScope {
public:
    StackSpaceScope(StackSpaceAllocator *stack): stack_(stack), saved_(stack->level()) {}
    ~StackSpaceScope() { stack_->set_level(saved_); }
private:
    StackSpaceAllocator *const stack_;
    StackSpaceAllocator::Level const saved_;
}; // class StackSpaceScope

template<class T>
class StableSpaceBuilder {
public:
    using SpanType = T;
    
    static_assert(std::is_same<T, Span16>::value ||
                  std::is_same<T, Span32>::value ||
                  std::is_same<T, Span64>::value, "T must be span type");
    
    StableSpaceBuilder();

    ~StableSpaceBuilder() {
        delete[] spans_;
        delete[] bitmap_;
    }
    
    int Reserve(size_t size) {
        if (size == 0) {
            return ReserveRef();
        } else {
            return Reserve(&r_, size, false/*isref*/);
        }
    }

    int ReserveRef() { return Reserve(&r_, kPointerSize, true/*isref*/); }
    
    int AppendAny(Any *val) {
        int location = Reserve(&r_, sizeof(val), true/*isref*/);
        *reinterpret_cast<Any **>(address(location)) = val;
        return location;
    }
    
    DEF_PTR_GETTER(T, spans);
    DEF_PTR_GETTER(uint32_t, bitmap);
    DEF_VAL_GETTER(size_t, capacity);
    DEF_VAL_GETTER(size_t, length);
    
    template<class S>
    inline S *offset(int location) { return reinterpret_cast<S *>(address(location)); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(StableSpaceBuilder);
protected:
    struct SpanState {
        size_t used;
        size_t index;
    }; // struct SpanState
    
    template<class V>
    inline int Append(V value) {
        int location = Reserve(&p_, sizeof(value), false/*isref*/);
        *reinterpret_cast<V *>(address(location)) = value;
        return location;
    }
    
    int Reserve(SpanState *state, size_t size, bool isref);
    
    void CheckCapacity();
    
    Address address(int pos) {
        DCHECK_GE(pos, 0);
        DCHECK_LT(pos, length_ * sizeof(T));
        return reinterpret_cast<Address>(spans_) + pos;
    }
    
    T *spans_;
    uint32_t *bitmap_;
    size_t capacity_;
    size_t length_ = 0;
    SpanState p_; // primitive type
    SpanState r_; // reference type
}; // class GlobalSpaceBuilder


class GlobalSpaceBuilder final : public StableSpaceBuilder<Span64> {
public:
    GlobalSpaceBuilder() = default;
    int AppendI32(int32_t val) { return Append(val); }
    int AppendU32(uint32_t val) { return Append(val); }
    int AppendI64(int64_t val) { return Append(val); }
    int AppendU64(uint64_t val) { return Append(val); }
    int AppendF32(float val) { return Append(val); }
    int AppendF64(double val) { return Append(val); }
    
    SpanType *TakeSpans() {
        SpanType *rv = spans_;
        spans_ = nullptr;
        return rv;
    }

    uint32_t *TakeBitmap() {
        uint32_t *rv = bitmap_;
        bitmap_ = nullptr;
        return rv;
    }

    DISALLOW_IMPLICIT_CONSTRUCTORS(GlobalSpaceBuilder);
}; // class ConstantPoolBuilder



class ConstantPoolBuilder final : public StableSpaceBuilder<Span32> {
public:
    ConstantPoolBuilder() = default;

    int FindOrInsertI32(int32_t val) {
        return FindOrInsert({.kind = Key::kP32, .p32 = static_cast<uint32_t>(val)}, val);
    }
    
    int FindOrInsertU32(uint32_t val) {
        return FindOrInsert({.kind = Key::kP32, .p32 = val}, val);
    }

    int FindOrInsertI64(int64_t val) {
        return FindOrInsert({.kind = Key::kP64, .p64 = static_cast<uint64_t>(val)}, val);
    }

    int FindOrInsertU64(uint64_t val) {
        return FindOrInsert({.kind = Key::kP64, .p64 = val}, val);
    }

    int FindOrInsertF32(float val) {
        return FindOrInsert({.kind = Key::kF32, .f32 = val}, val);
    }

    int FindOrInsertF64(double val) {
        return FindOrInsert({.kind = Key::kF64, .f64 = val}, val);
    }
    
    int FindString(std::string_view val) {
        Key key{.kind = Key::kString, .string = val};
        if (auto iter = constants_.find(key); iter == constants_.end()) {
            return -1;
        } else {
            return iter->second;
        }
    }
    
    int FindOrInsertString(String *val) {
        return FindOrInsertAny({
            .kind = Key::kString,
            .string = std::string_view(val->data(), val->length())
        }, val);
    }
    
    int FindOrInsertMetadata(MetadataObject *val) {
        return FindOrInsert({.kind = Key::kMetadata, .metadata = val}, val);
    }
    
    std::vector<Span32> ToSpanVector() const {
        std::vector<Span32> rv(length());
        ::memcpy(&rv[0], spans(), rv.size() * sizeof(Span32));
        return rv;
    }

    std::vector<uint32_t> ToBitmapVector() const {
        std::vector<uint32_t> rv((length() + 31) / 32);
        ::memcpy(&rv[0], bitmap(), rv.size() * sizeof(uint32_t));
        return rv;
    }

private:
    struct Key {
        enum Kind {
            kP32,
            kP64,
            kF32,
            kF64,
            kString,
            kMetadata,
        };
        Kind kind;
        union {
            uint32_t  p32;
            uint64_t  p64;
            float     f32;
            double    f64;
            MetadataObject *metadata;
            std::string_view string;
        };
    };
    
    struct KeyHash : public std::unary_function<size_t, Key> {
        size_t operator () (Key key) const;
    };

    struct KeyEqualTo : public std::binary_function<Key, Key, bool> {
        bool operator () (Key lhs, Key rhs) const;
    };
    
    template<class T>
    int FindOrInsert(const Key &key, T value) {
        if(auto iter = constants_.find(key); iter != constants_.end()) {
            return iter->second;
        }
        int location = Append(value);
        constants_[key] = location;
        return location;
    }
    
    int FindOrInsertAny(const Key &key, Any *value) {
        if(auto iter = constants_.find(key); iter != constants_.end()) {
            return iter->second;
        }
        int location = AppendAny(value);
        constants_[key] = location;
        return location;
    }

    std::unordered_map<Key, int, KeyHash, KeyEqualTo> constants_;
}; // class ConstantPoolBuilder


template<class T>
StableSpaceBuilder<T>::StableSpaceBuilder()
    : spans_(new T[2])
    , capacity_(2)
    , length_(2)
    , bitmap_(new uint32_t[1]) {
    bitmap_[0] = 0x2;
    r_.index = 1;
    r_.used = 0;
    p_.index = 0;
    p_.used = 0;
    ::memset(spans_, 0, sizeof(T) * length_);
}

template<class T>
int StableSpaceBuilder<T>::Reserve(SpanState *state, size_t size, bool isref) {
    if (state->used + size > sizeof(T)) {
        CheckCapacity();
        state->index = std::max(p_.index, r_.index) + 1;
        if (isref) {
            ::memset(spans_ + state->index, 0, sizeof(T));
            bitmap_[state->index / 32] |= 1 << (state->index % 32);
        }
        length_++;
        state->used = 0;
    }
    int location = static_cast<int>(state->index * sizeof(T) + state->used);
    state->used += size;
    return location;
}

template<class T>
void StableSpaceBuilder<T>::CheckCapacity() {
    if (length_ + 1 < capacity_) {
        return;
    }
    size_t old_bmp_size = (capacity_ + 31) / 32;
    capacity_ <<= 1;
    size_t bmp_size = (capacity_ + 31) / 32;
    T *new_spans = new T[capacity_];
    ::memcpy(new_spans, spans_, length_ * sizeof(T));
    uint32_t *new_bmp = new uint32_t[bmp_size];
    ::memcpy(new_bmp, bitmap_, old_bmp_size * sizeof(uint32_t));
    delete [] spans_;
    spans_ = new_spans;
    delete [] bitmap_;
    bitmap_ = new_bmp;
}

} // namespace lang

} // namespace mai

#endif // MAI_LANG_STABLE_SPACE_BUILDER_H_
