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
class NyInt;
class NyString;
template<class T> class NyArrayBase;
class NyByteArray;
class NyInt32Array;
class NyArray;
class NyTable;
class NyFunction;
class NyClosure;
class NyDelegated;
class NyUDO;
class NyThread;
class NyaaCore;
class NyFactory;
class NyRunnable;
    
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
        return IsFloat64() || IsInt(); // TODO:
    }

    bool IsRunnable() const {
        return IsClosure() || IsDelegated(); // TODO:
    }
    
    NyRunnable *ToRunnable() {
        return !IsRunnable() ? nullptr : reinterpret_cast<NyRunnable *>(this);
    }
    
    const NyRunnable *ToRunnable() const {
        return !IsRunnable() ? nullptr : reinterpret_cast<const NyRunnable *>(this);
    }
    
    inline bool IsUDO() const;
    
    inline const NyUDO *ToUDO() const;
    
    inline NyUDO *ToUDO();
    
    inline Object *GetMetaFunction(NyString *name, NyaaCore *N);
    
    inline NyRunnable *GetValidMetaFunction(NyString *name, NyaaCore *N);
    
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
    
#define DEF_HEAP_OBJECT(name) \
    DEF_HEAP_OBJECT_CASTS(name) \
    static bool EnsureIs(const NyObject *o, NyaaCore *N);
    
#define DEF_HEAP_OBJECT_CASTS(name) \
    inline static Ny##name *Cast(Object *ob) { \
        return !ob || !ob->IsObject() ? nullptr : ob->ToHeapObject()->To##name(); \
    } \
    inline static const Ny##name *Cast(const Object *ob) { \
        return !ob || !ob->IsObject() ? nullptr : ob->ToHeapObject()->To##name(); \
    }
    
    
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
    
    DEF_HEAP_OBJECT(Float64);
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyFloat64);
private:
    f64_t value_;
}; // class NyFloat64


class NyInt : public NyObject {
public:
    NyInt(size_t capactiy);
    
    DEF_VAL_GETTER(uint32_t, capacity);
    DEF_VAL_GETTER(uint32_t, offset);
    DEF_VAL_GETTER(uint32_t, header);
    
    Object *Add(Object *rhs, NyaaCore *N) const;
    
    NyInt *Add(int64_t rhs, NyaaCore *N) const;
    
    NyInt *Add(const NyInt *rhs, NyaaCore *N) const;
    
    uint32_t HashVal() const;
    
    bool IsZero() const;
    
    f64_t ToFloat64Val() const;

    int64_t ToInt64Val() const;
    
    NyString *ToString(NyaaCore *N) const;
    
    bool Equals(const NyInt *rhs) const { return Compare(this, rhs) == 0; }
    
    static int Compare(const NyInt *lhs, const NyInt *rhs);
    
    void Iterate(ObjectVisitor *visitor) {}
    
    size_t PlacedSize() const { return RequiredSize(capacity_); }
    
    static size_t RequiredSize(uint32_t capacity) {
        return sizeof(NyInt) + sizeof(uint32_t) * (capacity < 2 ? 0 : capacity);
    }
    
    DEF_HEAP_OBJECT(Int);
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyInt);
private:
    uint32_t capacity_;
    uint32_t offset_;
    uint32_t header_;
    uint32_t vals_[2];
}; // class NyLong


////////////////////////////////////////////////////////////////////////////////////////////////////
// Maps:
////////////////////////////////////////////////////////////////////////////////////////////////////
    
class MapIterator;

class NyMap : public NyObject {
public:
    using Iterator = MapIterator;

    NyMap(NyObject *maybe, uint64_t kid, bool linear, NyaaCore *N);

    uint64_t kid() const { return kid_; }
    void set_kid(uint64_t kid) { kid_ = kid; }
    bool linear() const { return linear_; }
    DEF_PTR_GETTER(NyObject, generic);

    uint32_t Length() const;

    inline std::tuple<Object *, Object *> GetFirstPair();
    inline std::tuple<Object *, Object *> GetNextPair(Object *key, NyaaCore *N);
    
    NyString *ToString(NyaaCore *N);

    void RawPut(Object *key, Object *value, NyaaCore *N);
    Object *RawGet(Object *key, NyaaCore *N);

    void Iterate(ObjectVisitor *visitor) {
        visitor->VisitPointer(this, reinterpret_cast<Object **>(&generic_));
    }

    constexpr size_t PlacedSize() const { return sizeof(*this); }

    friend class MapIterator;
    DEF_HEAP_OBJECT(Map);
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
    
    std::tuple<Object *, Object *> GetFirstPair();
    std::tuple<Object *, Object *> GetNextPair(Object *key, NyaaCore *N);

    NyTable *RawPut(Object *key, Object *value, NyaaCore *N);
    
    Object *RawGet(Object *key, NyaaCore *N);
    
    NyTable *Rehash(NyTable *origin, NyaaCore *N);
    
    void Iterate(ObjectVisitor *visitor);
    
    size_t PlacedSize() const { return RequiredSize(capacity_); }

    static size_t RequiredSize(uint32_t capacity) {
        return sizeof(NyTable) + sizeof(Entry) * capacity;
    }

    friend class MapIterator;
    DEF_HEAP_OBJECT(Table);
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyTable);
private:
    enum Kind : int {
        kFree,
        kSlot,
        kNode,
    };
    struct Entry {
        int     next; // 0 == nullptr
        Kind    kind;
        Object *key; // [strong ref]
        Object *value; // [strong ref]
    };
    
    bool DoPut(Object *key, Object *value, NyaaCore *N);
    
    bool DoDelete(Object *key, NyaaCore *N);
    
    Entry *Alloc() {
        while (free_ > n_slots() + 1) {
            Entry *p = &entries_[free_--];
            if (p->kind == kFree) {
                return p;
            }
        }
        return nullptr;
    }
    
    void Free(Entry *p) {
        DCHECK_GE(p, entries_ + 1 + n_slots());
        DCHECK_LT(p, entries_ + 1 + capacity_);
        int i = Ptr(p);
        p->kind = kFree;
        if (i > free_) {
            free_ = i;
        }
    }
    
    Entry *GetSlot(Object *key, NyaaCore *N) { return At(HashVal(key, N) % n_slots() + 1); }
    
    uint32_t HashVal(Object *key, NyaaCore *N) const {
        return ((!key ? 0 : key->HashVal(N)) | 1u) ^ seed_;
    }
    
    Entry *At(int pos) { return pos == 0 ? nullptr : entries_ + pos; }
    
    const Entry *At(int pos) const { return pos == 0 ? nullptr : entries_ + pos; }
    
    int Ptr(const Entry *p) const { return static_cast<int>(p - entries_); }

    uint32_t const seed_;
    uint32_t const capacity_;
    uint32_t size_;
    uint32_t free_;
    Entry entries_[1]; // [0] is dummy entry
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
    
    inline View<T> GetView(uint32_t begin, uint32_t len) const {
        DCHECK_LE(begin + len, size());
        return MakeView(elems_ + begin, len);
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

    void Iterate(ObjectVisitor *visitor) {}
    
    using NyArrayBase<Byte>::Get;
    using NyArrayBase<Byte>::PlacedSize;
    using NyArrayBase<Byte>::RequiredSize;
    using NyArrayBase<Byte>::Refill;
    using NyArrayBase<Byte>::elems_;
    
    DEF_HEAP_OBJECT(ByteArray);
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyByteArray);
protected:
    Address data() { return elems_; }
    const Byte *data() const { return elems_; }
}; // class NyByteArray


class NyString final : public NyByteArray {
public:
    static constexpr const int kHeaderSize = sizeof(uint32_t);
    //static constexpr const uint32_t kHashMask = 0x7fffffffu;
    
    NyString(const char *s, size_t n, NyaaCore *N);
    
    NyString(size_t capacity)
        : NyByteArray(static_cast<uint32_t>(capacity) + kHeaderSize) {
            uint32_t header = 0;
        NyByteArray::Add(&header, kHeaderSize, nullptr);
        DbgFillInitZag(data(), capacity + kHeaderSize);
    }
    
    NyString *Done(NyaaCore *N);
    
    uint32_t hash_val() const { return header(); }
    
    uint32_t size() const { return size_ - kHeaderSize; }
    
    const char *bytes() const { return reinterpret_cast<const char *>(data() + kHeaderSize); }
    
    using NyByteArray::data;
    
    NyString *Add(const char *s, size_t n, NyaaCore *N) {
        return static_cast<NyString *>(NyByteArray::Add(s, n, N));
    }
    
    NyString *Add(const NyString *s, NyaaCore *N) { return Add(s->bytes(), s->size(), N); }

    int Compare(const char *z, size_t n) const;
    
    int Compare(const NyString *s) const { return Compare(s->bytes(), s->size()); }
    
    Object *TryNumeric(NyaaCore *N) const;
    
    void Iterate(ObjectVisitor *visitor) {}
    
    using NyByteArray::PlacedSize;
    
    static size_t RequiredSize(uint32_t size) {
        return RoundUp(sizeof(NyString) + kHeaderSize + size + 1, kAllocateAlignmentSize);
    }
    
    DEF_HEAP_OBJECT(String);
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyString);
private:
    uint32_t header() const { return *reinterpret_cast<const uint32_t *>(data()); }
}; // class NyString
    
static_assert(sizeof(NyString) == sizeof(NyByteArray), "Incorrect NyString size.");
    
class NyInt32Array : public NyArrayBase<int32_t> {
public:
    NyInt32Array(uint32_t capacity) : NyArrayBase<int32_t>(capacity) {}
    
    //NyInt32Array *Put(int64_t key, int32_t value, NyaaCore *N);
    
    NyInt32Array *Add(int32_t value, NyaaCore *N);
    NyInt32Array *Add(int32_t *values, size_t size, NyaaCore *N);
    
    void Iterate(ObjectVisitor *visitor) {}

    using NyArrayBase<int32_t>::Get;
    using NyArrayBase<int32_t>::Refill;
    using NyArrayBase<int32_t>::PlacedSize;
    using NyArrayBase<int32_t>::RequiredSize;

    DEF_HEAP_OBJECT(Int32Array);
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyInt32Array);
}; // class NyInt32Array
    
    
class NyArray : public NyArrayBase<Object *> {
public:
    NyArray(uint32_t capacity)
        : NyArrayBase<Object *>(capacity) {
        ::memset(elems_, 0, sizeof(Object *) * capacity);
    }
    
    inline std::tuple<Object *, Object *> GetFirstPair();
    inline std::tuple<Object *, Object *> GetNextPair(Object *key);
    
    NyArray *Put(int64_t key, Object *value, NyaaCore *N);
    
    NyArray *Add(Object *value, NyaaCore *N);
    
    using NyArrayBase<Object *>::Get;
    using NyArrayBase<Object *>::Refill;
    using NyArrayBase<Object *>::PlacedSize;
    using NyArrayBase<Object *>::RequiredSize;
    
    void Refill(const NyArray *base, NyaaCore *N);
    
    void Iterate(ObjectVisitor *visitor) { visitor->VisitPointers(this, elems_, elems_ + size_); }

    DEF_HEAP_OBJECT(Array);
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyArray);
}; // class NyArray

    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Function and Codes
////////////////////////////////////////////////////////////////////////////////////////////////////
class NyRunnable : public NyObject {
public:
    int Apply(Arguments *args, int nrets, NyaaCore *N);
    
    DEF_HEAP_OBJECT_CASTS(Runnable);
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyRunnable);
protected:
    NyRunnable() {}
}; // class NyCallable

class NyFunction : public NyObject {
public:
    NyFunction(NyString *name,
               uint8_t n_params,
               bool vargs,
               uint32_t n_upvals,
               uint32_t max_stack_size,
               NyString *file_name,
               NyInt32Array *file_info,
               NyByteArray *bcbuf,
               NyArray *proto_pool,
               NyArray *const_pool,
               NyaaCore *N);
    
    DEF_VAL_GETTER(uint8_t, n_params);
    DEF_VAL_GETTER(bool, vargs);
    DEF_VAL_GETTER(uint32_t, n_upvals);
    DEF_VAL_GETTER(uint64_t, call_count);
    DEF_VAL_GETTER(uint32_t, max_stack);
    DEF_PTR_GETTER(NyString, name);
    DEF_PTR_GETTER(NyByteArray, bcbuf);
    DEF_PTR_GETTER(NyString, file_name);
    DEF_PTR_GETTER(NyInt32Array, file_info);
    DEF_PTR_GETTER(NyArray, proto_pool);
    DEF_PTR_GETTER(NyArray, const_pool);
    
    void SetName(NyString *name, NyaaCore *N);

    struct UpvalDesc {
        NyString *name; // [strong ref]
        bool in_stack;
        int32_t index;
    };

    const UpvalDesc &upval(size_t i) const {
        DCHECK_LT(i, n_upvals_);
        return upvals_[i];
    }
    
    void SetUpval(size_t i, NyString *name, bool in_stack, int32_t index,
                  NyaaCore *N);
    
    static Handle<NyFunction> Compile(const char *z, NyaaCore *N) {
        return Compile(z, !z ? 0 : ::strlen(z), N);
    }
    
    static Handle<NyFunction> Compile(const char *z, size_t n, NyaaCore *N);

    void Iterate(ObjectVisitor *visitor);
    
    size_t PlacedSize() const { return RequiredSize(n_upvals_); }
    
    static size_t RequiredSize(uint32_t n_upvals) {
        return sizeof(NyFunction) + n_upvals * sizeof(upvals_[0]);
    }
    
    static bool EnsureIs(const NyObject *o, NyaaCore *N);

    DISALLOW_IMPLICIT_CONSTRUCTORS(NyFunction);
private:
    uint8_t n_params_;
    bool vargs_;
    uint32_t n_upvals_;
    uint32_t max_stack_;
    uint64_t call_count_ = 0;
    NyString *name_; // [strong ref]
    NyByteArray *bcbuf_; // [strong ref]
    NyString *file_name_; // [strong ref]
    NyInt32Array *file_info_; // [strong ref]
    NyArray *proto_pool_; // [strong ref] internal defined functions.
    NyArray *const_pool_; // [strong ref]
    UpvalDesc upvals_[0]; // elements [strong ref]
}; // class NyCallable
    
class NyClosure : public NyRunnable {
public:
    NyClosure(NyFunction *proto, NyaaCore *N);
    
    DEF_PTR_GETTER(NyFunction, proto);
    Object *upval(int i) const {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, proto_->n_upvals());
        return upvals_[i];
    }
    
    void Bind(int i, Object *upval, NyaaCore *N);
    
    int Call(Arguments *args, int nrets, NyaaCore *N);
    
    static Handle<NyClosure> Compile(const char *z, NyaaCore *N) {
        return Compile(z, !z ? 0 : ::strlen(z), N);
    }
    
    static Handle<NyClosure> Compile(const char *z, size_t n, NyaaCore *N);

    void Iterate(ObjectVisitor *visitor) {
        visitor->VisitPointer(this, reinterpret_cast<Object **>(&proto_));
        visitor->VisitPointers(this, upvals_, upvals_ + proto_->n_upvals());
    }
    
    size_t PlacedSize() const { return RequiredSize(proto_->n_upvals()); }
    
    static size_t RequiredSize(uint32_t n_upvals) {
        return sizeof(NyClosure) + n_upvals * sizeof(Object *);
    }
    
    DEF_HEAP_OBJECT(Closure);
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyClosure);
private:
    NyFunction *proto_; // [strong ref]
    Object *upvals_[0]; // [strong ref]
}; // class NyClosure

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
    
    int Call(Arguments *args, int nrets, NyaaCore *N);
    
    int RawCall(Arguments *args, NyaaCore *N);
    
    DEF_VAL_GETTER(Kind, kind);
    
    Address fp_addr() { return static_cast<Address>(stub_); }

    void Iterate(ObjectVisitor *visitor) {
        visitor->VisitPointers(this, upvals_, upvals_ + n_upvals_);
    }
    
    size_t PlacedSize() const { return RequiredSize(n_upvals_); }
    
    static size_t RequiredSize(uint32_t n_upvals) {
        return sizeof(NyDelegated) + n_upvals * sizeof(Object *);
    }

    DEF_HEAP_OBJECT(Delegated);
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
    using Finalizer = UDOFinalizer;
    
    NyUDO(size_t n_fields, bool dont_visit) : tag_(MakeTag(n_fields, dont_visit)) {}
    
    bool ignore_managed() const { return tag_ & 0x1; }
    
    size_t n_fields() const { return tag_ >> 1; }
    
    Address data() { return reinterpret_cast<Address>(this + 1); }

    void SetFinalizer(Finalizer fp, NyaaCore *N);
    
    NyString *ToString(NyaaCore *N);

    Object *RawGet(Object *key, NyaaCore *N);
    
    void RawPut(Object *key, Object *value, NyaaCore *N);

    Object *GetField(size_t i, NyaaCore *N);

    void SetField(size_t i, Object *value, NyaaCore *N);
    
    void Iterate(ObjectVisitor *visitor) {
        if (!ignore_managed()) {
            visitor->VisitPointers(this, fields_, fields_ + n_fields());
        }
    }

    size_t PlacedSize() const { return RequiredSize(n_fields()); }
    
    static size_t RequiredSize(size_t n_fields) {
        return sizeof(NyUDO) + n_fields * sizeof(Object *);
    }
    
    static size_t GetNFiedls(size_t total_bytes) {
        DCHECK_GE(total_bytes, sizeof(NyUDO));
        return ((total_bytes - sizeof(NyUDO)) + (kPointerSize - 1)) / kPointerSize;
    }

    DEF_HEAP_OBJECT(UDO);
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyUDO);
private:
    static uintptr_t MakeTag(size_t n_fields, bool ignore_managed) {
        return static_cast<uintptr_t>(n_fields << 1 | (ignore_managed ? 0x1 : 0));
    }

    uintptr_t tag_;
    Object *fields_[0];
}; // class NyUDO
    
class MapIterator final {
public:
    MapIterator(NyMap *owns)
        : table_(!owns->linear_ ? owns->table_ : nullptr)
        , array_(owns->linear_ ? owns->array_ : nullptr) {
    }
    
    MapIterator(NyTable *table) : table_(table), array_(nullptr) {}
    
    void SeekFirst();
    
    bool Valid() const {
        if (table_) {
            return index_ < table_->capacity();
        } else {
            return index_ < array_->capacity();
        }
    }

    void Next();
    
    Object *key() const {
        if (table_) {
            return table_->entries_[index_ + 1].key;
        } else {
            return NySmi::New(index_);
        }
    }

    Object *value() const {
        if (table_) {
            return table_->entries_[index_ + 1].value;
        } else {
            return array_->Get(index_);
        }
    }

private:
    NyTable *table_;
    NyArray *array_;
    int64_t index_ = 0;
    
}; // class MapIterator

template<class T>
void UDOFinalizeDtor(NyUDO *udo, NyaaCore *) { static_cast<T *>(udo)->~T(); }

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
    
inline bool NyObject::IsUDO() const {
    return IsThread() || GetMetatable()->kid() > kUdoKidBegin;
}
    
inline const NyUDO *NyObject::ToUDO() const {
    return !IsUDO() ? nullptr : static_cast<const NyUDO *>(this);
}
    
inline Object *NyObject::GetMetaFunction(NyString *name, NyaaCore *N) {
    return GetMetatable()->RawGet(name, N);
}

inline NyRunnable *NyObject::GetValidMetaFunction(NyString *name, NyaaCore *N) {
    return NyRunnable::Cast(GetMetaFunction(name, N));
}

inline std::tuple<Object *, Object *> NyMap::GetFirstPair() {
    if (linear_) {
        return array_->GetFirstPair();
    } else {
        return table_->GetFirstPair();
    }
}

inline std::tuple<Object *, Object *> NyMap::GetNextPair(Object *key, NyaaCore *N) {
    if (!key) {
        return GetFirstPair();
    }
    if (linear_) {
        return array_->GetNextPair(key);
    } else {
        return table_->GetNextPair(key, N);
    }
}

inline std::tuple<Object *, Object *> NyArray::GetFirstPair() {
    for (int64_t i = 0; i < size(); ++i) {
        Object *val = elems_[i];
        if (val) {
            return {NySmi::New(i), val};
        }
    }
    return {nullptr, nullptr};
}

inline std::tuple<Object *, Object *> NyArray::GetNextPair(Object *key) {
    int64_t idx = key->ToSmi();
    for (int64_t i = idx + 1; i < size(); ++i) {
        Object *val = elems_[i];
        if (val) {
            return {NySmi::New(i), val};
        }
    }
    return {nullptr, nullptr};
}

inline NyUDO *NyObject::ToUDO() { return !IsUDO() ? nullptr : static_cast<NyUDO *>(this); }
    
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
