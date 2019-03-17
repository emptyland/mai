#ifndef MAI_NYAA_NYAA_VALUES_H_
#define MAI_NYAA_NYAA_VALUES_H_

#include "mai-lang/handles.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {
    
class Object;
class NyObject;
class NyString;
class NyArray;
class NyTable;
class NyScript;
class NyaaCore;
class NyFactory;
    
class String;
class Array;
class Table;
class Script;

    
//--------------------------------------------------------------------------------------------------
// [Object]
class Object {
public:
    static Object *const kNil;
    
    bool is_smi() const { return word() & 0x1; }
    
    bool is_object() const { return !is_smi(); }
    
    uintptr_t word() const { return reinterpret_cast<uintptr_t>(this); }
    
    int64_t smi() const {
        DCHECK(is_object());
        return static_cast<int64_t>(word() >> 1);
    }
    
    NyObject *heap_object() const {
        DCHECK(is_object());
        return reinterpret_cast<NyObject *>(word());
    }
}; // class Value
    
    
class NyObject : public Object {
public:
    bool is_forward_mt() const { return mtword_ & 0x1; }
    
    bool is_direct_mt() const { return !is_forward_mt(); }
    
    NyTable *GetMetatable() const {
        void *addr = is_forward_mt() ? forward_address() : reinterpret_cast<void *>(mtword_);
        return *reinterpret_cast<NyTable **>(addr);
    }
    
    void SetMetatable(NyTable *mt, NyaaCore *N); // TODO:
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyObject);
private:
    void *forward_address() const {
        DCHECK(is_forward_mt());
        uintptr_t addr = mtword_ & ~0x1;
        DCHECK_EQ(0, addr % 4);
        return reinterpret_cast<void *>(addr);
    }
    
    uintptr_t mtword_;
}; // class NyObject

static_assert(sizeof(NyObject) == sizeof(uintptr_t), "Incorrect Object size");

class NyTable : public NyObject {
public:
    NyTable *Put(Object *key, Object *value, NyaaCore *N);
    
    Object *Get(Object *key);
private:
    uint32_t seed_;
    uint32_t size_;
    uint32_t capacity_;
    
    struct Entry {
        uint32_t flags;
        Object *key;
        union {
            Object *value;
            Entry  *next;
        };
    };
    
    Entry entries_[0];
}; // class NyTable
    
class NyArray : public NyObject {
public:
    NyArray *Put(int64_t index, Object *value, NyaaCore *N);
    
    Object *Get(int64_t index) const {
        return (index < 0 || index >= size_) ? nullptr : elems_[index];
    }
    
private:
    uint32_t size_;
    uint32_t capacity_;
    Object *elems_[0];
}; // class NyArray

class NyString : public NyObject {
public:
    uint32_t hash_val() const { return hash_val_; }
    
    uint32_t size() const { return size_; }
    
    const char *bytes() const { return bytes_; }
    
private:
    uint32_t hash_val_;
    uint32_t size_;
    char bytes_[4];
}; // class NyString

} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_NYAA_VALUES_H_
