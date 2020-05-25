#pragma once
#ifndef MAI_LANG_VALUE_H_
#define MAI_LANG_VALUE_H_

#include "lang/type-defs.h"
#include "lang/handle.h"
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <type_traits>
#include <string_view>
#include <string>

namespace mai {

namespace lang {

class Class;
class Field;
class Type;
class Function;
class Code;
class CapturedValue;
class ObjectVisitor;

// All heap object's base class.
class Any {
public:
    static const int32_t kOffsetKlass;
    static const int32_t kOffsetTags;
    
    static constexpr uint32_t kColorMask = 0x0ff;
    static constexpr uint32_t kClosureMask = 0xf00;
    
    // internal functions:
    inline bool is_forward() const;
    inline bool is_directly() const;
    inline Class *clazz() const;
    inline void set_clazz(const Class *clazz);
    inline Any *forward() const;
    inline void set_forward(Any *addr);
    inline void set_forward_address(uint8_t *addr);
    inline int color() const;
    inline void set_color(int color);
    inline uint32_t tags() const;
    inline bool QuicklyIs(uint32_t type_id) const;
    
    template<class T> inline T UnsafeGetField(const Field *field) const;
    
    template<class T> inline T *UnsafeAccess(const Field *field);
    
    template<class T> inline void UnsafeSetField(const Field *field, T value);

    template<class T> inline bool Is() const { return SlowlyIs(TypeTraits<T>::kType); }
    
    template<class T>
    inline Local<T> As() {
        return Is<T>() ? Local<T>(static_cast<T *>(this)) : Local<T>::Empty();
    }

    Any(const Any &) = delete;
    void operator = (const Any &) = delete;
    void operator delete (void *) = delete;

    friend class Machine;
protected:
    Any(const Class *clazz, uint32_t tags)
        : klass_(reinterpret_cast<uintptr_t>(clazz))
        , tags_(tags) {}
    
    // Type test
    // For API
    bool SlowlyIs(uint32_t type_id) const;
        
    // Write-Barrier for heap object writting
    bool WriteBarrier(Any **address) { return WriteBarrier(address, 1) == 1; }
    int WriteBarrier(Any **address, size_t n);
    
    uintptr_t klass_; // Forward pointer or Class pointer
    uint32_t tags_; // Tags for heap object
}; // class Any


// All array bese class
class AbstractArray : public Any {
public:
    static constexpr size_t kAppendIndex = -1;
    static const int32_t kOffsetElemType;
    static const int32_t kOffsetCapacity;
    static const int32_t kOffsetLength;
    
    // How many elems to allocation
    uint32_t capacity() const { return capacity_; }

    // Number of elements
    uint32_t length() const { return length_; }
    
    const Class *elem_type() const { return elem_type_; }

    friend class Machine;
protected:
    AbstractArray(const Class *clazz, const Class *elem_type, uint32_t capacity, uint32_t length,
                  uint32_t tags)
        : Any(clazz, tags)
        , elem_type_(elem_type)
        , capacity_(capacity)
        , length_(length) {
    }

    static AbstractArray *NewArray(BuiltinType type, size_t length);

    static AbstractArray *NewArrayCopied(const AbstractArray *origin, size_t increment);
    
    const Class *elem_type_;
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
template<class T, bool R = ElementTraits<T>::kIsReferenceType>
class Array : public AbstractArray {
public:
    static constexpr int32_t kOffsetElems = offsetof(Array, elems_);
    
    // Create new array with initialize-elements and length
    static inline Local<Array<T>> New(const T *elems, size_t length) {
        Local<Array<T>> array(New(length));
        for (size_t i = 0; i < length; i++) {
            array->elems_[i] = elems[i];
        }
        return array;
    }

    // Create new array with length
    static inline Local<Array<T>> New(size_t length) {
        Local<AbstractArray> abstract(NewArray(TypeTraits<T>::kType, length));
        if (abstract.is_empty()) {
            return Local<Array<T>>::Empty();
        }
        return Local<Array<T>>(static_cast<Array<T> *>(*abstract));
    }

    inline T At(size_t i) const { return (i < length_) ? elems_[i] : T(); }

    // The + operator:
    inline Local<Array<T>> Plus(size_t index, T elem) const {
        Local<AbstractArray> abstract(NewArrayCopied(this, index == kAppendIndex ? 1 : 0));
        if (abstract.is_empty() || abstract.is_value_null()) {
            return Local<Array<T>>::Empty();
        }
        Local<Array<T>> copied(static_cast<Array<T> *>(*abstract));
        if (index == kAppendIndex) {
            copied->elems_[length_] = elem;
        } else {
            copied->elems_[index] = elem;
        }
        return copied;
    }

    // The - operator:
    Local<Array<T>> Minus(size_t index) const {
        if (index > length_) {
            return Local<Array<T>>(const_cast<Array<T> *>(this));
        }
        Local<Array<T>> copied(New(length() - 1));
        ::memcpy(copied->elems_, elems_, index * sizeof(T));
        ::memcpy(copied->elems_ + index, elems_ + index + 1, (length_ - 1 - index) * sizeof(T));
        return copied;
    }
    
    // Internal functions
    inline T quickly_get(size_t i) const;
    inline void quickly_set(size_t i, T value);
    inline void quickly_set_length(size_t length);
    inline void QuicklyAppendNoResize(T data);
    inline void QuicklyAppendNoResize(const T *data, size_t n);
    inline T *QuicklyAdvanceNoResize(size_t n);

    friend class Machine;
protected:
    Array(const Class *clazz, const Class *elem_type, uint32_t capacity, uint32_t length, uint32_t tags)
        : AbstractArray(clazz, elem_type, capacity, length, tags) {
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
    static inline Local<Array<T>> New(Handle<CoreType> elems[], size_t length) {
        Local<Array<T>> array(New(length));
        for (size_t i = 0; i < length; i++) {
            array->elems_[i] = *elems[i];
        }
        array->WriteBarrier(reinterpret_cast<Any **>(array->elems_), length);
        return array;
    }

    // Create new array with length
    static inline Local<Array<T>> New(size_t length) {
        Local<Any> any(NewArray(TypeTraits<CoreType>::kType, length));
        if (any.is_empty()) {
            return Local<Array<T>>::Empty();
        }
        return Local<Array<T>>(static_cast<Array<T> *>(*any));
    }

    // Accessor
    inline Local<CoreType> At(size_t i) const {
        return (i < length_) ? Local<CoreType>(elems_[i]) : Local<CoreType>::Empty();
    }

    // The + operator:
    Local<Array<T>> Plus(size_t index, const Handle<CoreType> &elem) const {
        Local<AbstractArray> abstract(NewArrayCopied(this, index == kAppendIndex ? 1 : 0));
        if (abstract.is_empty() || abstract.is_value_null()) {
            return Local<Array<T>>::Empty();
        }
        Local<Array<T>> copied(static_cast<Array<T> *>(*abstract));
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
    Local<Array<T>> Minus(size_t index) const {
        if (index > length_) {
            return Local<Array<T>>::Empty();
        }
        Local<Array> copied(New(length_ - 1));
        ::memcpy(copied->elems_, elems_, index * sizeof(T));
        ::memcpy(copied->elems_ + index, elems_ + index + 1, (length_ - 1 - index) * sizeof(T));
        copied->WriteBarrier(reinterpret_cast<Any **>(copied->elems_), copied->length_);
        return copied;
    }

    // Internal functions
    inline T quickly_get(size_t i) const;
    inline void quickly_set(size_t i, T value);
    inline void quickly_set_nobarrier(size_t i, T value);
    inline void QuicklySetAll(size_t i, T *value, size_t n);
    inline void QuicklyAppend(T value);
    inline void Iterate(ObjectVisitor *visitor);
    
    friend class Machine;
protected:
    Array(const Class *clazz, const Class *elem_type, uint32_t capacity, uint32_t length, uint32_t tags)
        : AbstractArray(clazz, elem_type, capacity, length, tags) {
        ::memset(elems_, 0, sizeof(T) * length);
    }

    T elems_[0]; // [strong ref]
}; // template<class T> class Array<T, true>



// The string header
class String : public Array<char> {
public:
    static Local<String> NewUtf8(const char *utf8_string, size_t n);

    static Local<String> NewUtf8(const char *utf8_string) {
        return NewUtf8(utf8_string, strlen(utf8_string));
    }

    using Array<char>::capacity;
    using Array<char>::length;
    using Array<char>::elems_;

    const char *data() const { return elems_; }

    friend class Machine;
private:
    String(const Class *clazz, const Class *elem_type, uint32_t capacity, const char *utf8_string,
           uint32_t length, uint32_t tags)
        : Array<char>(clazz, elem_type, capacity, length, tags) {
        ::memcpy(elems_, utf8_string, length);
        elems_[length] = '\0'; // for c-style string
    }
    
    static constexpr size_t RequiredSize(size_t capacity) {
        return sizeof(String) + capacity;
    }
}; // class String


class AbstractMap : public Any {
public:
    static constexpr uint32_t kInitialBucketShift = 4;
    static constexpr uint32_t kInitialBucketSize = 1u << kInitialBucketShift;
    
    const Class *key_type() const { return key_type_; }
    const Class *value_type() const { return value_type_; }
    
    // Number of buckets
    uint32_t bucket_size() const { return 1u << bucket_shift_; }
    
    // Number of all entries
    uint32_t capacity() const { return 1u << (bucket_shift_ + 1); }

    uint32_t random_seed() const { return random_seed_; }
    
    uint32_t length() const { return length_; }
    
protected:
    AbstractMap(const Class *clazz, const Class *key, const Class *value,
                uint32_t initial_bucket_shift, uint32_t random_seed, uint32_t tags)
        : Any(clazz, tags)
        , key_type_(key)
        , value_type_(value)
        , bucket_shift_(initial_bucket_shift)
        , random_seed_(random_seed)
        , free_((1u << (initial_bucket_shift + 1)) - 1) {
    }
    
    static AbstractMap *NewMap(BuiltinType key, BuiltinType value, uint32_t bucket_size);
    AbstractMap *NewMap(uint32_t bucket_size);
    bool IsKeyReferenceType() const;
    bool IsValueReferenceType() const;

    const Class *key_type_;
    const Class *value_type_;
    const uint32_t bucket_shift_;
    const uint32_t random_seed_;
    uint32_t free_;
    uint32_t length_ = 0;
}; // class AbstractMap


template<class T> struct Hash {
    uint32_t operator () (T value) const { return 0; }
    bool operator () (T lhs, T rhs) const { return lhs == rhs; }
};

template<> struct Hash<int8_t> {
    uint32_t operator () (int8_t value) const { return static_cast<uint32_t>(value); }
    bool operator () (int8_t lhs, int8_t rhs) const { return lhs == rhs; }
};

template<> struct Hash<uint8_t> {
    uint32_t operator () (uint8_t value) const { return static_cast<uint32_t>(value); }
    bool operator () (uint8_t lhs, uint8_t rhs) const { return lhs == rhs; }
};

template<> struct Hash<int32_t> {
    uint32_t operator () (int32_t value) const { return static_cast<uint32_t>(value); }
    bool operator () (int32_t lhs, int32_t rhs) const { return lhs == rhs; }
};

template<> struct Hash<uint32_t> {
    uint32_t operator () (uint32_t value) const { return static_cast<uint32_t>(value); }
    bool operator () (uint32_t lhs, uint32_t rhs) const { return lhs == rhs; }
};

template<> struct Hash<int64_t> {
    uint32_t operator () (int64_t value) const {
        return static_cast<uint32_t>(value * value >> 16);
    }
    bool operator () (int64_t lhs, int64_t rhs) const { return lhs == rhs; }
};

template<> struct Hash<uint64_t> {
    uint32_t operator () (uint64_t value) const {
        return static_cast<uint32_t>(value * value >> 16);
    }
    bool operator () (uint64_t lhs, uint64_t rhs) const { return lhs == rhs; }
};

template<> struct Hash<float> {
    uint32_t operator () (float value) const {
        uint32_t h = *reinterpret_cast<uint32_t *>(&value);
        return h * h | 1;
    }

    bool operator () (float lhs, float rhs) const {
        return ::fabs(lhs - rhs) <= 0.00001f;
    }
};

template<> struct Hash<double> {
    uint32_t operator () (double value) const {
        uint64_t h = *reinterpret_cast<uint64_t *>(&value);
        return static_cast<uint32_t>(((h * h) >> 16) | 1);
    }

    bool operator () (double lhs, double rhs) const {
        return ::fabs(lhs - rhs) <= 0.00001f;
    }
};

template<> struct Hash<String*> {
    uint32_t operator () (String *value) const;
    bool operator () (String *lhs, String *rhs) const;
};

template<> struct Hash<Any*> {
    uint32_t operator () (Any *value) const;
    bool operator () (Any *lhs, Any *rhs) const;
};


// The Map's header
template<class K>
class ImplementMap : public AbstractMap {
public:
    struct Entry {
        enum Kind {
            kFree,
            kBucket,
            kNode,
        };
        uint32_t  next;
        Kind      kind;
        uintptr_t value;
        K         key;
    }; // struct Entry
    
    class Iterator {
    public:
        inline Iterator(ImplementMap *owns): owns_(owns) {}
        inline void SeekToFirst() { NextBucket(1); }
        inline void Next() {
            if (!Valid()) {
                return;
            }
            if (index_ = owns_->entries_[index_].next; !index_) {
                NextBucket(bucket_ + 1);
            }
        }
        inline bool Valid() const { return index_ != 0; }

        inline Entry *operator -> () const { return entry(); }
        inline Entry *entry() const { return !index_ ? nullptr : &owns_->entries_[index_]; }
    private:
        inline void NextBucket(uint32_t begin) {
            for (bucket_ = begin; bucket_ < owns_->bucket_size() + 1; bucket_++) {
                if (owns_->entries_[bucket_].kind == Entry::kBucket) {
                    break;
                }
            }
            if (bucket_ < owns_->bucket_size() + 1) {
                index_ = bucket_;
            }
        }
        
        ImplementMap *const owns_;
        uint32_t bucket_ = 0;
        uint32_t index_ = 0;
    }; // class Iterator
    
    static constexpr int32_t kOffsetKeyType = offsetof(ImplementMap, key_type_);
    static constexpr int32_t kOffsetValueType = offsetof(ImplementMap, value_type_);
    static constexpr int32_t kOffsetRandomSeed = offsetof(ImplementMap, random_seed_);
    static constexpr int32_t kOffsetLength = offsetof(ImplementMap, length_);
    static constexpr int32_t kOffsetEntries = offsetof(ImplementMap, entries_);
    
    static inline size_t RequiredSize(uint32_t bucket_size) {
        return sizeof(ImplementMap<K>) + (sizeof(Entry) * bucket_size * 2);
    }

    // Internal Interfaces
    template<class V>
    inline ImplementMap *UnsafePut(K key, V value);
    
    friend class Machine;
protected:
    inline ImplementMap(const Class *clazz, const Class *key, const Class *value,
               uint32_t initial_bucket_shift, uint32_t random_seed, uint32_t tags)
        : AbstractMap(clazz, key, value, initial_bucket_shift, random_seed, tags) {
        ::memset(entries_, 0, sizeof(Entry) * (capacity() + 1));
    }
    
    ImplementMap<K> *Clone(int incr_wanted) {
        uint32_t wanted = length() + incr_wanted;
        if (!wanted) {
            return static_cast<ImplementMap<K> *>(NewMap(0));
        }
        uint32_t request_bucket_shift = bucket_shift_;
        if (wanted >= bucket_size() * 1.8) {
            request_bucket_shift++;
        } else if (wanted < bucket_size() - 4 && bucket_size() >= kInitialBucketSize * 2) {
            request_bucket_shift--;
        }
        
        ImplementMap<K> *dest = nullptr;
        for (;;) {
            dest = static_cast<ImplementMap<K> *>(NewMap(1u << request_bucket_shift));
            if (!dest) {
                return nullptr;
            }
            if (Rehash(dest)) {
                break;
            }
            request_bucket_shift++;
        }
        return dest;
    }
    
    inline Entry *FindOrMakeRoom(K key, ImplementMap<K> **receiver) {
        ImplementMap<K> *dest = this;
        Entry *room = dest->FindForPut(key);
        while (!room) {
            uint32_t request_size = !dest->bucket_size() ? 16 : dest->bucket_size() * 2;
            dest = static_cast<ImplementMap<K> *>(dest->NewMap(request_size));
            if (!dest) {
                return nullptr;
            }
            if (!Rehash(dest)) {
                continue;
            }
            room = dest->FindForPut(key);
        }
        *receiver = dest;
        return room;
    }

    inline void RemoveRoom(K key, ImplementMap<K> **receiver) {
        Entry *room = Delete(key);
        if (!room) {
            *receiver = this;
            return;
        }
        if (ElementTraits<K>::kIsReferenceType) {
            WriteBarrier(reinterpret_cast<Any **>(&room->key));
        }
        if (length() == 0) {
            *receiver = static_cast<ImplementMap<K> *>(NewMap(0));
            return;
        }
        ImplementMap<K> *dest = this;
        if (length() < capacity() / 2 - 4 && bucket_size() >= kInitialBucketSize * 2) {
            dest = static_cast<ImplementMap<K> *>(dest->NewMap(dest->bucket_size() / 2));
            if (!dest) {
                *receiver = nullptr;
                return;
            }
            if (!Rehash(dest)) {
                return;
            }
        }
        *receiver = dest;
    }
    
    inline bool Rehash(ImplementMap<K> *dest) {
        Iterator iter(this);
        for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
            Entry *entry = dest->FindForPut(iter->key);
            if (!entry) {
                return false;
            }
            entry->key = iter->key;
            if (ElementTraits<K>::kIsReferenceType) {
                dest->WriteBarrier(reinterpret_cast<Any **>(&entry->key));
            }
            entry->value = iter->value;
            if (IsValueReferenceType()) {
                dest->WriteBarrier(reinterpret_cast<Any **>(&entry->value));
            }
        }
        return true;
    }

    inline Entry *FindForPut(K key) {
        Entry *slot = GetSlot(key);
        if (slot->kind == Entry::kFree) {
            slot->kind = Entry::kBucket;
            slot->next = 0;
            length_++;
            return slot;
        }
        Entry *node = slot;
        while (node != &entries_[0]) {
            if (Hash<K>{}(key, node->key)) {
                return node;
            }
            node = &entries_[node->next];
        }
        uint32_t index = AllocateRoom();
        if (index == 0){
            return nullptr;
        }
        node = &entries_[index];
        node->kind = Entry::kNode;
        node->next = slot->next;
        slot->next = index;
        length_++;
        return node;
    }
    
    inline Entry *FindForGet(K key) {
        Entry *slot = GetSlot(key);
        if (slot->kind == Entry::kFree) {
            return nullptr;
        }
        Entry *node = slot;
        while (node != &entries_[0]) {
            if (Hash<K>{}(key, node->key)) {
                return node;
            }
            node = &entries_[node->next];
        }
        return nullptr;
    }
    
    inline Entry *Delete(K key) {
        Entry *slot = GetSlot(key);
        if (slot->kind == Entry::kFree) {
            return nullptr;
        }
        if (Hash<K>{}(key, slot->key)) {
            uint32_t next = slot->next;
            ::memcpy(slot, &entries_[next], sizeof(*slot));
            FreeRoom(next);
            if (slot->kind == Entry::kNode) {
                slot->kind = Entry::kBucket;
                ::memset(&entries_[next], 0, sizeof(Entry));
            }
            length_--;
            return slot;
        }
        Entry *prev = slot;
        Entry *node = &entries_[slot->next];
        while (node != &entries_[0]) {
            if (Hash<K>{}(key, node->key)) {
                FreeRoom(prev->next);
                prev->next = node->next;
                ::memset(node, 0, sizeof(*node));
                length_--;
                return node;
            }
            prev = node;
            node = &entries_[node->next];
        }
        return nullptr;
    }

    inline uint32_t AllocateRoom() {
        while (entries_[1 + free_].kind != Entry::kFree && free_ >= bucket_size()) {
            free_--;
        }
        return free_ < bucket_size() ? 0 : (free_-- + 1);
    }
    
    inline void FreeRoom(uint32_t index) {
        if (index - 1 > free_) { free_ = index - 1; }
    }
    
    inline Entry *GetSlot(K key) {
        uint32_t slot = (Hash<K>{}(key) ^ random_seed_) & ((1u << bucket_shift_) - 1);
        return &entries_[1 + slot]; // Skip index:0
    }

    Entry entries_[1];
}; // template<class T> class ImplementMap


template<class K, class V,
    bool KT = ElementTraits<K>::kIsReferenceType,
    bool VT = ElementTraits<V>::kIsReferenceType>
class Map : public ImplementMap<K> {
public:
    using Entry = typename ImplementMap<K>::Entry;
    
    class Iterator {
    public:
        inline Iterator(const Handle<Map> &owns): core_(*owns) {}
        inline Iterator(Map *owns): core_(owns) {}
        inline void SeekToFirst() { core_.SeekToFirst(); }
        inline void Next() { core_.Next(); }
        inline bool Valid() const { return core_.Valid(); }
        inline K key() const { return !core_.Valid() ? K(0) : core_->key; }
        inline V value() const {
            return !core_.Valid() ? V(0) : *reinterpret_cast<V *>(&core_->value);
        }
    private:
        typename ImplementMap<K>::Iterator core_;
    }; // class Iterator

    static inline Local<Map<K,V>> New(uint32_t bucket_size = AbstractMap::kInitialBucketSize) {
        Local<Map<K,V>> map(static_cast<Map<K,V> *>(NewMap(TypeTraits<K>::kType,
                                                           TypeTraits<V>::kType, bucket_size)));
        return map;
    }
    
    inline Local<Map<K,V>> Plus(K key, V value) {
        Local<Map<K,V>> map(static_cast<Map<K,V> *>(Clone(1)));
        if (map.is_value_null()) {
            return map;
        }
        map->Put(key, value, &map);
        return map;
    }
    
    inline Local<Map<K,V>> Minus(K key) {
        Local<Map<K,V>> map(static_cast<Map<K,V> *>(Clone(-1)));
        if (map.is_value_null()) {
            return map;
        }
        if (map->length() > 0) {
            map->Erase(key);
        }
        return map;
    }
    
    inline void Put(K key, V value, Local<Map<K,V>> *handle) {
        ImplementMap<K> *dummy = nullptr;
        Entry *room = FindOrMakeRoom(key, &dummy);
        if (!room) {
            *handle = nullptr;
            return;
        }
        room->key = key;
        *reinterpret_cast<V *>(&room->value) = value;
        *handle = static_cast<Map<K,V> *>(dummy);
    }
    
    inline void Set(K key, V value) {
        if (Entry *room = FindForPut(key)) {
            room->key = key;
            *reinterpret_cast<V *>(&room->value) = value;
        }
    }
    
    inline void Remove(K key, Local<Map<K, V>> *handle) {
        ImplementMap<K> *dummy = nullptr;
        RemoveRoom(key, &dummy);
        *handle = static_cast<Map<K,V> *>(dummy);
    }

    inline void Erase(K key) { Delete(key); }
    
    inline bool Get(K key, V *receiver) {
        Entry *room = FindForGet(key);
        if (!room) {
            return false;
        }
        if (receiver) {
            *receiver = *reinterpret_cast<V *>(&room->value);
        }
        return true;
    }
    
private:
    using AbstractMap::NewMap;
    using ImplementMap<K>::FindOrMakeRoom;
    using ImplementMap<K>::FindForPut;
    using ImplementMap<K>::FindForGet;
    using ImplementMap<K>::RemoveRoom;
    using ImplementMap<K>::Delete;
    using ImplementMap<K>::Clone;
}; // class Map


template<class K, class V>
class Map<K, V, true, false> : public ImplementMap<K> {
public:
    using CoreKeyType = typename ElementTraits<K>::CoreType;
    using Entry = typename ImplementMap<K>::Entry;

    class Iterator {
    public:
        inline Iterator(const Handle<Map> &owns): core_(*owns) {}
        inline Iterator(Map *owns): core_(owns) {}
        inline void SeekToFirst() { core_.SeekToFirst(); }
        inline void Next() { core_.Next(); }
        inline bool Valid() const { return core_.Valid(); }
        inline Local<CoreKeyType> key() const {
            return !core_.Valid() ? Local<CoreKeyType>::Empty() : Local<CoreKeyType>(core_->key);
        }
        inline V value() const {
            return !core_.Valid() ? V(0) : *reinterpret_cast<V*>(&core_->value);
        }
    private:
        typename ImplementMap<K>::Iterator core_;
    }; // class Iterator

    static inline Local<Map<K,V>> New(uint32_t bucket_size = 16) {
        Local<Map<K,V>> map(static_cast<Map<K,V> *>(NewMap(TypeTraits<CoreKeyType>::kType,
                                                           TypeTraits<V>::kType,
                                                           bucket_size)));
        return map;
    }
    
    void Put(const Handle<CoreKeyType> &key, V value, Local<Map<K,V>> *handle) {
        ImplementMap<K> *dummy = nullptr;
        Entry *room = FindOrMakeRoom(*key, &dummy);
        if (!room) {
            *handle = nullptr;
            return;
        }
        if (room->key != *key) {
            room->key = *key;
            WriteBarrier(reinterpret_cast<Any **>(&room->key));
        }
        *reinterpret_cast<V *>(&room->value) = value;
        *handle = static_cast<Map<K,V> *>(dummy);
    }

    inline void Set(const Handle<CoreKeyType> &key, V value) {
        if (Entry *room = FindForPut(*key)) {
            if (room->key != *key) {
                room->key = *key;
                WriteBarrier(reinterpret_cast<Any **>(&room->key));
            }
            *reinterpret_cast<V *>(&room->value) = value;
        }
    }

    inline void Remove(K key, Local<Map<K, V>> *handle) {
        ImplementMap<K> *dummy = nullptr;
        RemoveRoom(key, &dummy);
        *handle = static_cast<Map<K,V> *>(dummy);
    }

    inline void Erase(const Handle<CoreKeyType> &key) {
        if (Entry *room = Delete(*key)) {
            WriteBarrier(reinterpret_cast<Any **>(&room->key));
        }
    }
    
    inline bool Get(const Handle<CoreKeyType> &key, V *receiver) {
        Entry *room = FindForGet(*key);
        if (!room) {
            return false;
        }
        if (receiver) {
            *receiver = *reinterpret_cast<V *>(&room->value);
        }
        return true;
    }

private:
    using AbstractMap::NewMap;
    using Any::WriteBarrier;
    using ImplementMap<K>::FindOrMakeRoom;
    using ImplementMap<K>::FindForPut;
    using ImplementMap<K>::FindForGet;
    using ImplementMap<K>::RemoveRoom;
    using ImplementMap<K>::Delete;
}; // class Map<K, V, true, false>

template<class K, class V>
class Map<K, V, true, true> : public ImplementMap<K> {
public:
    using CoreKeyType = typename ElementTraits<K>::CoreType;
    using CoreValueType = typename ElementTraits<V>::CoreType;
    using Entry = typename ImplementMap<K>::Entry;
    
    class Iterator {
    public:
        inline Iterator(const Handle<Map> &owns): core_(*owns) {}
        inline Iterator(Map *owns): core_(owns) {}
        inline void SeekToFirst() { core_.SeekToFirst(); }
        inline void Next() { core_.Next(); }
        inline bool Valid() const { return core_.Valid(); }
        inline Local<CoreKeyType> key() const {
            return !core_.Valid() ? Local<CoreKeyType>::Empty() : Local<CoreKeyType>(core_->key);
        }
        inline Local<CoreValueType> value() const {
            return !core_.Valid() ? Local<CoreValueType>::Empty()
                : Local<CoreValueType>(reinterpret_cast<V>(core_->value));
        }
    private:
        typename ImplementMap<K>::Iterator core_;
    }; // class Iterator

    static inline Local<Map<K,V>> New(uint32_t bucket_size = 16) {
        Local<Map<K,V>> map(static_cast<Map<K,V> *>(NewMap(TypeTraits<CoreKeyType>::kType,
                                                           TypeTraits<CoreValueType>::kType,
                                                           bucket_size)));
        return map;
    }
    
    inline void Put(const Handle<CoreKeyType> &key, const Handle<CoreValueType> &value,
                    Local<Map<K,V>> *handle) {
        ImplementMap<K> *dummy = nullptr;
        Entry *room = FindOrMakeRoom(*key, &dummy);
        if (!room) {
            *handle = nullptr;
            return;
        }
        if (room->key != *key) {
            room->key = *key;
            WriteBarrier(reinterpret_cast<Any **>(&room->key));
        }
        if (reinterpret_cast<CoreValueType *>(room->value) != *value) {
            room->value = reinterpret_cast<uintptr_t>(*value);
            WriteBarrier(reinterpret_cast<Any **>(&room->value));
        }
        *handle = static_cast<Map<K,V> *>(dummy);
    }

    inline void Set(const Handle<CoreKeyType> &key, const Handle<CoreValueType> &value) {
        if (Entry *room = FindForPut(*key)) {
            if (room->key != *key) {
                room->key = *key;
                WriteBarrier(reinterpret_cast<Any **>(&room->key));
            }
            if (reinterpret_cast<CoreValueType *>(room->value) != *value) {
                room->value = reinterpret_cast<uintptr_t>(*value);
                WriteBarrier(reinterpret_cast<Any **>(&room->value));
            }
        }
    }

    inline void Remove(K key, Local<Map<K, V>> *handle) {
        ImplementMap<K> *dummy = nullptr;
        RemoveRoom(key, &dummy);
        *handle = static_cast<Map<K,V> *>(dummy);
    }
    
    inline void Erase(const Handle<CoreKeyType> &key) {
        if (Entry *room = Delete(key)) {
            WriteBarrier(reinterpret_cast<Any **>(&room->key));
            WriteBarrier(reinterpret_cast<Any **>(&room->value));
        }
    }
    
    inline Local<CoreValueType> Get(const Handle<CoreKeyType> &key) {
        Entry *room = FindForGet(*key);
        if (!room) {
            return Local<CoreValueType>::Empty();
        }
        return Local<CoreValueType>(reinterpret_cast<CoreValueType *>(room->value));
    }
    
private:
    using Any::WriteBarrier;
    using AbstractMap::NewMap;
    using ImplementMap<K>::FindOrMakeRoom;
    using ImplementMap<K>::FindForPut;
    using ImplementMap<K>::FindForGet;
    using ImplementMap<K>::RemoveRoom;
    using ImplementMap<K>::Delete;
}; // class Map<K, V, true, true>

template<class K, class V>
class Map<K, V, false, true> : public ImplementMap<K> {
public:
    using CoreValueType = typename ElementTraits<V>::CoreType;
    using Entry = typename ImplementMap<K>::Entry;
    
    class Iterator {
    public:
        inline Iterator(const Handle<Map> &owns): core_(*owns) {}
        inline Iterator(Map *owns): core_(owns) {}
        inline void SeekToFirst() { core_.SeekToFirst(); }
        inline void Next() { core_.Next(); }
        inline bool Valid() const { return core_.Valid(); }
        inline K key() const { return !core_.Valid() ? K(0) : core_->key; }
        inline Local<CoreValueType> value() const {
            return !core_.Valid() ? Local<CoreValueType>::Empty()
                : Local<CoreValueType>(reinterpret_cast<V>(core_->value));
        }
    private:
        typename ImplementMap<K>::Iterator core_;
    }; // class Iterator

    static inline Local<Map<K,V>> New(uint32_t bucket_size = 16) {
        Local<Map<K,V>> map(static_cast<Map<K,V> *>(NewMap(TypeTraits<K>::kType,
                                                           TypeTraits<CoreValueType>::kType,
                                                           bucket_size)));
        return map;
    }
    
    inline void Put(K key, const Handle<CoreValueType> &value, Local<Map<K,V>> *handle) {
        ImplementMap<K> *dummy = nullptr;
        Entry *room = FindOrMakeRoom(key, &dummy);
        if (!room) {
            *handle = nullptr;
            return;
        }
        room->key = key;
        if (reinterpret_cast<CoreValueType *>(room->value) != *value) {
            room->value = reinterpret_cast<uintptr_t>(*value);
            WriteBarrier(reinterpret_cast<Any **>(&room->value));
        }
        *handle = static_cast<Map<K,V> *>(dummy);
    }

    inline void Set(K key, const Handle<CoreValueType> &value) {
        Entry *room = FindForPut(key);
        if (room) {
            room->key = key;
            if (reinterpret_cast<CoreValueType *>(room->value) != *value) {
                room->value = reinterpret_cast<uintptr_t>(*value);
                WriteBarrier(reinterpret_cast<Any **>(&room->value));
            }
        }
    }

    inline void Remove(K key, Local<Map<K, V>> *handle) {
        ImplementMap<K> *dummy = nullptr;
        RemoveRoom(key, &dummy);
        *handle = static_cast<Map<K,V> *>(dummy);
    }

    inline void Erase(K key) {
        Entry *room = Delete(key);
        if (room) {
            WriteBarrier(reinterpret_cast<Any **>(&room->value));
        }
    }
    
    inline Local<CoreValueType> Get(K key) {
        Entry *room = FindForGet(key);
        if (!room) {
            return Local<CoreValueType>::Empty();
        }
        return Local<CoreValueType>(reinterpret_cast<CoreValueType *>(room->value));
    }
private:
    using Any::WriteBarrier;
    using AbstractMap::NewMap;
    using ImplementMap<K>::FindOrMakeRoom;
    using ImplementMap<K>::FindForPut;
    using ImplementMap<K>::FindForGet;
    using ImplementMap<K>::RemoveRoom;
    using ImplementMap<K>::Delete;
}; // class Map<K, V, false, true>


// Value Base Class
class AbstractValue : public Any  {
public:
    static const int32_t kOffsetValue;
    static constexpr size_t kMaxValueSize = sizeof(void *);

    friend class Machine;
protected:
    AbstractValue(const Class *clazz, const void *data, size_t n, uint32_t tags)
        : Any(clazz, tags) {
        ::memcpy(value_, data, n);
    }

    AbstractValue(const Class *clazz, uint32_t tags)
        : Any(clazz, tags) {
        ::memset(value_, 0, kMaxValueSize);
    }

    void *address() { return value_; }
    const void *address() const { return value_; }

    static AbstractValue *ValueOf(BuiltinType type, const void *value, size_t n);

    uint32_t padding_; // Padding aligment to cache line
    uint8_t value_[kMaxValueSize]; // Storage value
}; // class AbstractValue


// Heap boxed number
template<class T>
class Number : public AbstractValue {
public:
    using AbstractValue::kOffsetValue;
    using AbstractValue::address;
    using AbstractValue::value_;
    
    static_assert(sizeof(T) <= kMaxValueSize, "Bad type T!");
    static_assert(std::is_floating_point<T>::value ||
                  std::is_integral<T>::value, "T is not a number type");

    inline T value() const { return *static_cast<const T *>(address()); }

    static inline Local<Number<T>> ValueOf(T value) {
        Local<AbstractValue> abstract(AbstractValue::ValueOf(TypeTraits<T>::kType, &value,
                                                              sizeof(value)));
        if (abstract.is_empty() || abstract.is_value_null()) {
            return Local<Number<T>>::Empty();
        }
        return Local<Number<T>>(static_cast<Number *>(*abstract));
    }
    
    friend class Machine;
private:
    inline Number(const Class *clazz, T value)
        : AbstractValue(clazz, &value, sizeof(value)) {
    }
}; // template<class T> class Number



// Callable closure function
class Closure : public Any {
public:
    struct CapturedVar {
        CapturedValue *value;
    }; //struct CapturedVar
    static_assert(sizeof(CapturedVar) == sizeof(CapturedVar::value), "Incorrect captured var size");

    static const int32_t kOffsetProto;
    static const int32_t kOffsetCode;
    static const int32_t kOffsetCapturedVarSize;
    static const int32_t kOffsetCapturedVar;

    static constexpr uint32_t kCxxFunction = 0x100;
    static constexpr uint32_t kMaiFunction = 0x200;

    static Local<Closure> New(Code *stub, uint32_t captured_var_size);
    
    uint32_t captured_var_size() const { return captured_var_size_; }
    
    // Internal methods:
    inline bool is_cxx_function() const;
    inline bool is_mai_function() const;
    inline Code *code() const;
    inline Function *function() const;
    inline void SetCapturedVar(uint32_t i, CapturedValue *value);
    inline void set_captured_var_no_barrier(uint32_t i, CapturedValue *value);
    inline CapturedValue *captured_var(uint32_t i) const;
    inline void Iterate(ObjectVisitor *visitor);
    
    friend class Machine;
private:
    Closure(const Class *clazz, Code *cxx_fn, uint32_t captured_var_size, uint32_t tags)
        : Any(clazz, tags|kCxxFunction)
        , cxx_fn_(cxx_fn)
        , captured_var_size_(captured_var_size) {
        ::memset(captured_var_, 0, sizeof(CapturedVar) * captured_var_size_);
    }

    Closure(const Class *clazz, Function *mai_fn, uint32_t captured_var_size, uint32_t tags)
        : Any(clazz, tags|kMaiFunction)
        , mai_fn_(mai_fn)
        , captured_var_size_(captured_var_size) {
        ::memset(captured_var_, 0, sizeof(CapturedVar) * captured_var_size_);
    }

    union {
        Code *cxx_fn_;
        Function *mai_fn_;
    }; // union: Function proto or cxx function stub
    uint32_t captured_var_size_; // Number of captured_var_size_
    CapturedVar captured_var_[0]; // Captured variables
}; // class Closure


// All exception's base class
class Throwable : public Any {
public:
    static const int32_t kOffsetStacktrace;
    
    // Internal functions:
    inline Array<String *> *stacktrace() const;
    
    // Print stack strace to file
    void PrintStackstrace(FILE *file) const;

    // Throw a exception
    static void Throw(const Handle<Throwable> &exception);
    
    // Internal functions
    inline void QuickSetStacktrace(Array<String *> *stackstrace);

    friend class Runtime;
    friend class Machine;
protected:
    Throwable(const Class *clazz, Array<String *> *stacktrace, uint32_t tags)
        : Any(clazz, tags)
        , stacktrace_(stacktrace) {
        if (stacktrace) {
            WriteBarrier(reinterpret_cast<Any **>(&stacktrace_));
        }
    }
    
    static Throwable *NewPanic(int code, String *message);
    
    //static Throwable *NewException(String *message, Exception *cause);
    
    static Array<String *> *MakeStacktrace(uint8_t *frame_bp);

    Array<String *> *stacktrace_; // [strong ref] stacktrace information
}; // class Exception



// Panic can not be catch
class Panic : public Throwable {
public:
    static const int32_t kOffsetCode;
    static const int32_t kOffsetMessage;
    
    enum Level {
        kCrash,
        kFatal,
        kError,
    };
    
    // Code of panic reason
    Level code() const { return code_; }

    // Message of panic information
    Local<String> message() const { return Local<String>(quickly_message()); }

    // Internal functions
    inline String *quickly_message() const;

    // New a panic
    static Local<Panic> New(int code, Handle<String> message) {
        Local<Throwable> throwable(NewPanic(code, *message));
        if (throwable.is_empty()) {
            return Local<Panic>::Empty();
        }
        return Local<Panic>(static_cast<Panic *>(*throwable));
    }

    friend class Machine;
private:
    Panic(const Class *clazz, Array<String *> *stacktrace, Level code, String *message, uint32_t tags);

    Level code_; // code of error
    String *message_; // [strong ref] Message of error
}; // class Panic

// String builder
class IncrementalStringBuilder {
public:
    static constexpr size_t kInitSize = 16;
    
    IncrementalStringBuilder(): IncrementalStringBuilder("", 0) {}
    
    IncrementalStringBuilder(const Handle<String> &init)
        : IncrementalStringBuilder(init->data(), init->length()) {}

    ~IncrementalStringBuilder() {
        if (incomplete_) {
            GlobalHandles::DeleteHandle(reinterpret_cast<void **>(incomplete_));
        }
    }
    
    void AppendString(const Handle<String> &handle) {
        if (handle.is_not_empty()) {
            AppendString(handle->data(), handle->length());
        }
    }
    
    void AppendString(const String *str) { AppendString(str->data(), str->length()); }

    void AppendString(const std::string &str) { AppendString(str.data(), str.size()); }

    void AppendString(const std::string_view &str) { AppendString(str.data(), str.size()); }

    void AppendFormat(const char *fmt, ...);

    void AppendVFormat(const char *fmt, va_list va);

    void AppendString(const char *z) {
        if (z) {
            AppendString(z, ::strlen(z));
        }
    }

    void AppendString(const char *z, size_t n);

    Local<String> Build() const { return Local<String>(Finish()); }
    
    // Internal functions
    inline String* QuickBuild() const;
    
    IncrementalStringBuilder(const IncrementalStringBuilder &) = delete;
    void operator = (const IncrementalStringBuilder &) = delete;
private:
    IncrementalStringBuilder(const char *z, size_t n);
    
    String *Finish() const;
    
    Array<char> *incomplete() const { return *incomplete_; }
    
    Array<char> **incomplete_ = nullptr; // [global handle] incomplete buffer.
}; // class IncrementalStringBuilder

// CXX Function template for binding
class FunctionTemplate {
public:
    template<class T>
    struct ParameterTraits {
        static constexpr uint32_t kType = TypeTraits<T>::kType;
    }; // struct ParameterTraits
    
    template<class T>
    struct ParameterTraits<Handle<T>> {
        static constexpr uint32_t kType = TypeTraits<T>::kType;
    }; // struct ParameterTraits
    
    template<>
    struct ParameterTraits<void*> {
        static constexpr uint32_t kType = kType_u64;
    }; // struct ParameterTraits
    
    template<>
    struct ParameterTraits<const Class *> {
        static constexpr uint32_t kType = kType_u64;
    }; // struct ParameterTraits
    
    template<class T>
    struct ReturnTraits {
        static constexpr uint32_t kType = TypeTraits<typename std::remove_pointer<T>::type>::kType;
    }; // struct ReturnTraits
    
    template<>
    struct ReturnTraits<void*> {
        static constexpr uint32_t kType = kType_u64;
    }; // struct ReturnTraits
    
    template<class R>
    static inline Local<Closure> New(R(*func)()) {
        Code *stub = MakeStub({}/*parametres*/, false/*has_vargs*/, ReturnTraits<R>::kType,
                              reinterpret_cast<uint8_t *>(func));
        if (!stub) {
            return Local<Closure>::Empty();
        } else {
            return Closure::New(stub, 0/*captured_var_size*/);
        }
    }
    
    template<class R, class A>
    static inline Local<Closure> New(R(*func)(A)) {
        Code *stub = MakeStub({ParameterTraits<A>::kType}/*parametres*/, false/*has_vargs*/,
                              ReturnTraits<R>::kType, reinterpret_cast<uint8_t *>(func));
        if (!stub) {
            return Local<Closure>::Empty();
        } else {
            return Closure::New(stub, 0/*captured_var_size*/);
        }
    }
    
    template<class R, class A, class B>
    static inline Local<Closure> New(R(*func)(A, B)) {
        Code *stub = MakeStub({ParameterTraits<A>::kType, ParameterTraits<B>::kType}/*parametres*/,
                              false/*has_vargs*/, ReturnTraits<R>::kType,
                              reinterpret_cast<uint8_t *>(func));
        if (!stub) {
            return Local<Closure>::Empty();
        } else {
            return Closure::New(stub, 0/*captured_var_size*/);
        }
    }
    
    template<class R, class A, class B, class C>
    static inline Local<Closure> New(R(*func)(A, B, C)) {
        Code *stub = MakeStub({ParameterTraits<A>::kType,
                               ParameterTraits<B>::kType,
                               ParameterTraits<C>::kType}/*parametres*/,
                              false/*has_vargs*/, ReturnTraits<R>::kType,
                              reinterpret_cast<uint8_t *>(func));
        if (!stub) {
            return Local<Closure>::Empty();
        } else {
            return Closure::New(stub, 0/*captured_var_size*/);
        }
    }
    
    template<class R, class A, class B, class C, class D>
    static inline Local<Closure> New(R(*func)(A, B, C, D)) {
        Code *stub = MakeStub({ParameterTraits<A>::kType,
                               ParameterTraits<B>::kType,
                               ParameterTraits<C>::kType,
                               ParameterTraits<D>::kType}/*parametres*/,
                              false/*has_vargs*/, ReturnTraits<R>::kType,
                              reinterpret_cast<uint8_t *>(func));
        if (!stub) {
            return Local<Closure>::Empty();
        } else {
            return Closure::New(stub, 0/*captured_var_size*/);
        }
    }
    
    template<class R, class A, class B, class C, class D, class E>
    static inline Local<Closure> New(R(*func)(A, B, C, D, E)) {
        Code *stub = MakeStub({ParameterTraits<A>::kType,
                               ParameterTraits<B>::kType,
                               ParameterTraits<C>::kType,
                               ParameterTraits<D>::kType,
                               ParameterTraits<E>::kType}/*parametres*/,
                              false/*has_vargs*/, ReturnTraits<R>::kType,
                              reinterpret_cast<uint8_t *>(func));
        if (!stub) {
            return Local<Closure>::Empty();
        } else {
            return Closure::New(stub, 0/*captured_var_size*/);
        }
    }

    // Can not construction and destory
    FunctionTemplate(const FunctionTemplate &) = delete;
    void operator = (const FunctionTemplate &) = delete;
    FunctionTemplate() = delete;
    ~FunctionTemplate() = delete;
private:
    static Code *MakeStub(const std::vector<uint32_t> &parameters, bool has_vargs,
                          uint32_t return_type, uint8_t *cxx_func_entry);
}; // class FunctionTemplate

} // namespace lang

} // namespace mai

#endif // MAI_LANG_VALUE_H_
