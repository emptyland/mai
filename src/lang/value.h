#ifndef MAI_LANG_VALUE_H_
#define MAI_LANG_VALUE_H_

#include <stdint.h>
#include "glog/logging.h"

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
    
    void *operator new (size_t) = delete;
    void operator delete (void *) = delete;
protected:
    Any(Class *clazz, uint32_t tags)
        : forward_(reinterpret_cast<uintptr_t>(clazz))
        , tags_(tags) {}
    
    uintptr_t forward_; // Forward pointer or Class pointer
    uint32_t tags_; // Tags for heap object
}; // class Any


// The Array's header
template<class T>
class Array : public Any {
public:
    static constexpr int32_t kOffsetCapacity = offsetof(Array, capacity_);
    static constexpr int32_t kOffsetLength = offsetof(Array, length_);
    static constexpr int32_t kOffsetElems = offsetof(Array, elems_);
    
private:
    Array(Class *clazz, uint32_t capacity, uint32_t length)
        : Any(clazz, 0)
        , capacity_(capacity)
        , length_(length) {}

    uint32_t capacity_;
    uint32_t length_;
    T elems_[0]; // [? strong ref]
}; //template<class T> class Array


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
    
    inline Map(Class *clazz, uint32_t initial_bucket_shift, uint32_t random_seed)
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
    MutableMap(Class *clazz, uint32_t initial_bucket_shift, uint32_t random_seed);
    
    uint32_t bucket_shift_;
    uint32_t length_;
    uint32_t random_seed_;
    Array<Entry*> *bucket_; // [strong ref]
}; // class MutableMap



} // namespace lang

} // namespace mai

#endif // MAI_LANG_VALUE_H_
