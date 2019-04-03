#ifndef MAI_NYAA_NYAA_VALUES_H_
#define MAI_NYAA_NYAA_VALUES_H_

#include "mai-lang/handles.h"
#include "nyaa/builtin.h"
#include "nyaa/memory.h"
#include "nyaa/visitors.h"
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
//class ObjectVisitor;
    
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
    
    bool IsWeak() const { return IsObject() && (word() & 0x2); }
    
    uintptr_t word() const { return reinterpret_cast<uintptr_t>(this); }
    
    int64_t ToSmi() const {
        DCHECK(IsSmi());
        return static_cast<int64_t>(word()) >> 2;
    }
    
    NyObject *ToHeapObject() const {
        DCHECK(IsObject());
        return reinterpret_cast<NyObject *>(word() & ~0x3);
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
        return reinterpret_cast<Object *>((word << 2) | 1);
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyInt32);
}; // class NyInt32
    
class NySmi {
public:
    static constexpr const int64_t kMaxValue =  0x1fffffffffffffffll;
    static constexpr const int64_t kMinValue = -0x1fffffffffffffffll;
    
    static Object *New(int64_t value) {
        intptr_t word = value;
        return reinterpret_cast<Object *>((word << 2) | 1);
    }
    
    static Object *Add(Object *lhs, Object *rhs, NyaaCore *N);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NySmi);
}; // class NyInt32
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Object in Heap
////////////////////////////////////////////////////////////////////////////////////////////////////
class NyObject : public Object {
public:
#if defined(MAI_OS_DARWIN)
    static constexpr const int kMaskBitsOrder = 42;
    static constexpr const uintptr_t kColorMask = 0xfull << kMaskBitsOrder;
#endif
    
    NyObject() : mtword_(0) {}

    bool is_forward() const { return mtword_ & 0x1; }
    
    bool is_direct() const { return !is_forward(); }

    NyObject *Foward() const {
        return is_forward() ? static_cast<NyObject *>(forward_address()) : nullptr;
    }

#if defined(NYAA_USE_POINTER_COLOR)
    HeapColor GetColor() const {
        return static_cast<HeapColor>((mtword_ & kColorMask) >> kMaskBitsOrder);
    }

    void SetColor(HeapColor color) {
        mtword_ &= ~kColorMask;
        mtword_ |= ((static_cast<uintptr_t>(color) & 0xfull) << kMaskBitsOrder);
    }
#endif

    NyMap *GetMetatable() const {
        DCHECK_EQ(mtword_ % 4, 0);
#if defined(NYAA_USE_POINTER_COLOR)
        return reinterpret_cast<NyMap *>(mtword_ & ~kColorMask);
#else // !defined(NYAA_USE_POINTER_COLOR)
        return reinterpret_cast<NyMap *>(mtword_);
#endif // defined(NYAA_USE_POINTER_COLOR)
    }

    void SetMetatable(NyMap *mt, NyaaCore *N);
    
    void Iterate(ObjectVisitor *visitor);
    
    // Real size in heap.
    size_t PlacedSize() const;
    
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
        DCHECK(is_forward());
        uintptr_t addr = mtword_ & ~0x1;
        DCHECK_EQ(0, addr % 4);
#if defined(NYAA_USE_POINTER_COLOR)
        return reinterpret_cast<void *>(addr & ~kColorMask);
#else // !defined(NYAA_USE_POINTER_COLOR)
        return reinterpret_cast<void *>(addr);
#endif // defined(NYAA_USE_POINTER_COLOR)
    }
    
    // 0x0000000101719fe0
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
    
    void Iterate(ObjectVisitor *visitor) {}
    
    constexpr size_t PlacedSize() const { return sizeof(*this); }
    
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
    
    void Iterate(ObjectVisitor *visitor) {}
    
    size_t PlacedSize() const { return RequiredSize(capacity_); }
    
    static size_t RequiredSize(uint32_t capacity) {
        return sizeof(NyLong) + sizeof(uint32_t) * (capacity < 2 ? 0 : capacity);
    }
    
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
    
    void RawPut(Object *key, Object *value, NyaaCore *N);
    
    Object *RawGet(Object *key, NyaaCore *N);
    
    void Iterate(ObjectVisitor *visitor) {
        visitor->VisitPointer(this, reinterpret_cast<Object **>(&generic_));
    }
    
    constexpr size_t PlacedSize() const { return sizeof(*this); }
    
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
    
    void Iterate(ObjectVisitor *visitor);
    
    size_t PlacedSize() const { return RequiredSize(capacity_); }

    enum Kind {
        kFree,
        kSlot,
        kNode,
    };
    struct Entry {
        Entry  *next;
        Kind    kind;
        Object *key; // [strong ref]
        Object *value; // [strong ref]
    };
    
    static size_t RequiredSize(uint32_t capacity) {
        return sizeof(NyTable) + sizeof(Entry) * capacity;
    }
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyTable);
private:

    
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
        //::memset(elems_, 0, sizeof(T) * capacity_);
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
    
    inline size_t PlacedSize() const { return RequiredSize(capacity_); }

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
    
    void Iterate(ObjectVisitor *visitor) {}
    
    using NyArrayBase<Byte>::Get;
    using NyArrayBase<Byte>::PlacedSize;
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
        return static_cast<NyString *>(NyByteArray::Add(s, n, N));
    }
    
    NyString *Add(const NyString *s, NyaaCore *N) { return Add(s->bytes(), s->size(), N); }

    uint32_t hash_val() const { return header() & kHashMask; }
    
    uint32_t size() const { return size_ - kHeaderSize; }
    
    const char *bytes() const { return reinterpret_cast<const char *>(data() + kHeaderSize); }
    
    Object *TryNumeric(NyaaCore *N) const;
    
    void Iterate(ObjectVisitor *visitor) {}
    
    using NyByteArray::PlacedSize;
    
    static size_t RequiredSize(uint32_t size) {
        return RoundUp(sizeof(NyString) + kHeaderSize + size + 1, kAllocateAlignmentSize);
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
    
    void Iterate(ObjectVisitor *visitor) {}

    using NyArrayBase<int32_t>::Get;
    using NyArrayBase<int32_t>::Refill;
    using NyArrayBase<int32_t>::PlacedSize;
    using NyArrayBase<int32_t>::RequiredSize;

    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyInt32Array);
}; // class NyInt32Array
    
    
class NyArray : public NyArrayBase<Object *> {
public:
    NyArray(uint32_t capacity)
        : NyArrayBase<Object *>(capacity) {
        ::memset(elems_, 0, sizeof(Object *) * capacity);
    }
    
    NyArray *Put(int64_t key, Object *value, NyaaCore *N);
    
    NyArray *Add(Object *value, NyaaCore *N);
    
    using NyArrayBase<Object *>::Get;
    using NyArrayBase<Object *>::Refill;
    using NyArrayBase<Object *>::PlacedSize;
    using NyArrayBase<Object *>::RequiredSize;
    
    void Refill(const NyArray *base, NyaaCore *N);
    
    void Iterate(ObjectVisitor *visitor) { visitor->VisitPointers(this, elems_, elems_ + size_); }
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyArray);
}; // class NyArray

    
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
               uint32_t n_upvals,
               NyScript *script,
               NyaaCore *N);
    
    DEF_VAL_GETTER(uint8_t, n_params);
    DEF_VAL_GETTER(bool, vargs);
    
    DEF_VAL_GETTER(uint32_t, n_upvals);
    DEF_PTR_GETTER(NyScript, script);
    DEF_VAL_GETTER(uint64_t, call_count);

    Object *upval(int i) const {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, n_upvals_);
        return upvals_[i];
    }
    
    void Bind(int i, Object *upval, NyaaCore *N);
    
    int Call(Arguments *, NyaaCore *N);
    
    void Iterate(ObjectVisitor *visitor) {
        visitor->VisitPointer(this, reinterpret_cast<Object **>(&script_));
        visitor->VisitPointers(this, upvals_, upvals_ + n_upvals_);
    }
    
    size_t PlacedSize() const { return RequiredSize(n_upvals_); }
    
    static size_t RequiredSize(uint32_t n_upvals) {
        return sizeof(NyFunction) + n_upvals * sizeof(Object *);
    }
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyFunction);
private:
    uint8_t n_params_;
    bool vargs_;
    
    uint32_t n_upvals_;
    uint64_t call_count_ = 0;
    NyScript *script_; // [strong ref]
    Object *upvals_[0]; // [strong ref], MUST BE TAIL
}; // class NyCallable
    

class NyScript : public NyRunnable {
public:
    NyScript(uint32_t max_stack_size,
             NyString *file_name,
             NyInt32Array *file_info,
             NyByteArray *bcbuf,
             NyArray *const_pool,
             NyaaCore *N);
    
    DEF_VAL_GETTER(uint32_t, max_stack_size);
    DEF_PTR_GETTER(NyByteArray, bcbuf);
    DEF_PTR_GETTER(NyString, file_name);
    DEF_PTR_GETTER(NyInt32Array, file_info);
    DEF_PTR_GETTER(NyArray, const_pool);
    
    int Run(NyaaCore *N);
    
    void Iterate(ObjectVisitor *visitor) {
        visitor->VisitPointer(this, reinterpret_cast<Object **>(&bcbuf_));
        visitor->VisitPointer(this, reinterpret_cast<Object **>(&file_name_));
        visitor->VisitPointer(this, reinterpret_cast<Object **>(&file_info_));
        visitor->VisitPointer(this, reinterpret_cast<Object **>(&const_pool_));
    }

    constexpr size_t PlacedSize() const { return sizeof(*this); }
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyScript);
private:
    uint32_t unused_; // TODO:
    uint32_t max_stack_size_;
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
    
    NyDelegated(Kind kind, Address fp, uint32_t n_upvals)
        : kind_(kind)
        , stub_(static_cast<void *>(fp))
        , n_upvals_(n_upvals) {
        ::memset(upvals_, 0, n_upvals * sizeof(Object *));
    }
    
    DEF_VAL_GETTER(uint32_t, n_upvals);
    
    Object *upval(int i) const {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, n_upvals_);
        return upvals_[i];
    }
    
    void Bind(int i, Object *upval, NyaaCore *N);
    
    int Call(Arguments *, NyaaCore *N);
    
    DEF_VAL_GETTER(Kind, kind);
    
    Address fp_addr() { return static_cast<Address>(stub_); }
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
    void Iterate(ObjectVisitor *visitor) {
        visitor->VisitPointers(this, upvals_, upvals_ + n_upvals_);
    }
    
    size_t PlacedSize() const { return RequiredSize(n_upvals_); }
    
    static size_t RequiredSize(uint32_t n_upvals) {
        return sizeof(NyDelegated) + n_upvals * sizeof(Object *);
    }

    DISALLOW_IMPLICIT_CONSTRUCTORS(NyDelegated);
private:
    Kind kind_;
    uint32_t n_upvals_;
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
    Object *upvals_[0]; // [strong ref], MUST BE TAIL
}; // class NyFunction

    
////////////////////////////////////////////////////////////////////////////////////////////////////
// User Definition Object
////////////////////////////////////////////////////////////////////////////////////////////////////
class NyUDO : public NyObject {
public:
    NyUDO() {}

    DISALLOW_IMPLICIT_CONSTRUCTORS(NyUDO);
private:
    Object *fields_[0];
}; // class NyUDO

//template<class T>
//void UDOFinalizeDtor(Nyaa *, NyUDO *udo) { static_cast<T *>(udo)->~T(); }

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
