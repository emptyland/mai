#ifndef MAI_NYAA_OBJECT_FACTORY_IMPL_H_
#define MAI_NYAA_OBJECT_FACTORY_IMPL_H_

#include "nyaa/object-factory.h"

namespace mai {

namespace nyaa {
    
class Heap;
class NyaaCore;
    
class ObjectFactoryImpl : public ObjectFactory {
public:
    ObjectFactoryImpl(NyaaCore *core, Heap *heap);
    virtual ~ObjectFactoryImpl() override;
    
    virtual NyString *NewString(const char *s, size_t n, bool old) override;
    virtual NyTable *NewTable(uint32_t capacity, uint32_t seed, NyTable *base, bool old) override;
    virtual NyByteArray *NewByteArray(uint32_t capacity, NyByteArray *base, bool old) override;
    virtual NyInt32Array *NewInt32Array(uint32_t capacity, NyInt32Array *base, bool old) override;
    virtual NyArray *NewArray(uint32_t capacity, NyArray *base, bool old) override;
    virtual NyDelegated *NewDelegated(DelegatedKind kind, Address fp, bool old) override;
    virtual NyThread *NewThread(bool old) override;
private:
    NyaaCore *const core_;
    Heap *const heap_;
};

} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_OBJECT_FACTORY_IMPL_H_
