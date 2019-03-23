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
    
    virtual NyTable *NewTable(uint32_t capacity, uint32_t seed, NyTable *base = nullptr,
                              bool old = false) = 0;
    
    virtual NyByteArray *NewByteArray(uint32_t capacity, NyByteArray *base = nullptr,
                                      bool old = false) = 0;
    
    virtual NyInt32Array *NewInt32Array(uint32_t capacity, NyInt32Array *base = nullptr,
                                      bool old = false) = 0;
    
    virtual NyArray *NewArray(uint32_t capacity, NyArray *base = nullptr, bool old = false) = 0;
    
    virtual NyDelegated *NewDelegated(DelegatedKind kind, Address fp, bool old = false) = 0;
    
    virtual NyFunction *NewFunction(size_t n_params, bool vargs, size_t max_stack_size,
                                    NyScript *script) = 0;
    
    virtual NyScript *NewScript(NyString *file_name, NyInt32Array *file_info, NyByteArray *bcbuf,
                                NyArray *const_pool) = 0;
    
    virtual NyThread *NewThread(bool old = false) = 0;
    
    virtual NyUDO *NewUninitializedUDO(size_t size, NyMap *clazz, bool old = false) = 0;
    
    NyString *NewString(const char *s, bool old = false) {
        return NewString(s, !s ? 0 : ::strlen(s), old);
    }
    
    __attribute__ (( __format__ (__printf__, 2, 3)))
    NyString *Sprintf(const char *fmt, ...);
    
    NyString *Vsprintf(const char *fmt, va_list ap);
    
    NyMap *NewMap(uint32_t capacity, uint32_t seed, uint64_t kid = 0, bool linear = false,
                  bool old = false);
    
    NyDelegated *NewDelegated(Handle<Value> (*fp)(Local<Value>, Local<String>, Nyaa *),
                              bool old = false) {
        return NewDelegated(kPropertyGetter, reinterpret_cast<Address>(fp), old);
    }
    
    NyDelegated *NewDelegated(Handle<Value> (*fp)(Local<Value>, Local<String>, Local<Value>, Nyaa *),
                              bool old = false) {
        return NewDelegated(kPropertySetter, reinterpret_cast<Address>(fp), old);
    }
    
    NyDelegated *NewDelegated(int (*fp)(Nyaa *), bool old = false) {
        return NewDelegated(kArg0, reinterpret_cast<Address>(fp), old);
    }
    
    NyDelegated *NewDelegated(int (*fp)(Local<Value>, Nyaa *), bool old = false) {
        return NewDelegated(kArg1, reinterpret_cast<Address>(fp), old);
    }
    
    NyDelegated *NewDelegated(int (*fp)(Local<Value>, Local<Value>, Nyaa *), bool old = false) {
        return NewDelegated(kArg2, reinterpret_cast<Address>(fp), old);
    }
    
    NyDelegated *NewDelegated(int (*fp)(Local<Value>, Local<Value>, Local<Value>, Nyaa *),
                              bool old = false) {
        return NewDelegated(kArg3, reinterpret_cast<Address>(fp), old);
    }
    
    NyDelegated *NewDelegated(int (*fp)(Arguments *, Nyaa *), bool old = false) {
        return NewDelegated(kUniversal, reinterpret_cast<Address>(fp), old);
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ObjectFactory);
}; // class ObjectFactory
    
} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_OBJECT_FACTORY_H_
