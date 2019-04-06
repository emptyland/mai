#ifndef MAI_NYAA_OBJECT_FACTORY_H_
#define MAI_NYAA_OBJECT_FACTORY_H_

#include "nyaa/builtin.h"
#include "base/base.h"
#include "mai-lang/handles.h"
#include <string_view>
#include <stdarg.h>

namespace mai {
    
namespace nyaa {
    
class NyObject;
class NyFloat64;
class NyLong;
class NyString;
class NyScript;
class NyMap;
class NyTable;
class NyArray;
class NyByteArray;
class NyInt32Array;
class NyDelegated;
class NyFunction;
class NyUDO;
class NyThread;
    
class Value;
class String;
class Arguments;
    
class ObjectFactory {
public:
    ObjectFactory() {}
    virtual ~ObjectFactory() {}
    
    virtual NyFloat64 *NewFloat64(f64_t value, bool old = false) = 0;
    
    virtual NyString *NewString(const char *s, size_t n, bool old = false) = 0;
    
    virtual NyString *NewUninitializedString(size_t capacity, bool old = false) = 0;
    
    virtual NyMap *NewMap(NyObject *maybe, uint64_t kid, bool linear, bool old = false) = 0;
    
    virtual NyTable *NewTable(size_t capacity, uint32_t seed, NyTable *base = nullptr,
                              bool old = false) = 0;
    
    virtual NyByteArray *NewByteArray(size_t capacity, NyByteArray *base = nullptr,
                                      bool old = false) = 0;
    
    virtual NyInt32Array *NewInt32Array(size_t capacity, NyInt32Array *base = nullptr,
                                      bool old = false) = 0;
    
    virtual NyArray *NewArray(size_t capacity, NyArray *base = nullptr, bool old = false) = 0;
    
    virtual NyDelegated *NewDelegated(DelegatedKind kind, Address fp, size_t n_upvals = 0,
                                      bool old = false) = 0;
    
    virtual NyFunction *NewFunction(size_t n_params, bool vargs, NyScript *script,
                                    size_t n_upvals = 0, bool old = false) = 0;
    
    virtual NyScript *NewScript(size_t max_stack_size, NyString *file_name, NyInt32Array *file_info,
                                NyByteArray *bcbuf, NyArray *const_pool, bool old = false) = 0;
    
    virtual NyThread *NewThread(bool old = false) = 0;
    
    virtual NyUDO *NewUninitializedUDO(size_t size, NyMap *clazz, bool ignore_managed,
                                       bool old = false) = 0;
    
    NyString *NewString(const char *s, bool old = false) {
        return NewString(s, !s ? 0 : ::strlen(s), old);
    }
    
    __attribute__ (( __format__ (__printf__, 2, 3)))
    NyString *Sprintf(const char *fmt, ...);
    
    NyString *Vsprintf(const char *fmt, va_list ap);
    
    NyMap *NewMap(uint32_t capacity, uint32_t seed, uint64_t kid = 0, bool linear = false,
                  bool old = false);
    
    NyDelegated *NewDelegated(Handle<Value> (*fp)(Local<Value>, Local<String>, Nyaa *),
                              size_t n_upvals = 0, bool old = false) {
        return NewDelegated(kPropertyGetter, reinterpret_cast<Address>(fp), n_upvals, old);
    }
    
    NyDelegated *NewDelegated(Handle<Value> (*fp)(Local<Value>, Local<String>, Local<Value>, Nyaa *),
                              size_t n_upvals = 0, bool old = false) {
        return NewDelegated(kPropertySetter, reinterpret_cast<Address>(fp), n_upvals, old);
    }
    
    NyDelegated *NewDelegated(int (*fp)(Nyaa *), size_t n_upvals = 0, bool old = false) {
        return NewDelegated(kArg0, reinterpret_cast<Address>(fp), n_upvals, old);
    }
    
    NyDelegated *NewDelegated(int (*fp)(Local<Value>, Nyaa *), size_t n_upvals = 0,
                              bool old = false) {
        return NewDelegated(kArg1, reinterpret_cast<Address>(fp), n_upvals, old);
    }
    
    NyDelegated *NewDelegated(int (*fp)(Local<Value>, Local<Value>, Nyaa *),
                              size_t n_upvals = 0, bool old = false) {
        return NewDelegated(kArg2, reinterpret_cast<Address>(fp), n_upvals, old);
    }
    
    NyDelegated *NewDelegated(int (*fp)(Local<Value>, Local<Value>, Local<Value>, Nyaa *),
                              size_t n_upvals = 0, bool old = false) {
        return NewDelegated(kArg3, reinterpret_cast<Address>(fp), n_upvals, old);
    }
    
    NyDelegated *NewDelegated(int (*fp)(Arguments *, Nyaa *), size_t n_upvals = 0,
                              bool old = false) {
        return NewDelegated(kUniversal, reinterpret_cast<Address>(fp), n_upvals, old);
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ObjectFactory);
}; // class ObjectFactory
    
} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_OBJECT_FACTORY_H_
