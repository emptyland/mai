#ifndef MAI_NYAA_NYAA_VALUES_H_
#define MAI_NYAA_NYAA_VALUES_H_

#include "mai-lang/handles.h"
#include "nyaa/builtin.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {
    
class Object;
class NyObject;
class NyString;
template<class T> class NyArrayBase;
class NyByteArray;
class NyInt32Array;
class NyArray;
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
class Arguments;

    
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
    
    NyString *Str(NyaaCore *N) const;
    
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
    
class NyInt64 {
public:
    static Object *New(int64_t value) {
        intptr_t word = value;
        return reinterpret_cast<Object *>((word << 1) | 1);
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyInt64);
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
    
    NyString *Str(NyaaCore *N) const;
    
    bool IsDelegated(NyaaCore *N) const;
    bool IsString(NyaaCore *N) const;
    bool IsScript(NyaaCore *N) const;
    bool IsThread(NyaaCore *N) const;
    bool IsTable(NyaaCore *N) const;
    bool IsArray(NyaaCore *N) const;
    bool IsByteArray(NyaaCore *N) const;
    bool IsInt32Array(NyaaCore *N) const;
    bool IsScriptArray(NyaaCore *N) const;
    
    bool IsMap(NyaaCore *N) const {
        return IsArray(N) || IsByteArray(N) || IsInt32Array(N) || IsTable(N);
    }
    
    bool IsRunnable(NyaaCore *N) const {
        return IsScript(N) || IsDelegated(N); // TODO:
    }
    
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
    
class NyMap : public NyObject {
public:
    uint32_t Length() const;
    
    NyMap *Put(Object *key, Object *value, NyaaCore *N);
    
    Object *Get(Object *key, NyaaCore *N);
    
}; // class NyMap

class NyTable : public NyMap {
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
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
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
    
template<class T>
class NyArrayBase : public NyMap {
public:
    inline NyArrayBase(uint32_t capacity)
        : size_(0)
        , capacity_(capacity) {
        ::memset(elems_, 0, sizeof(T) * capacity_);
    }
    
    DEF_VAL_GETTER(uint32_t, size);
    DEF_VAL_GETTER(uint32_t, capacity);
    
    inline T Get(int64_t key) const {
        DCHECK_GE(key, 0);
        DCHECK_LT(key, size());
        return elems_[key];
    }
    
    inline void Refill(const NyArrayBase *base) {
        DCHECK_LE(base->size(), capacity());
        ::memcpy(elems_, base->elems_, base->size() * sizeof(T));
        size_ = base->size();
    }

    inline static size_t RequiredSize(uint32_t capacity) {
        return sizeof(NyArrayBase<T>) + sizeof(T) * capacity;
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyArrayBase);
protected:
    uint32_t size_;
    uint32_t capacity_;
    T elems_[0];
}; // class NyArrayBase


class NyByteArray : public NyArrayBase<Byte> {
public:
    NyByteArray(uint32_t capacity) : NyArrayBase<Byte>(capacity) {}
    
    NyByteArray *Put(int64_t key, Byte value, NyaaCore *N);
    NyByteArray *Add(Byte value, NyaaCore *N);
    NyByteArray *Add(const Byte *value, size_t n, NyaaCore *N);
    
    using NyArrayBase<Byte>::Get;
    using NyArrayBase<Byte>::RequiredSize;
    using NyArrayBase<Byte>::Refill;
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyByteArray);
}; // class NyByteArray
    
    
class NyInt32Array : public NyArrayBase<int32_t> {
public:
    NyInt32Array(uint32_t capacity) : NyArrayBase<int32_t>(capacity) {}
    
    NyInt32Array *Put(int64_t key, int32_t value, NyaaCore *N);
    
    NyInt32Array *Add(int32_t value, NyaaCore *N);

    using NyArrayBase<int32_t>::Get;
    using NyArrayBase<int32_t>::Refill;
    using NyArrayBase<int32_t>::RequiredSize;

    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyInt32Array);
}; // class NyInt32Array
    
    
class NyArray : public NyArrayBase<Object *> {
public:
    NyArray(uint32_t capacity) : NyArrayBase<Object *>(capacity) {}
    
    NyArray *Put(int64_t key, Object *value, NyaaCore *N);
    
    NyArray *Add(Object *value, NyaaCore *N);
    
    using NyArrayBase<Object *>::Get;
    using NyArrayBase<Object *>::Refill;
    using NyArrayBase<Object *>::RequiredSize;
    
    void Refill(const NyArray *base, NyaaCore *N);
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyArray);
}; // class NyArray


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
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyString);
private:
    uint32_t hash_val_;
    uint32_t size_;
    char bytes_[4];
}; // class NyString
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Function and Codes
////////////////////////////////////////////////////////////////////////////////////////////////////
class NyRunnable : public NyObject {
public:
    int Apply(Arguments *, NyaaCore *N);
    
}; // class NyCallable
    

class NyScript : public NyRunnable {
public:
    NyScript(NyString *file_name,
             NyInt32Array *file_info,
             NyByteArray *bcbuf,
             NyArray *const_pool,
             NyaaCore *N);
    
    DEF_PTR_GETTER(NyByteArray, bcbuf);
    DEF_PTR_GETTER(NyString, file_name);
    DEF_PTR_GETTER(NyInt32Array, file_info);
    DEF_PTR_GETTER(NyArray, const_pool);
    
    int Run(NyaaCore *N);
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyScript);
private:
    NyByteArray *bcbuf_;
    NyString *file_name_;
    NyInt32Array *file_info_;
    NyArray *const_pool_;
}; // class NyScript
    

class NyDelegated : public NyRunnable {
public:
    using Kind = DelegatedKind;
    using PropertyGetterFP = Handle<Value> (*)(Local<Value>, Local<String>, Nyaa *);
    using PropertySetterFP = Handle<Value> (*)(Local<Value>, Local<String>, Local<Value>, Nyaa *);
    using Arg0FP = int (*)(Nyaa *);
    using Arg1FP = int (*)(Local<Value>, Nyaa *);
    using Arg2FP = int (*)(Local<Value>, Local<Value>, Nyaa *);
    using Arg3FP = int (*)(Local<Value>, Local<Value>, Local<Value>, Nyaa *);
    using UniversalFP = int (*)(Arguments *, Nyaa *);
    
    NyDelegated(Kind kind, Address fp)
        : kind_(kind), stub_(static_cast<void *>(fp)) {}
    
    int Call(Arguments *, NyaaCore *N);
    
    DEF_VAL_GETTER(Kind, kind);
    
    Address fp_addr() { return static_cast<Address>(stub_); }
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);

    DISALLOW_IMPLICIT_CONSTRUCTORS(NyDelegated);
private:
    Kind kind_;
    union {
        PropertyGetterFP getter_;
        PropertySetterFP setter_;
        Arg0FP call0_;
        Arg1FP call1_;
        Arg2FP call2_;
        Arg3FP call3_;
        UniversalFP calln_;
        void *stub_;
    };
}; // class NyFunction

    
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

    
template<class T> inline Local<T> ApiWarp(Local<Value> api, NyaaCore *N) {
    NyObject *o = reinterpret_cast<NyObject *>(*api);
    return T::EnsureIs(o, N) ? Local<T>::New(o) : Local<T>();
}
    
template<class T> inline Local<T> ApiWarpNoCheck(Local<Value> api, NyaaCore *N) {
    NyObject *o = reinterpret_cast<NyObject *>(*api);
    return Local<T>::New(o);
}

} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_NYAA_VALUES_H_
