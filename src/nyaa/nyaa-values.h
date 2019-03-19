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
class NyArrayBase;
class NyByteArray;
class NyInt32Array;
class NyObjectArray;
class NyTable;
class NyScript;
class NyUDO;
class NyThread;
class NyaaCore;
class NyFactory;
    
class String;
class Array;
class Table;
class Script;
class TryCatch;

    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Object
////////////////////////////////////////////////////////////////////////////////////////////////////
class Object {
public:
    static Object *const kNil;
    
    bool is_smi() const { return word() & 0x1; }
    
    bool is_object() const { return !is_smi(); }
    
    uintptr_t word() const { return reinterpret_cast<uintptr_t>(this); }
    
    int64_t smi() const {
        DCHECK(is_smi());
        return static_cast<int64_t>(word() >> 1);
    }
    
    NyObject *heap_object() const {
        DCHECK(is_object());
        return reinterpret_cast<NyObject *>(word());
    }
    
    bool IsKey(NyaaCore *N) const;
    bool IsNotKey(NyaaCore *N) const { return !IsKey(N); }

    uint32_t HashVal(NyaaCore *N) const;
    
    static bool Equals(Object *lhs, Object *rhs, NyaaCore *N);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Object);
protected:
    Object() {}
}; // class Value
    
    
class NyInt32 {
public:
    static Object *New(int32_t value) {
        intptr_t word = value;
        return reinterpret_cast<Object *>((word << 1) | 1);
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyInt32);
}; // class NyInt32
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Object in Heap
////////////////////////////////////////////////////////////////////////////////////////////////////
class NyObject : public Object {
public:
    NyObject() : mtword_(0) {}

    bool is_forward_mt() const { return mtword_ & 0x1; }
    
    bool is_direct_mt() const { return !is_forward_mt(); }
    
    NyTable *GetMetatable() const {
        void *addr = is_forward_mt() ? forward_address() : reinterpret_cast<void *>(mtword_);
        return *reinterpret_cast<NyTable **>(addr);
    }
    
    void SetMetatable(NyTable *mt, NyaaCore *N);
    
    bool Equals(Object *rhs, NyaaCore *N) const;
    
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
    NyTable(uint32_t seed, uint32_t capacity);
    
    DEF_VAL_GETTER(uint32_t, size);
    DEF_VAL_GETTER(uint32_t, capacity);
    
    uint32_t n_slots() const { return capacity_ >> 1; }

    NyTable *Put(Object *key, Object *value, NyaaCore *N);
    
    Object *Get(Object *key, NyaaCore *N);
    
    NyTable *Rehash(NyTable *origin, NyaaCore *N);

    static size_t RequiredSize(uint32_t capacity) {
        return sizeof(NyTable) + sizeof(Entry) * capacity;
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyTable);
private:
    enum EKind {
        kFree,
        kSlot,
        kNode,
    };
    struct Entry {
        Entry  *next;
        EKind   kind;
        Object *key;
        Object *value;
    };
    
    bool DoPut(Object *key, Object *value, NyaaCore *N);
    
    Entry *GetSlot(Object *key, NyaaCore *N) { return GetSlot(HashVal(key, N) % n_slots()); }
    
    Entry *GetSlot(uint32_t i) { DCHECK_LT(i, n_slots()); return &entries_[i]; }
    
    uint32_t HashVal(Object *key, NyaaCore *N) const {
        return ((!key ? 0 : key->HashVal(N)) | 1u) ^ seed_;
    }
    
    uint32_t const seed_;
    uint32_t const capacity_;
    uint32_t size_;
    Entry *free_;
    Entry entries_[0];
}; // class NyTable
    
class NyArrayBase : public NyObject {
public:
    DEF_VAL_GETTER(uint32_t, size);
    DEF_VAL_GETTER(uint32_t, capacity);

    Address payload() { return elems_; }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyArrayBase);
private:
    uint32_t size_;
    uint32_t capacity_;
    Byte elems_[0];
}; // class NyArrayBase


class NyByteArray : public NyArrayBase {
public:
    NyTable *Put(int64_t key, Byte value, NyaaCore *N);
    
    Byte Get(int64_t key) {
        DCHECK_GE(key, 0);
        DCHECK_LT(key, size());
        return payload()[key];
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyByteArray);
}; // class NyArrayBase


class NyString : public NyObject {
public:
    NyString(const char *s, size_t n);
    
    DEF_VAL_GETTER(uint32_t, hash_val);
    DEF_VAL_GETTER(uint32_t, size);
    
    const char *bytes() const { return bytes_; }
    
    static size_t RequiredSize(uint32_t size) {
        size = size < 3 ? 0 : size - 3;
        return sizeof(NyString) + size;
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyString);
private:
    uint32_t hash_val_;
    uint32_t size_;
    char bytes_[4];
}; // class NyString

    
////////////////////////////////////////////////////////////////////////////////////////////////////
// User Definition Object
////////////////////////////////////////////////////////////////////////////////////////////////////
class NyUDO : public NyObject {
public:
    using FinalizeFP = void (*)(Nyaa *, NyUDO *);
    
    explicit NyUDO(FinalizeFP fzfp) : fzfp_(fzfp) {}
    
    Object *GetField(NyString *name);
    
    bool SetField(NyString *name, Object *value, NyaaCore *N);
    
    DEF_VAL_PROP_RW(FinalizeFP, fzfp);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyUDO);
private:
    FinalizeFP fzfp_;
}; // class NyUDO

template<class T>
void UDOFinalizeDtor(Nyaa *, NyUDO *udo) { static_cast<T *>(udo)->~T(); }

} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_NYAA_VALUES_H_
