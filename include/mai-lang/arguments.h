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
        assert(i < length_); return Local<Value>::New(address_[i]);
    }
    
    Local<Value> Callee() const { return Local<Value>::Warp(callee_); }
    
    void SetCallee(Local<Value> callee);
    
    void Set(size_t i, Local<Value> arg) {
        assert(i < length_); address_[i] = *arg;
    }
    
private:
    size_t length_;
    Value **address_;
    Value **callee_ = nullptr;
}; // class Arguments
    
} // namespace nyaa
    
} // namespace mai

#endif // MAI_LANG_ARGUMENTS_H_
