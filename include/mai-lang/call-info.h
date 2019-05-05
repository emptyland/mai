#ifndef MAI_LANG_ARGUMENTS_H_
#define MAI_LANG_ARGUMENTS_H_

#include "mai-lang/handles.h"
#include <stdint.h>
#include <assert.h>

namespace mai {
 
namespace nyaa {
    
template<class T> class ReturnValues;
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Returns
////////////////////////////////////////////////////////////////////////////////////////////////////
int Returnf(Nyaa *N, int nrets, ...);

inline int Return() { return Returnf(nullptr, 0); }

template<class T1>
inline int Return(const Handle<T1> &ret1) {
    if (ret1.is_empty()) {
        return -1;
    }
    return Returnf(nullptr, 1, *ret1);
}

template<class T1, class T2>
inline int Return(const Handle<T1> &ret1, const Handle<T2> &ret2) {
    if (ret1.is_empty() || ret2.is_empty()) {
        return -1;
    }
    return Returnf(nullptr, 2, *ret1, *ret2);
}

template<class T1, class T2, class T3>
inline int Return(const Handle<T1> &ret1, const Handle<T2> &ret2, const Handle<T3> &ret3) {
    if (ret1.is_empty() || ret2.is_empty() || ret3.is_empty()) {
        return -1;
    }
    return Returnf(nullptr, 3, *ret1, *ret2, *ret3);
}
    
class ReturnValuesBase {
protected:
    ReturnValuesBase(Nyaa *N) : N_(N) {}

    void Add(bool val) { Add(static_cast<int64_t>(val)); }
    void Add(int64_t val);
    void Add(double val);
    void AddNil() { AddInternal(nullptr); }
    void Add(const char *z, size_t n);
    void AddInternal(Object *val);
    void AddInternalSome(int nrets, ...);

private:
    Nyaa *N_;
}; // class ReturnValuesBase
    
template<class T>
class ReturnValues final : ReturnValuesBase {
public:
    ReturnValues(Nyaa *N) : ReturnValuesBase(N) {}

    inline ReturnValues &Add(bool val) { Add(val); return *this; }
    inline ReturnValues &Add(int64_t val) { Add(val); return *this; }
    inline ReturnValues &Add(double val) { Add(val); return *this; }
    inline ReturnValues &Add(const char *z) { Add(z, !z ? 0 : ::strlen(z)); return *this; }
    inline ReturnValues &Add(const char *z, size_t n) { Add(z, n); return *this; }
    inline ReturnValues &AddNil() { AddNil(); return *this; }
    inline ReturnValues &Add(const Handle<T> val) {
        AddInternal(reinterpret_cast<Object *>(*val));
        return *this;
    }
    inline ReturnValues &Add(const Persistent<T> val) {
        AddInternal(reinterpret_cast<Object *>(*val));
        return *this;
    }

    template<class S>
    inline ReturnValues &Add(const Handle<S> val) {
        AddInternal(reinterpret_cast<Object *>(*Handle<T>::Cast(val)));
        return *this;
    }

    template<class S>
    inline ReturnValues &Add(const Persistent<S> val) {
        AddInternal(reinterpret_cast<Object *>(*Persistent<T>::Cast(val)));
        return *this;
    }
}; // template<class T> class ReturnValues

////////////////////////////////////////////////////////////////////////////////////////////////////
// FunctionCallbackInfo
////////////////////////////////////////////////////////////////////////////////////////////////////
class FunctionCallbackBase {
public:
    size_t Length() const { return length_; }
    Nyaa *VM() const { return N_; }
    
    __attribute__ (( __format__ (__printf__, 2, 3)))
    void Raisef(const char *fmt, ...);
protected:
    FunctionCallbackBase(void *address, size_t length, Nyaa *N);
    FunctionCallbackBase(size_t length, Nyaa *N);

    //void *callee_ = nullptr;
    void *address_;
private:
    size_t length_;
    Nyaa *N_;
}; // class FunctionCallbackBase
    
template<class T>
class FunctionCallbackInfo : public FunctionCallbackBase {
public:
    FunctionCallbackInfo(void *address, size_t length, Nyaa *N)
        : FunctionCallbackBase(address, length, N) {}
    FunctionCallbackInfo(size_t length, Nyaa *N)
        : FunctionCallbackBase(length, N) {}
    
    void SetCallee(Handle<T> callee) {
        static_cast<T **>(address_)[0] = *callee;
    }
    
    using FunctionCallbackBase::VM;
    using FunctionCallbackBase::Length;
    using FunctionCallbackBase::Raisef;
    
    Local<T> Callee() const { return Local<T>::New(static_cast<T **>(address_)[0]); }

    Local<T> Data() const { return Local<T>::New(static_cast<T **>(address_)[1]); }

    Local<T> operator [] (size_t i) const {
        if (i >= Length()) {
            return Local<T>();
        }
        return Local<T>::New(*static_cast<T **>(address_)[1 + i]);
    }
    
    ReturnValues<T> GetReturnValues() const { return ReturnValues<T>(VM()); }
}; // class FunctionCallbackInfo

////////////////////////////////////////////////////////////////////////////////////////////////////
// Arguments
////////////////////////////////////////////////////////////////////////////////////////////////////
class Arguments final {
public:
    Arguments(Value **address, size_t length);
    
    Arguments(size_t length);
    
    size_t Length() const { return length_; }
    
    Local<Value> operator [] (size_t i) const { return Get(i); }
    
    Local<Value> Get(size_t i) const {
        return i >= Length() ? Local<Value>() : Local<Value>::New(address_[i]);
    }
    
    Local<Value> Callee() const { return Local<Value>::Warp(callee_); }
    
    void SetCallee(Local<Value> callee);
    
    void Set(size_t i, Local<Value> arg) {
        assert(i < length_); address_[i] = *arg;
    }
    
    Value **Address() const { return address_; }
    
private:
    size_t length_;
    Value **address_;
    Value **callee_ = nullptr;
}; // class Arguments
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// TryCatch
////////////////////////////////////////////////////////////////////////////////////////////////////
__attribute__ (( __format__ (__printf__, 1, 2)))
int Raisef(const char *fmt, ...);
    
class TryCatchCore;
    
class TryCatch final {
public:
    TryCatch(Nyaa *N);
    ~TryCatch();
    
    bool HasCaught() const;
    
    Handle<Value> Exception() const;
    
    Handle<String> Message() const;
    
    Handle<Value> StackTrace() const;
    
private:
    Nyaa *const N_;
    std::unique_ptr<TryCatchCore> catch_point_;
}; // class TryCatch
} // namespace nyaa
    
} // namespace mai

#endif // MAI_LANG_ARGUMENTS_H_
