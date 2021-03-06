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
class NyString;
class NyClosure;
class NyMap;
class NyTable;
class NyArray;
class NyByteArray;
class NyBytecodeArray;
class NyInt32Array;
class NyDelegated;
class NyFunction;
class NyCode;
class NyUDO;
class NyThread;
class Heap;
class Value;
class String;
    
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
    
    virtual NyBytecodeArray *NewBytecodeArray(NyInt32Array *source_lines, Address bytecodes,
                                              size_t bytecode_bytes_size, bool old = false) = 0;
    
    virtual NyDelegated *NewDelegated(DelegatedKind kind, Address fp, size_t n_upvals = 0,
                                      bool old = false) = 0;
    
    virtual NyFunction *NewFunction(NyString *name, size_t n_params, bool vargs, size_t n_upvals,
                                    size_t max_stack, NyString *file_name, NyObject *exec,
                                    NyArray *proto_pool, NyArray *const_pool, bool old = false) = 0;

    virtual NyCode *NewCode(int kind, NyInt32Array *source_lines, const uint8_t *instructions,
                            size_t instructions_byte_size) = 0;
    
    virtual NyClosure *NewClosure(NyFunction *proto, bool old = false) = 0;
    
    virtual NyThread *NewThread(size_t initial_stack_size, bool old = false) = 0;
    
    virtual NyUDO *NewUninitializedUDO(size_t size, NyMap *clazz,
                                       bool ignore_managed, bool old = false) = 0;
    
    NyString *NewString(const char *s, bool old = false) {
        return NewString(s, !s ? 0 : ::strlen(s), old);
    }
    
    __attribute__ (( __format__ (__printf__, 2, 3)))
    NyString *Sprintf(const char *fmt, ...);
    
    NyString *Vsprintf(const char *fmt, va_list ap);
    
    NyMap *NewMap(uint32_t capacity, uint32_t seed, uint64_t kid = 0, bool linear = false,
                  bool old = false);

    NyDelegated *NewDelegated(void (*fp)(const FunctionCallbackInfo<Object> &), size_t n_upvals = 0,
                              bool old = false) {
        return NewDelegated(kFunctionCallback, reinterpret_cast<Address>(fp), n_upvals, old);
    }
    
    NyDelegated *NewDelegated(void (*fp)(const FunctionCallbackInfo<Value> &), size_t n_upvals = 0,
                              bool old = false) {
        return NewDelegated(kFunctionCallback, reinterpret_cast<Address>(fp), n_upvals, old);
    }
    
    static ObjectFactory *NewHeapFactory(NyaaCore *core, Heap *heap);
    
    static ObjectFactory *NewMallocFactory(NyaaCore *core);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ObjectFactory);
}; // class ObjectFactory
    
} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_OBJECT_FACTORY_H_
