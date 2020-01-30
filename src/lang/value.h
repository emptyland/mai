#ifndef MAI_LANG_VALUE_H_
#define MAI_LANG_VALUE_H_

#include "lang/type-defs.h"
#include "lang/handle.h"
#include <stdint.h>
#include <string.h>
#include <type_traits>

namespace mai {

namespace lang {

class Class;
class Type;

// All heap object's base class.
class Any {
public:
    // internal functions:
    inline bool is_forward() const;
    inline Class *clazz() const;
    inline Any *forward() const;
    inline uint32_t tags() const;

    Any(const Any &) = delete;
    void operator = (const Any &) = delete;
    
    //void *operator new (size_t) = delete;
    void operator delete (void *) = delete;
protected:
    Any(const Class *clazz, uint32_t tags)
        : forward_(reinterpret_cast<uintptr_t>(clazz))
        , tags_(tags) {}
    
    // Write-Barrier for heap object writting
    bool WriteBarrier(Any **address) { return WriteBarrier(address, 1) == 1; }
    int WriteBarrier(Any **address, size_t n);
    
    uintptr_t forward_; // Forward pointer or Class pointer
    uint32_t tags_; // Tags for heap object
}; // class Any


// All array bese class
class AbstractArray : public Any {
public:
    static constexpr size_t kAppendIndex = -1;
    static const int32_t kOffsetCapacity;
    static const int32_t kOffsetLength;
    
    // How many elems to allocation
    uint32_t capacity() const { return capacity_; }

    // Number of elements
    uint32_t length() const { return length_; }

    friend class Machine;
protected:
    AbstractArray(const Class *clazz, uint32_t capacity, uint32_t length)
        : Any(clazz, 0)
        , capacity_(capacity)
        , length_(length) {
    }
    
    static AbstractArray *NewArray(BuiltinType type, size_t length);

    static AbstractArray *NewArrayCopied(const AbstractArray *origin, size_t increment);
    
    uint32_t capacity_;
    uint32_t length_;
}; // class AbstractArray


template<class T>
struct ElementTraits {
    using CoreType = typename std::remove_pointer<T>::type;
    
    static constexpr bool kIsReferenceType =
        std::is_base_of<Any, CoreType>::value || std::is_same<Any, CoreType>::value;
}; // template<class T> struct ElementTraits


// The Array's header
template<class T, bool R = ElementTraits<T>::kIsReferenceType >
class Array : public AbstractArray {
public:
    static constexpr int32_t kOffsetElems = offsetof(Array, elems_);
    
    // Create new array with initialize-elements and length
    static inline Handle<Array<T>> NewImmutable(const T *elems, size_t length) {
        Handle<Array<T>> array(NewImmutable(length));
        for (size_t i = 0; i < length; i++) {
            array->elems_[i] = elems[i];
        }
        return array;
    }

    // Create new array with length
    static inline Handle<Array<T>> NewImmutable(size_t length) {
        Handle<AbstractArray> abstract(NewArray(TypeTraits<Array<T>>::kType, length));
        if (abstract.is_empty()) {
            return Handle<Array<T>>::Empty();
        }
        return Handle<Array<T>>(static_cast<Array<T> *>(*abstract));
    }

    inline T At(size_t i) const { return (i < length_) ? elems_[i] : T(); }

    // The + operator:
    inline Handle<Array<T>> Plus(size_t index, T elem) const {
        Handle<AbstractArray> abstract(NewArrayCopied(this, index == kAppendIndex ? 1 : 0));
        if (abstract.is_empty() || abstract.is_value_null()) {
            return Handle<Array<T>>::Empty();
        }
        Handle<Array<T>> copied(static_cast<Array<T> *>(*abstract));
        if (index == kAppendIndex) {
            copied->elems_[length_] = elem;
        } else {
            copied->elems_[index] = elem;
        }
        return copied;
    }

    // The - operator:
    Handle<Array<T>> Minus(size_t index) const {
        if (index > length_) {
            return Handle<Array<T>>(const_cast<Array<T> *>(this));
        }
        Handle<Array<T>> copied(NewImmutable(length() - 1));
        ::memcpy(copied->elems_, elems_, index * sizeof(T));
        ::memcpy(copied->elems_ + index, elems_ + index + 1, (length_ - 1 - index) * sizeof(T));
        return copied;
    }

    friend class Machine;
protected:
    Array(const Class *clazz, uint32_t capacity, uint32_t length)
        : AbstractArray(clazz, capacity, length) {
        ::memset(elems_, 0, sizeof(T) * length);
    }

    T elems_[0];
}; //template<class T> class Array


template<class T>
class Array<T, true> : public AbstractArray {
public:
    //using Any::WriteBarrier;
    using CoreType = typename ElementTraits<T>::CoreType;

    static constexpr int32_t kOffsetElems = offsetof(Array, elems_);

    // Create new array with initialize-elements and length
    static inline Handle<Array<T>> NewImmutable(Handle<CoreType> elems[], size_t length) {
        Handle<Array<T>> array(NewImmutable(length));
        for (size_t i = 0; i < length; i++) {
            array->elems_[i] = *elems[i];
        }
        array->WriteBarrier(reinterpret_cast<Any **>(array->elems_), length);
        return array;
    }

    // Create new array with length
    static inline Handle<Array<T>> NewImmutable(size_t length) {
        Handle<Any> any(NewArray(TypeTraits<Array<T>>::kType, length));
        if (any.is_empty()) {
            return Handle<Array<T>>::Empty();
        }
        return Handle<Array<T>>(static_cast<Array<T> *>(*any));
    }

    // Accessor
    Handle<CoreType> At(size_t i) const {
        return (i < length_) ? Handle<CoreType>(elems_[i]) : Handle<CoreType>::Empty();
    }

    // The + operator:
    Handle<Array<T>> Plus(size_t index, Handle<CoreType> elem) const {
        Handle<AbstractArray> abstract(NewArrayCopied(this, index == kAppendIndex ? 1 : 0));
        if (abstract.is_empty() || abstract.is_value_null()) {
            return Handle<Array<T>>::Empty();
        }
        Handle<Array<T>> copied(static_cast<Array<T> *>(*abstract));
        if (index == kAppendIndex) {
            copied->elems_[length_] = *elem;
            copied->WriteBarrier(reinterpret_cast<Any **>(&copied->elems_[length_]));
        } else {
            copied->elems_[index] = *elem;
            copied->WriteBarrier(reinterpret_cast<Any **>(&copied->elems_[index]));
        }
        return copied;
    }

    // The - operator:
    Handle<Array<T>> Minus(size_t index) const {
        if (index > length_) {
            return Handle<Array<T>>(this);
        }
        Handle<Array> copied(NewImmutable(length_ - 1));
        ::memcpy(copied->elems_, elems_, index * sizeof(T));
        ::memcpy(copied->elems_ + index, elems_ + index + 1, (length_ - 1 - index) * sizeof(T));
        copied->WriteBarrier(reinterpret_cast<Any **>(copied->elems_), copied->length_);
        return copied;
    }

    friend class Machine;
protected:
    Array(const Class *clazz, uint32_t capacity, uint32_t length)
        : AbstractArray(clazz, capacity, length) {
        ::memset(elems_, 0, sizeof(T) * length);
    }

    T elems_[0]; // [strong ref]
}; // template<class T> class Array<T, true>



// The string header
class String : public Array<char> {
public:
    static Handle<String> NewUtf8(const char *utf8_string, size_t n);

    static Handle<String> NewUtf8(const char *utf8_string) {
        return NewUtf8(utf8_string, strlen(utf8_string));
    }

    using Array<char>::capacity;
    using Array<char>::length;
    using Array<char>::elems_;

    const char *data() const { return elems_; }

    friend class Machine;
private:
    String(const Class *clazz, uint32_t capacity, const char *utf8_string, uint32_t length)
        : Array<char>(clazz, capacity, length) {
        ::memcpy(elems_, utf8_string, length);
        elems_[length] = '\0'; // for c-style string
    }
    
    static constexpr size_t RequiredSize(size_t capacity) {
        return sizeof(String) + capacity;
    }
}; // class String

// The Map's header
template<class T>
class Map : public Any {
public:
    static constexpr int32_t kOffsetBucketSize = offsetof(Map, bucket_shift_);
    static constexpr int32_t kOffsetLength = offsetof(Map, length_);
    static constexpr int32_t kOffsetRandomSeed = offsetof(Map, random_seed_);
    static constexpr int32_t kOffsetEntries = offsetof(Map, entries_);
    
    uint32_t bucket_size() const { return 1u << bucket_shift_; }
private:
    static constexpr size_t kPaddingSize = sizeof(T) % 4;
    
    inline Map(const Class *clazz, uint32_t initial_bucket_shift, uint32_t random_seed)
        : Any(clazz, 0)
        , bucket_shift_(initial_bucket_shift)
        , length_(0)
        , random_seed_(random_seed)
        , free_(1u << initial_bucket_shift) {
        ::memset(entries_, 0, sizeof(Entry) * (1u << (initial_bucket_shift + 1)));
    }
    
    enum EntryKind {
        kFree,
        kHead,
        kNode,
    };
    
    struct Entry {
        uint32_t hash;
        uint32_t next;
        EntryKind kind;
        T        key;
        uint8_t  padding[kPaddingSize];
        Any     *value; // [strong ref]
    }; // struct Entry
    
    uint32_t bucket_shift_;
    uint32_t length_;
    uint32_t random_seed_;
    uint32_t free_;
    Entry entries_[0];
}; // template<class T> class Map


class MutableMapEntry : public Any {
public:    
    static const int32_t kOffsetNext;
    static const int32_t kOffsetHash;
    static const int32_t kOffsetKey;
    static const int32_t kOffsetValue;
    
    friend class MutableMap;
private:
    MutableMapEntry();
    
    MutableMapEntry *next_; // [strong ref]
    uint32_t hash_;
    uint32_t padding0_;
    Any *key_; // [strong ref]
    Any *value_; // [strong ref]
}; // class MapEntry


class MutableMap : public Any {
public:
    using Entry = MutableMapEntry;
    
private:
    MutableMap(const Class *clazz, uint32_t initial_bucket_shift, uint32_t random_seed);
    
    uint32_t bucket_shift_;
    uint32_t length_;
    uint32_t random_seed_;
    Array<Entry*> *bucket_; // [strong ref]
}; // class MutableMap



} // namespace lang

} // namespace mai

#endif // MAI_LANG_VALUE_H_
