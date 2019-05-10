#ifndef MAI_LANG_ARGUMENTS_H_
#define MAI_LANG_ARGUMENTS_H_

#include "mai-lang/handles.h"
#include <stdint.h>
#include <assert.h>

namespace mai {
 
namespace nyaa {
    
class NyaaCore;
template<class T> class ReturnValues;
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Returns
////////////////////////////////////////////////////////////////////////////////////////////////////
class ErrorsBase {
public:
    __attribute__ (( __format__ (__printf__, 2, 3)))
    void Raisef(const char *fmt, ...);

protected:
    ErrorsBase(Nyaa *N) : N_(N) {}

    void Raise(const char *z, size_t n, void *ex);
private:
    Nyaa *N_;
}; // class ErrorsBase
    
    
template<class T>
class Errors final : public ErrorsBase {
public:
    using i = ErrorsBase;
    
    Errors(Nyaa *N) : ErrorsBase(N) {}

    void Raise(const char *z, const Handle<T> ex) { i::Raise(z, !z ? 0 : ::strlen(z), *ex); }
    
    template<class S>
    void Raise(const char *z, const Handle<S> ex) {
        i::Raise(z, !z ? 0 : ::strlen(z), *Handle<T>::Cast(ex));
    }
}; // template<class T> class Errors

class ReturnValuesBase {
protected:
    ReturnValuesBase(Nyaa *N);

    void Add(bool val) { Add(static_cast<int64_t>(val)); }
    void Add(int64_t val);
    void AddF64(double val);
    void AddNil() { AddInternal(nullptr); }
    void Add(const char *z, size_t n);
    void AddInternal(Object *val);
    void AddInternalSome(int nrets, ...);
    void Set(int nrets);
private:
    Nyaa *N_;
    //int nrets_;
}; // class ReturnValuesBase
    
template<class T>
class ReturnValues final : ReturnValuesBase {
public:
    using i = ReturnValuesBase;
    
    ReturnValues(Nyaa *N) : ReturnValuesBase(N) {}

    inline ReturnValues &Add(int64_t val) { i::Add(val); return *this; }
    inline ReturnValues &Add(const char *z) { i::Add(z, !z ? 0 : ::strlen(z)); return *this; }
    inline ReturnValues &Add(const char *z, size_t n) { i::Add(z, n); return *this; }
    inline ReturnValues &AddF64(double val) { i::AddF64(val); return *this; }
    inline ReturnValues &AddNil() { i::AddNil(); return *this; }
    inline ReturnValues &Add(const Handle<T> val) {
        i::AddInternal(reinterpret_cast<Object *>(*val));
        return *this;
    }
    inline ReturnValues &Add(const Persistent<T> val) {
        i::AddInternal(reinterpret_cast<Object *>(*val));
        return *this;
    }

    template<class S>
    inline ReturnValues &Add(const Handle<S> val) {
        i::AddInternal(reinterpret_cast<Object *>(*Handle<T>::Cast(val)));
        return *this;
    }

    template<class S>
    inline ReturnValues &Add(const Persistent<S> val) {
        i::AddInternal(reinterpret_cast<Object *>(*Persistent<T>::Cast(val)));
        return *this;
    }
    
    inline void Set(int nrets) { i::Set(nrets); }
}; // template<class T> class ReturnValues

////////////////////////////////////////////////////////////////////////////////////////////////////
// FunctionCallbackInfo
////////////////////////////////////////////////////////////////////////////////////////////////////
class FunctionCallbackBase {
public:
    size_t Length() const { return length_; }
    Nyaa *VM() const { return N_; }
    NyaaCore *Core() const;
    
    __attribute__ (( __format__ (__printf__, 2, 3)))
    void Raisef(const char *fmt, ...);
protected:
    FunctionCallbackBase(void *address, size_t length, Nyaa *N);
    FunctionCallbackBase(size_t length, Nyaa *N);
    
    Object *CurrentEnv() const;

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

    FunctionCallbackInfo(T *callee, T *argv[], size_t length, Nyaa *N)
        : FunctionCallbackBase(length, N) {
        static_cast<T **>(address_)[0] = callee;
        for (size_t i = 0; i < length; ++i) {
            static_cast<T **>(address_)[1 + i] = argv[i];
        }
    }
    
    void SetCallee(Handle<T> callee) {
        static_cast<T **>(address_)[0] = *callee;
    }
    
    using FunctionCallbackBase::VM;
    using FunctionCallbackBase::Length;
    using FunctionCallbackBase::Raisef;
    using FunctionCallbackBase::CurrentEnv;
    
    Local<T> Callee() const { return Local<T>::New(static_cast<T **>(address_)[0]); }

    Local<T> Data() const { return Local<T>::New(static_cast<T **>(address_)[1]); }
    
    Local<T> Env() const { return Local<T>::New(reinterpret_cast<T *>(CurrentEnv())); }

    Local<T> operator [] (size_t i) const {
        if (i >= Length()) {
            return Local<T>();
        }
        return Local<T>::New(static_cast<T **>(address_)[1 + i]);
    }
    
    ReturnValues<T> GetReturnValues() const { return ReturnValues<T>(VM()); }
    Errors<T> GetErrors() const { return Errors<T>(VM()); }
}; // class FunctionCallbackInfo
    
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
