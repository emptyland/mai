#pragma once
#ifndef MAI_LANG_VALUE_H_
#define MAI_LANG_VALUE_H_

#include "lang/type-defs.h"
#include "lang/handle.h"
#include <stdint.h>
#include <string.h>
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

// All heap object's base class.
class Any {
public:
    static const int32_t kOffsetKlass;
    static const int32_t kOffsetTags;
    
    static constexpr uint32_t kColorMask = 0x0ff;
    static constexpr uint32_t kClosureMask = 0xf00;
    
    // internal functions:
    inline bool is_forward() const;
    inline Class *clazz() const;
    inline void set_clazz(const Class *clazz);
    inline Any *forward() const;
    inline void set_forward(Any *addr);
    inline void set_forward_address(uint8_t *addr);
    inline uint32_t tags() const;
    inline bool QuicklyIs(uint32_t type_id) const;
    
    template<class T>
    inline T UnsafeGetField(const Field *field) const;

    template<class T>
    inline bool Is() const { return SlowlyIs(TypeTraits<T>::kType); }
    
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
    static const int32_t kOffsetCapacity;
    static const int32_t kOffsetLength;
    
    // How many elems to allocation
    uint32_t capacity() const { return capacity_; }

    // Number of elements
    uint32_t length() const { return length_; }

    friend class Machine;
protected:
    AbstractArray(const Class *clazz, uint32_t capacity, uint32_t length, uint32_t tags)
        : Any(clazz, tags)
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
    static inline Local<Array<T>> NewImmutable(const T *elems, size_t length) {
        Local<Array<T>> array(NewImmutable(length));
        for (size_t i = 0; i < length; i++) {
            array->elems_[i] = elems[i];
        }
        return array;
    }

    // Create new array with length
    static inline Local<Array<T>> NewImmutable(size_t length) {
        Local<AbstractArray> abstract(NewArray(TypeTraits<Array<T>>::kType, length));
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
        Local<Array<T>> copied(NewImmutable(length() - 1));
        ::memcpy(copied->elems_, elems_, index * sizeof(T));
        ::memcpy(copied->elems_ + index, elems_ + index + 1, (length_ - 1 - index) * sizeof(T));
        return copied;
    }
    
    // Internal functions
    inline T quickly_get(size_t i) const;
    inline void quickly_set(size_t i, T value);
    inline void quickly_set_length(size_t length);
    inline void QuicklyAppendNoResize(const T *data, size_t n);
    inline T *QuicklyAppendNoResize(size_t n);

    friend class Machine;
protected:
    Array(const Class *clazz, uint32_t capacity, uint32_t length, uint32_t tags)
        : AbstractArray(clazz, capacity, length, tags) {
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
    static inline Local<Array<T>> NewImmutable(Handle<CoreType> elems[], size_t length) {
        Local<Array<T>> array(NewImmutable(length));
        for (size_t i = 0; i < length; i++) {
            array->elems_[i] = *elems[i];
        }
        array->WriteBarrier(reinterpret_cast<Any **>(array->elems_), length);
        return array;
    }

    // Create new array with length
    static inline Local<Array<T>> NewImmutable(size_t length) {
        Local<Any> any(NewArray(TypeTraits<Array<T>>::kType, length));
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
            return Local<Array<T>>(this);
        }
        Local<Array> copied(NewImmutable(length_ - 1));
        ::memcpy(copied->elems_, elems_, index * sizeof(T));
        ::memcpy(copied->elems_ + index, elems_ + index + 1, (length_ - 1 - index) * sizeof(T));
        copied->WriteBarrier(reinterpret_cast<Any **>(copied->elems_), copied->length_);
        return copied;
    }

    // Internal functions
    inline T quickly_get(size_t i) const;
    inline void quickly_set_nobarrier(size_t i, T value);
    
    friend class Machine;
protected:
    Array(const Class *clazz, uint32_t capacity, uint32_t length, uint32_t tags)
        : AbstractArray(clazz, capacity, length, tags) {
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
    String(const Class *clazz, uint32_t capacity, const char *utf8_string, uint32_t length, uint32_t tags)
        : Array<char>(clazz, capacity, length, tags) {
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
    
    // Internal methods:
    inline bool is_cxx_function() const;
    inline bool is_mai_function() const;
    inline Code *code() const;
    inline Function *function() const;
    inline void SetCapturedVar(uint32_t i, CapturedValue *value);
    inline void set_captured_var_no_barrier(uint32_t i, CapturedValue *value);
    inline CapturedValue *captured_var(uint32_t i) const;
    
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
    
    static Throwable *NewException(String *message, Exception *cause);
    
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
    template<class T, bool Ref = ElementTraits<T>::kIsReferenceType>
    struct ParameterTraits {
        static constexpr uint32_t kType = TypeTraits<T>::kType;
    }; // struct ParameterTraits
    
    template<class T>
    struct ParameterTraits<T*, true> {
        static constexpr uint32_t kType = TypeTraits<typename std::remove_pointer<T>::type>::kType;
    }; // struct ParameterTraits
    
    template<class T>
    struct ParameterTraits<T*, false> {
        static constexpr uint32_t kType = kType_u64;
    }; // struct ParameterTraits
    
    template<class R>
    static inline Local<Closure> New(R(*func)()) {
        Code *stub = MakeStub({}/*parametres*/, false/*has_vargs*/, ParameterTraits<R>::kType,
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
                              ParameterTraits<R>::kType, reinterpret_cast<uint8_t *>(func));
        if (!stub) {
            return Local<Closure>::Empty();
        } else {
            return Closure::New(stub, 0/*captured_var_size*/);
        }
    }
    
    template<class R, class A, class B>
    static inline Local<Closure> New(R(*func)(A, B)) {
        Code *stub = MakeStub({ParameterTraits<A>::kType, ParameterTraits<B>::kType}/*parametres*/,
                              false/*has_vargs*/, ParameterTraits<R>::kType,
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
                              false/*has_vargs*/, ParameterTraits<R>::kType,
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
                              false/*has_vargs*/, ParameterTraits<R>::kType,
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
                              false/*has_vargs*/, ParameterTraits<R>::kType,
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
    //static uint32_t GetTypeId(const Any *object);
    
    static Code *MakeStub(const std::vector<uint32_t> &parameters, bool has_vargs,
                          uint32_t return_type, uint8_t *cxx_func_entry);
}; // class FunctionTemplate

} // namespace lang

} // namespace mai

#endif // MAI_LANG_VALUE_H_
