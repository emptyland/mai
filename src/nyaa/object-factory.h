#ifndef MAI_NYAA_OBJECT_FACTORY_H_
#define MAI_NYAA_OBJECT_FACTORY_H_

#include "base/base.h"
#include <string_view>

namespace mai {
    
namespace nyaa {
    
class NyString;
class NyScript;
class NyTable;
class NyObjectArray;
class NyByteArray;
class NyInt32Array;
    
class ObjectFactory {
public:
    ObjectFactory() {}
    virtual ~ObjectFactory() {}
    
    virtual NyString *NewString(const char *s, size_t n, bool old) = 0;
    
    virtual NyString *NewString(const char *s, bool old = false) {
        return NewString(s, !s ? 0 : ::strlen(s), old);
    }
    
    virtual NyTable *NewTable(uint32_t capacity, uint32_t seed, NyTable *base = nullptr,
                              bool old = false) = 0;
    
    virtual NyByteArray *NewByteArray(uint32_t capacity, NyByteArray *base = nullptr,
                                      bool old = false) = 0;
    
//    virtual NyInt32Array *NewInt32Array(uint32_t capacity, const NyInt32Array *base = nullptr,
//                                        bool old = false) = 0;
//    
//    virtual NyInt32Array *NewInt32Array(uint32_t capacity, const NyInt32Array *base = nullptr,
//                                        bool old = false) = 0;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ObjectFactory);
}; // class ObjectFactory
    
} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_OBJECT_FACTORY_H_
