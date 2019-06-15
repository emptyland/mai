#ifndef MAI_NYAA_NYAA_VALUES_H_
#define MAI_NYAA_NYAA_VALUES_H_

#include "mai-lang/handles.h"
#include "nyaa/builtin.h"
#include "nyaa/memory.h"
#include "nyaa/visitors.h"
#include "asm/utils.h"
#include "base/base.h"
#include "glog/logging.h"
#include <math.h>
#include <numeric>

namespace mai {
namespace base {
class Arena;
} // namespace base
namespace nyaa {
    
class ObjectFactory;
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
class NyCode;
    
class String;
class Array;
class Table;
class Script;
class TryCatch;
template<class T> class FunctionCallbackInfo;

    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Object
////////////////////////////////////////////////////////////////////////////////////////////////////
class Object {
public:
    using Template = arch::ObjectTemplate<Object, int32_t>;
    
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
    
    inline BuiltinType GetType() const;

    bool IsNil() const { return this == kNil; }
    
    bool IsTrue() const { return !IsNil() && !IsFalse(); }
    
    bool IsFalse() const;
    
    NyString *ToString(NyaaCore *N);
    
    bool IsKey(NyaaCore *N) const;
    
    bool IsNotKey(NyaaCore *N) const { return !IsKey(N); }

    uint32_t HashVal(NyaaCore *N) const;
    
    static bool Equal(Object *lhs, Object *rhs, NyaaCore *N);
    static bool LessThan(Object *lhs, Object *rhs, NyaaCore *N);
    static bool LessEqual(Object *lhs, Object *rhs, NyaaCore *N);
    
    static Object *Add(Object *lhs, Object *rhs, NyaaCore *N);
    static Object *Sub(Object *lhs, Object *rhs, NyaaCore *N);
    static Object *Mul(Object *lhs, Object *rhs, NyaaCore *N);
    static Object *Div(Object *lhs, Object *rhs, NyaaCore *N);
    static Object *Mod(Object *lhs, Object *rhs, NyaaCore *N);
    
    static Object *Get(Object *lhs, Object *key, NyaaCore *N);
    static void Put(Object *lhs, Object *key, Object *value, NyaaCore *N);
    
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
    //static constexpr const int64_t kMinValue = -0x1fffffffffffffffll;
    static constexpr const int64_t kMinValue = -0x2000000000000000ll;
    
    static Object *New(int64_t value) {
        intptr_t word = value;
        return reinterpret_cast<Object *>((word << 2) | 1);
    }
    
    static Object *Add(Object *lhs, Object *rhs, NyaaCore *N);
    static Object *Sub(Object *lhs, Object *rhs, NyaaCore *N);
    static Object *Mul(Object *lhs, Object *rhs, NyaaCore *N);
    static Object *Div(Object *lhs, Object *rhs, NyaaCore *N);
    static Object *Mod(Object *lhs, Object *rhs, NyaaCore *N);

    DISALLOW_IMPLICIT_CONSTRUCTORS(NySmi);
}; // class NyInt32
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Object in Heap
////////////////////////////////////////////////////////////////////////////////////////////////////
class NyObject : public Object {
public:
#if defined(NYAA_USE_POINTER_COLOR) || defined(NYAA_USE_POINTER_TYPE)
    static constexpr const int kColorBitsOrder = 42;
    static constexpr const uintptr_t kColorMask = 0xfull << kColorBitsOrder;
    static constexpr const int kTypeBitsOrder = kColorBitsOrder + 4;
    static constexpr const uintptr_t kTypeMask = 0xffull << kTypeBitsOrder;
    static constexpr const uintptr_t kDataMask = kColorMask | kTypeMask;
#endif
    
    NyObject() : mtword_(0) {}

    bool is_forward() const { return mtword_ & 0x1; }
    
    bool is_direct() const { return !is_forward(); }

    NyObject *Foward() const {
        return is_forward() ? static_cast<NyObject *>(forward_address()) : nullptr;
    }

#if defined(NYAA_USE_POINTER_COLOR)
    HeapColor GetColor() const {
        return static_cast<HeapColor>((mtword_ & kColorMask) >> kColorBitsOrder);
    }

    void SetColor(HeapColor color) {
        mtword_ &= ~kColorMask;
        mtword_ |= ((static_cast<uintptr_t>(color) & 0xfull) << kColorBitsOrder);
    }
#endif
    
    inline BuiltinType GetType() const;
    
    inline void SetType(BuiltinType type);

    NyMap *GetMetatable() const {
        DCHECK_EQ(mtword_ % 4, 0);
#if defined(NYAA_USE_POINTER_COLOR) || defined(NYAA_USE_POINTER_TYPE)
        return reinterpret_cast<NyMap *>(mtword_ & ~kDataMask);
#else // !defined(NYAA_USE_POINTER_COLOR) && !defined(NYAA_USE_POINTER_TYPE)
        return reinterpret_cast<NyMap *>(mtword_);
#endif // defined(NYAA_USE_POINTER_COLOR) || defined(NYAA_USE_POINTER_TYPE)
    }

    void SetMetatable(NyMap *mt, NyaaCore *N);
    
    void Iterate(ObjectVisitor *visitor);
    
    // Real size in heap.
    size_t PlacedSize() const;
    
    bool Equal(Object *rhs, NyaaCore *N);
    bool LessThan(Object *rhs, NyaaCore *N);
    bool LessEqual(Object *rhs, NyaaCore *N);
    
    Object *Add(Object *rhs, NyaaCore *N);
    Object *Sub(Object *rhs, NyaaCore *N);
    Object *Mul(Object *rhs, NyaaCore *N);
    Object *Div(Object *rhs, NyaaCore *N);
    Object *Mod(Object *rhs, NyaaCore *N);
    
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
    
    inline Object *GetMetaFunction(NyString *name, NyaaCore *N) const;
    inline NyRunnable *GetValidMetaFunction(NyString *name, NyaaCore *N) const;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyObject);
private:
    void *forward_address() const {
        DCHECK(is_forward());
        uintptr_t addr = mtword_ & ~0x1;
        DCHECK_EQ(0, addr % 4);
#if defined(NYAA_USE_POINTER_COLOR) || defined(NYAA_USE_POINTER_TYPE)
        return reinterpret_cast<void *>(addr & ~kDataMask);
#else // !defined(NYAA_USE_POINTER_COLOR) && !defined(NYAA_USE_POINTER_TYPE)
        return reinterpret_cast<void *>(addr);
#endif // defined(NYAA_USE_POINTER_COLOR) || defined(NYAA_USE_POINTER_TYPE)
    }
    
    // 0x0000000101719fe0
    uintptr_t mtword_; // [strong ref]
}; // class NyObject

static_assert(sizeof(NyObject) == sizeof(uintptr_t), "Incorrect Object size");
    
#define DEF_HEAP_OBJECT(name) \
    DEF_HEAP_OBJECT_CASTS(name) \
    static bool EnsureIs(const NyObject *o, NyaaCore *N); \
    DISALLOW_IMPLICIT_CONSTRUCTORS(Ny##name)
    
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
    
    inline bool Equal(Object *rhs, NyaaCore *N) const;
    inline bool LessThan(Object *rhs, NyaaCore *N) const;
    inline bool LessEqual(Object *rhs, NyaaCore *N) const;
    
    Object *Add(Object *rhs, NyaaCore *N) const;
    Object *Sub(Object *rhs, NyaaCore *N) const;
    Object *Mul(Object *rhs, NyaaCore *N) const;
    Object *Div(Object *rhs, NyaaCore *N) const;
    Object *Mod(Object *rhs, NyaaCore *N) const;
    
    static bool Near(f64_t lhs, f64_t rhs) {
        return ::fabs(lhs - rhs) <= std::numeric_limits<f64_t>::epsilon();
    }
    
    uint32_t HashVal() const {
        uint64_t bits = *reinterpret_cast<const uint64_t *>(&value_);
        return static_cast<uint32_t>(bits * bits >> 16);
    }
    
    void Iterate(ObjectVisitor *visitor) {}
    
    constexpr size_t PlacedSize() const { return sizeof(*this); }
    
    DEF_HEAP_OBJECT(Float64);
private:
    
    f64_t value_;
}; // class NyFloat64


class NyInt : public NyObject {
public:
    static constexpr const int kMinRadix = 2;
    static constexpr const int kMaxRadix = 16;

    NyInt(uint32_t capactiy);
    
    DEF_VAL_GETTER(uint32_t, capacity);
    DEF_VAL_GETTER(uint32_t, offset);
    DEF_VAL_GETTER(uint32_t, header);

    bool IsZero() const;
    
    bool Equal(Object *rhs, NyaaCore *N) const;
    bool LessThan(Object *rhs, NyaaCore *N) const;
    bool LessEqual(Object *rhs, NyaaCore *N) const;
    
    NyInt *Shl(int bits, NyaaCore *N);
    NyInt *Shr(int bits, NyaaCore *N);

    Object *Add(Object *rhs, NyaaCore *N) const;
    Object *Sub(Object *rhs, NyaaCore *N) const;
    Object *Mul(Object *rhs, NyaaCore *N) const;
    Object *Div(Object *rhs, NyaaCore *N) const;
    Object *Mod(Object *rhs, NyaaCore *N) const;
    
    NyInt *Add(int64_t rhs, NyaaCore *N) const;
    NyInt *Sub(int64_t rhs, NyaaCore *N) const;
    NyInt *Mul(int64_t rhs, NyaaCore *N) const;
    NyInt *Div(int64_t rhs, NyaaCore *N) const;
    NyInt *Mod(int64_t rhs, NyaaCore *N) const;
    
    NyInt *Add(const NyInt *rhs, NyaaCore *N) const;
    NyInt *Sub(const NyInt *rhs, NyaaCore *N) const;
    NyInt *Mul(const NyInt *rhs, NyaaCore *N) const;
    NyInt *Div(const NyInt *rhs, NyaaCore *N) const { return std::get<0>(CompleteDiv(rhs, N)); }
    NyInt *Mod(const NyInt *rhs, NyaaCore *N) const { return std::get<1>(CompleteDiv(rhs, N)); }

    std::tuple<NyInt *, NyInt *> CompleteDiv(const NyInt *rhs, NyaaCore *N) const;
    
    uint32_t HashVal() const;
    
    f64_t ToF64() const;
    int64_t ToI64() const;
    
    NyString *ToString(NyaaCore *N, int radix = 10) const;
    
    NyInt *Clone(base::Arena *arena) const { return New(segments(), segments_size(), arena); }
    NyInt *Clone(ObjectFactory *factory) const { return New(segments(), segments_size(), factory); }
    
    void Fill(uint32_t s = 0) {
        int64_t i = segments_size();
        while (i-- > 0) { set_segment(i, s); }
    }

    bool Equals(const NyInt *rhs) const { return Compare(this, rhs) == 0; }
    
    static int Compare(const NyInt *lhs, const NyInt *rhs);
    
    void Iterate(ObjectVisitor *visitor) {}
    
    size_t PlacedSize() const { return RequiredSize(capacity_); }
    
    static size_t RequiredSize(uint32_t capacity) {
        return sizeof(NyInt) + sizeof(uint32_t) * (capacity <= 2 ? 0 : capacity);
    }
    
    static Object *UnboxIfNeed(NyInt *ob) {
        if (ob->segments_size() <= 2) {
            auto val = ob->ToI64();
            if (val >= NySmi::kMinValue && val <= NySmi::kMaxValue) {
                return NySmi::New(val);
            }
        }
        return ob;
    }
    
    static NyInt *Parse(const char *s, size_t n, ObjectFactory *factory);
    static NyInt *ParseOctLiteral(const char *s, size_t n, ObjectFactory *factory);
    static NyInt *ParseHexLiteral(const char *s, size_t n, ObjectFactory *factory);
    static NyInt *ParseDecLiteral(const char *s, size_t n, ObjectFactory *factory);
    static NyInt *ParseDigitals(const char *s, size_t n, int radix, ObjectFactory *factory);
    static NyInt *NewI64(int64_t val, ObjectFactory *factory);
    static NyInt *NewU64(uint64_t val, ObjectFactory *factory);
    static NyInt *New(const uint32_t *s, size_t n, ObjectFactory *factory);

    // for temp int objects
    static NyInt *NewI64(int64_t val, base::Arena *arena);
    static NyInt *NewU64(uint64_t val, base::Arena *arena);
    static NyInt *New(const uint32_t *s, size_t n, base::Arena *arena);
    static NyInt *NewUninitialized(size_t capacity, base::Arena *arena);
    
    DEF_HEAP_OBJECT(Int);
private:
    void InitI64(int64_t val) {
        bool neg = val < 0;
        if (neg) { val = -val; }
        InitP64(val, neg, val > 0xffffffff ? 2 : 1);
    }
    
    void InitP64(uint64_t val, bool neg, size_t reserved);
    
    bool negative() const { return header_ & 0x1; }
    int sign() const { return negative() ? -1 : 1; }
    size_t segments_size() const { return capacity_ - offset_; }
    size_t segments_capacity() const { return capacity_;  }
    const uint32_t *segments() const { return vals_ + offset_; }
    uint32_t *segments() { return vals_ + offset_; }
    View<uint32_t> segment_view() const { return MakeView(segments(), segments_size()); }
    MutView<uint32_t> segment_mut_view() { return MakeMutView(segments(), segments_size()); }
    
    uint32_t segment(size_t i) const {
        DCHECK_LT(i, segments_size());
        return segments()[i];
    }

    void set_segment(size_t i, uint32_t s) {
        DCHECK_LT(i, segments_size());
        segments()[i] = s;
    }
    
    void set_offset(size_t i) {
        DCHECK_LE(i, capacity_);
        offset_ = static_cast<uint32_t>(i);
    }

    void set_negative(bool neg) {
        if (neg) {
            header_ |= 0x1;
        } else {
            header_ &= ~0x1;
        }
    }
    
    static int AbsCompare(const NyInt *lhs, const NyInt *rhs);
    static void AddRaw(const NyInt *lhs, const NyInt *rhs, NyInt *rv);
    static std::tuple<NyInt *, NyInt *> DivRaw(const NyInt *lhs, const NyInt *rhs, NyaaCore *N);
    NyInt *DivMagnitude(MutView<uint32_t> divisor, NyInt *rv, NyaaCore *N) const;
    
    void Normalize();
    void Resize(size_t n);

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
    
    void Put(Object *key, Object *value, NyaaCore *N);
    Object *Get(Object *key, NyaaCore *N) const;

    void RawPut(Object *key, Object *value, NyaaCore *N);
    Object *RawGet(Object *key, NyaaCore *N) const;
    
    bool Equal(Object *rhs, NyaaCore *N);
    bool LessThan(Object *rhs, NyaaCore *N);
    bool LessEqual(Object *rhs, NyaaCore *N);

    Object *Add(Object *rhs, NyaaCore *N);
    Object *Sub(Object *rhs, NyaaCore *N);
    Object *Mul(Object *rhs, NyaaCore *N);
    Object *Div(Object *rhs, NyaaCore *N);
    Object *Mod(Object *rhs, NyaaCore *N);

    void Iterate(ObjectVisitor *visitor) {
        visitor->VisitPointer(this, reinterpret_cast<Object **>(&generic_));
    }

    constexpr size_t PlacedSize() const { return sizeof(*this); }

    friend class MapIterator;
    DEF_HEAP_OBJECT(Map);
private:
    Object *AttemptBinaryMetaFunction(Object *rhs, NyString *name, NyaaCore *N);
    
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
    Object *RawGet(Object *key, NyaaCore *N) const;
    
    NyTable *Rehash(NyTable *origin, NyaaCore *N);
    
    void Iterate(ObjectVisitor *visitor);
    
    size_t PlacedSize() const { return RequiredSize(capacity_); }

    static size_t RequiredSize(uint32_t capacity) {
        return sizeof(NyTable) + sizeof(Entry) * capacity;
    }

    friend class MapIterator;
    DEF_HEAP_OBJECT(Table);
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
    
    const Entry *GetSlot(Object *key, NyaaCore *N) const { return At(HashVal(key, N) % n_slots() + 1); }
    
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
    using Template = arch::ObjectTemplate<NyArrayBase<T>, int32_t>;
    
    static const int32_t kOffsetSize;
    static const int32_t kOffsetCapacity;
    static const int32_t kOffsetElems;
    
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
    
template<class T>
const int32_t NyArrayBase<T>::kOffsetSize = Template::OffsetOf(&NyArrayBase<T>::size_);
template<class T>
const int32_t NyArrayBase<T>::kOffsetCapacity = Template::OffsetOf(&NyArrayBase<T>::capacity_);
template<class T>
const int32_t NyArrayBase<T>::kOffsetElems = Template::OffsetOf(&NyArrayBase<T>::elems_);


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
}; // class NyArray

    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Function and Codes
////////////////////////////////////////////////////////////////////////////////////////////////////
class NyRunnable : public NyObject {
public:
    int Run(Object *argv[], int argc, int nrets, NyaaCore *N);
    
    DEF_HEAP_OBJECT_CASTS(Runnable);
    DISALLOW_IMPLICIT_CONSTRUCTORS(NyRunnable);
protected:
    NyRunnable() {}
}; // class NyCallable


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
    
    Object *Get(Object *key, NyaaCore *N);
    void Put(Object *key, Object *value, NyaaCore *N);

    Object *RawGet(Object *key, NyaaCore *N);
    void RawPut(Object *key, Object *value, NyaaCore *N);

    Object *GetField(size_t i, NyaaCore *N);
    void SetField(size_t i, Object *value, NyaaCore *N);
    
    bool Equal(Object *rhs, NyaaCore *N);
    bool LessThan(Object *rhs, NyaaCore *N);
    bool LessEqual(Object *rhs, NyaaCore *N);
    
    Object *Add(Object *rhs, NyaaCore *N);
    Object *Sub(Object *rhs, NyaaCore *N);
    Object *Mul(Object *rhs, NyaaCore *N);
    Object *Div(Object *rhs, NyaaCore *N);
    Object *Mod(Object *rhs, NyaaCore *N);
    
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
private:
    static uintptr_t MakeTag(size_t n_fields, bool ignore_managed) {
        return static_cast<uintptr_t>(n_fields << 1 | (ignore_managed ? 0x1 : 0));
    }
    
    Object *AttemptBinaryMetaFunction(Object *rhs, NyString *name, NyaaCore *N);

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
        return GetType() == kType##type; \
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
    
inline BuiltinType Object::GetType() const {
    if (IsNil()) {
        return kTypeNil;
    }
    if (IsSmi()) {
        return kTypeSmi;
    }
    return ToHeapObject()->GetType();
}
    
#if defined(NYAA_USE_POINTER_TYPE)
inline BuiltinType NyObject::GetType() const {
    //printf("hit!\n");
    return static_cast<BuiltinType>((mtword_ & kTypeMask) >> kTypeBitsOrder);
}

inline void NyObject::SetType(BuiltinType type) {
    DCHECK_NE(kTypeSmi, type);
    DCHECK_NE(kTypeNil, type);
    DCHECK_EQ(0, GetType()) << "Type can set once!";
    //printf("hit %d!\n", type);
    mtword_ &= ~kTypeMask;
    mtword_ |= ((static_cast<uintptr_t>(type) & 0xffull) << kTypeBitsOrder);
}

#else // defined(NYAA_USE_POINTER_TYPE)
inline BuiltinType NyObject::GetType() const {
    uint64_t kid = GetMetatable()->kid();
    return static_cast<BuiltinType>(kid > kUdoKidBegin ? kTypeUdo : kid);
}

inline void NyObject::SetType(BuiltinType type) {}
#endif // defined(NYAA_USE_POINTER_TYPE)
    
inline bool NyObject::IsUDO() const {
    return IsThread() || GetType() == kTypeUdo;
}
    
inline const NyUDO *NyObject::ToUDO() const {
    return !IsUDO() ? nullptr : static_cast<const NyUDO *>(this);
}
    
inline Object *NyObject::GetMetaFunction(NyString *name, NyaaCore *N) const{
    return GetMetatable()->RawGet(name, N);
}

inline bool NyFloat64::Equal(Object *rhs, NyaaCore *N) const {
    switch (rhs->GetType()) {
        case kTypeSmi:
            return NyFloat64::Near(value_, rhs->ToSmi());
        case kTypeInt:
            return NyFloat64::Near(value_, NyInt::Cast(rhs)->ToF64());
        case kTypeFloat64:
            return NyFloat64::Near(value_, NyFloat64::Cast(rhs)->value());
        default:
            break;
    }
    return false;
}

inline bool NyFloat64::LessThan(Object *rhs, NyaaCore *N) const {
    switch (rhs->GetType()) {
        case kTypeSmi:
            return value_ < rhs->ToSmi();
        case kTypeInt:
            return value_ < NyInt::Cast(rhs)->ToF64();
        case kTypeFloat64:
            return value_ < NyFloat64::Cast(rhs)->value();
        default:
            break;
    }
    return false;
}

inline bool NyFloat64::LessEqual(Object *rhs, NyaaCore *N) const {
    switch (rhs->GetType()) {
        case kTypeSmi:
            return value_ <= rhs->ToSmi();
        case kTypeInt:
            return value_ <= NyInt::Cast(rhs)->ToF64();
        case kTypeFloat64:
            return value_ <= NyFloat64::Cast(rhs)->value();
        default:
            break;
    }
    return false;
}

inline NyRunnable *NyObject::GetValidMetaFunction(NyString *name, NyaaCore *N) const {
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
