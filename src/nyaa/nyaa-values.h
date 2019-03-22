#ifndef MAI_NYAA_NYAA_VALUES_H_
#define MAI_NYAA_NYAA_VALUES_H_

#include "mai-lang/handles.h"
#include "nyaa/builtin.h"
#include "nyaa/memory.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {
    
class Object;
class NyObject;
class NyFloat64;
class NyLong;
class NyString;
template<class T> class NyArrayBase;
class NyByteArray;
class NyInt32Array;
class NyArray;
class NyTable;
class NyScript;
class NyFunction;
class NyDelegated;
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
    
    bool IsSmi() const { return word() & 0x1; }
    
    bool IsObject() const { return !IsSmi(); }
    
    uintptr_t word() const { return reinterpret_cast<uintptr_t>(this); }
    
    int64_t ToSmi() const {
        DCHECK(IsSmi());
        return static_cast<int64_t>(word() >> 1);
    }
    
    NyObject *ToHeapObject() const {
        DCHECK(IsObject());
        return reinterpret_cast<NyObject *>(word());
    }
    
    bool IsNil() const { return this == kNil; }
    
    bool IsTrue() const { return !IsNil() && !IsFalse(); }
    
    bool IsFalse() const;
    
    NyString *ToString(NyaaCore *N);
    
    bool IsKey(NyaaCore *N) const;
    
    bool IsNotKey(NyaaCore *N) const { return !IsKey(N); }

    uint32_t HashVal(NyaaCore *N) const;
    
    static bool Equals(Object *lhs, Object *rhs, NyaaCore *N);
    
    static Object *Add(Object *lhs, Object *rhs, NyaaCore *N);
    
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
    
class NySmi {
public:
    static Object *New(int64_t value) {
        intptr_t word = value;
        return reinterpret_cast<Object *>((word << 1) | 1);
    }
    
    static Object *Add(Object *lhs, Object *rhs, NyaaCore *N);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NySmi);
}; // class NyInt32
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Object in Heap
////////////////////////////////////////////////////////////////////////////////////////////////////
class NyObject : public Object {
public:
    NyObject() : mtword_(0) {}

    bool is_forward_mt() const { return mtword_ & 0x1; }
    
    bool is_direct_mt() const { return !is_forward_mt(); }
    
    NyMap *GetMetatable() const {
        void *addr = is_forward_mt() ? forward_address() : reinterpret_cast<void *>(mtword_);
        return reinterpret_cast<NyMap *>(addr);
    }
    
    void SetMetatable(NyMap *mt, NyaaCore *N);
    
    bool Equals(Object *rhs, NyaaCore *N);
    
    Object *Add(Object *rhs, NyaaCore *N);
    
    NyString *ToString(NyaaCore *N);
    
#define DECL_TYPE_CHECK(type) inline bool Is##type() const;
    DECL_BUILTIN_TYPES(DECL_TYPE_CHECK)
#undef DECL_TYPE_CHECK
    
#define DECL_TYPE_CAST(type) \
    inline Ny##type *To##type (); \
    inline const Ny##type *To##type () const;
    DECL_BUILTIN_TYPES(DECL_TYPE_CAST)
#undef DECL_TYPE_CAST
    
    bool IsNumeric() const {
        return IsFloat64() || IsLong(); // TODO:
    }

    bool IsRunnable() const {
        return IsScript() || IsDelegated() || IsFunction(); // TODO:
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyObject);
private:
    void *forward_address() const {
        DCHECK(is_forward_mt());
        uintptr_t addr = mtword_ & ~0x1;
        DCHECK_EQ(0, addr % 4);
        return reinterpret_cast<void *>(addr);
    }
    
    uintptr_t mtword_; // [strong ref]
}; // class NyObject

static_assert(sizeof(NyObject) == sizeof(uintptr_t), "Incorrect Object size");
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Inbox Numbers:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
class NyFloat64 : public NyObject {
public:
    NyFloat64(f64_t value) : value_(value) {}
    
    f64_t value() const { return value_; }
    
    Object *Add(Object *rhs, NyaaCore *N) const;
    
    uint32_t HashVal() const {
        uint64_t bits = *reinterpret_cast<const uint64_t *>(&value_);
        return static_cast<uint32_t>(bits * bits >> 16);
    }
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyFloat64);
private:
    f64_t value_;
}; // class NyFloat64


class NyLong : public NyObject {
public:
    NyLong(size_t capactiy);
    
    DEF_VAL_GETTER(uint32_t, capacity);
    DEF_VAL_GETTER(uint32_t, offset);
    DEF_VAL_GETTER(uint32_t, header);
    
    Object *Add(Object *rhs, NyaaCore *N) const;
    
    NyLong *Add(int64_t rhs, NyaaCore *N) const;
    
    NyLong *Add(const NyLong *rhs, NyaaCore *N) const;
    
    uint32_t HashVal() const;
    
    bool IsZero() const;
    
    f64_t ToFloat64Val() const;

    int64_t ToInt64Val() const;
    
    NyString *ToString(NyaaCore *N) const;
    
    bool Equals(const NyLong *rhs) const { return Compare(this, rhs) == 0; }
    
    static int Compare(const NyLong *lhs, const NyLong *rhs);
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyLong);
private:
    uint32_t capacity_;
    uint32_t offset_;
    uint32_t header_;
    uint32_t vals_[2];
}; // class NyLong


////////////////////////////////////////////////////////////////////////////////////////////////////
// Maps:
////////////////////////////////////////////////////////////////////////////////////////////////////

class NyMap : public NyObject {
public:
    NyMap(NyObject *maybe, uint64_t kid, bool linear, NyaaCore *N);
    
    uint64_t kid() const { return kid_; }
    
    uint32_t Length() const;
    
    void Put(Object *key, Object *value, NyaaCore *N);
    
    Object *Get(Object *key, NyaaCore *N);
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyMap);
private:
    union {
        NyTable *table_; // [strong ref]
        NyArray *array_; // [strong ref]
        NyObject *generic_; // [strong ref]
    };
    uint64_t linear_ : 8; // fast cache flag.
    uint64_t kid_ : 56;
}; // class NyMap

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
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyTable);
private:
    enum Kind {
        kFree,
        kSlot,
        kNode,
    };
    struct Entry {
        Entry  *next;
        Kind    kind;
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// Arrays:
////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
class NyArrayBase : public NyObject {
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
        return RoundUp(sizeof(NyArrayBase<T>) + sizeof(T) * capacity, kAllocateAlignmentSize);
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

    NyByteArray *Add(Byte value, NyaaCore *N);
    NyByteArray *Add(const void *value, size_t n, NyaaCore *N);
    
    View<Byte> View(uint32_t len) {
        len = std::min(len, size());
        return MakeView(elems_, len);
    }
    
    using NyArrayBase<Byte>::Get;
    using NyArrayBase<Byte>::RequiredSize;
    using NyArrayBase<Byte>::Refill;
    using NyArrayBase<Byte>::elems_;
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyByteArray);
protected:
    Address data() { return elems_; }
    const Byte *data() const { return elems_; }
}; // class NyByteArray


class NyString final : public NyByteArray {
public:
    static constexpr const int kHeaderSize = sizeof(uint32_t);
    static constexpr const uint32_t kHashMask = 0x7fffffffu;
    
    NyString(const char *s, size_t n, NyaaCore *N);
    
    NyString(size_t capacity)
        : NyByteArray(static_cast<uint32_t>(capacity) + kHeaderSize) {
        DbgFillInitZag(data(), capacity + kHeaderSize);
    }
    
    NyString *Done(NyaaCore *N);
    
    NyString *Add(const char *s, size_t n, NyaaCore *N) {
        return static_cast<NyString *>(NyByteArray::Add(s, n, N)->Add(0, N));
    }
    
    NyString *Add(const NyString *s, NyaaCore *N) { return Add(s->bytes(), s->size(), N); }

    uint32_t hash_val() const { return header() & kHashMask; }
    
    const char *bytes() const { return reinterpret_cast<const char *>(data() + kHeaderSize); }
    
    Object *TryNumeric(NyaaCore *N) const;
    
    static size_t RequiredSize(uint32_t size) {
        return RoundUp(sizeof(NyString) + sizeof(uint32_t) + size + 1, kAllocateAlignmentSize);
    }
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
private:
    uint32_t header() const { return *reinterpret_cast<const uint32_t *>(data()); }
}; // class NyString
    
static_assert(sizeof(NyString) == sizeof(NyByteArray), "Incorrect NyString size.");
    
class NyInt32Array : public NyArrayBase<int32_t> {
public:
    NyInt32Array(uint32_t capacity) : NyArrayBase<int32_t>(capacity) {}
    
    //NyInt32Array *Put(int64_t key, int32_t value, NyaaCore *N);
    
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


//class NyString : public NyObject {
//public:
//    NyString(const char *s, size_t n);
//
//    DEF_VAL_GETTER(uint32_t, hash_val);
//    DEF_VAL_GETTER(uint32_t, size);
//
//    const char *bytes() const { return bytes_; }
//
//    static size_t RequiredSize(uint32_t size) {
//        size = size < 3 ? 0 : size - 3;
//        return sizeof(NyString) + size;
//    }
//
//    static bool EnsureIs(const NyObject *o, NyaaCore *N);
//
//    DISALLOW_IMPLICIT_CONSTRUCTORS(NyString);
//private:
//    uint32_t hash_val_;
//    uint32_t size_;
//    char bytes_[4];
//}; // class NyString
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Function and Codes
////////////////////////////////////////////////////////////////////////////////////////////////////
class NyRunnable : public NyObject {
public:
    int Apply(Arguments *, NyaaCore *N);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyRunnable);
protected:
    NyRunnable() {}
}; // class NyCallable
    
    
class NyFunction : public NyRunnable {
public:
    NyFunction(uint8_t n_params,
               bool vargs,
               uint32_t max_stack_size,
               NyScript *script,
               NyaaCore *N);
    
    DEF_VAL_GETTER(uint8_t, n_params);
    DEF_VAL_GETTER(bool, vargs);
    DEF_VAL_GETTER(uint32_t, max_stack_size);
    DEF_PTR_GETTER(NyScript, script);
    DEF_VAL_GETTER(uint64_t, call_count);
    
    int Call(Arguments *, NyaaCore *N);
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyFunction);
private:
    uint8_t n_params_;
    bool vargs_;
    uint32_t max_stack_size_;
    NyScript *script_; // [strong ref]
    uint64_t call_count_ = 0;
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
    NyByteArray *bcbuf_; // [strong ref]
    NyString *file_name_; // [strong ref]
    NyInt32Array *file_info_; // [strong ref]
    NyArray *const_pool_; // [strong ref]
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

#define DECL_TYPE_CHECK(type) inline bool NyObject::Is##type() const { \
        return GetMetatable()->kid() == kType##type; \
    }
    DECL_BUILTIN_TYPES(DECL_TYPE_CHECK)
#undef DECL_TYPE_CHECK
    
#define DECL_TYPE_CAST(type) \
    inline Ny##type *NyObject::To##type () { \
        return !Is##type() ? nullptr : reinterpret_cast<Ny##type *>(this); \
    } \
    inline const Ny##type *NyObject::To##type () const { \
        return !Is##type() ? nullptr : reinterpret_cast<const Ny##type *>(this); \
    }
    DECL_BUILTIN_TYPES(DECL_TYPE_CAST)
#undef DECL_TYPE_CAST
    
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
