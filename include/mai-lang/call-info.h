#ifndef MAI_LANG_ARGUMENTS_H_
#define MAI_LANG_ARGUMENTS_H_

#include "mai-lang/handles.h"
#include <stdint.h>
#include <assert.h>

namespace mai {
 
namespace nyaa {

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
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// TryCatch
////////////////////////////////////////////////////////////////////////////////////////////////////
__attribute__ (( __format__ (__printf__, 2, 3)))
int Raisef(Nyaa *N, const char *fmt, ...);
    
class TryCatchCore;
    
class TryCatch final {
public:
    TryCatch(Isolate *isolate);
    ~TryCatch();
    
    bool HasCaught() const;
    
    Handle<Value> Exception() const;
    
    Handle<String> Message() const;
    
    Handle<Value> StackTrace() const;
    
private:
    Isolate *const isolate_;
    std::unique_ptr<TryCatchCore> catch_point_;
}; // class TryCatch
} // namespace nyaa
    
} // namespace mai

#endif // MAI_LANG_ARGUMENTS_H_
