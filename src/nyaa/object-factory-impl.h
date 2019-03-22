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
    
    virtual NyFloat64 *NewFloat64(f64_t value, bool old) override;
    virtual NyString *NewString(const char *s, size_t n, bool old) override;
    virtual NyString *NewUninitializedString(size_t capacity, bool old) override;
    virtual NyMap *NewMap(NyObject *maybe, uint64_t kid, bool linear, bool old) override;
    virtual NyTable *NewTable(uint32_t capacity, uint32_t seed, NyTable *base, bool old) override;
    virtual NyByteArray *NewByteArray(uint32_t capacity, NyByteArray *base, bool old) override;
    virtual NyInt32Array *NewInt32Array(uint32_t capacity, NyInt32Array *base, bool old) override;
    virtual NyArray *NewArray(uint32_t capacity, NyArray *base, bool old) override;
    virtual NyDelegated *NewDelegated(DelegatedKind kind, Address fp, bool old) override;
    virtual NyFunction *NewFunction(size_t n_params, bool vargs, size_t max_stack_size,
                                    NyScript *script) override;
    virtual NyScript *NewScript(NyString *file_name, NyInt32Array *file_info, NyByteArray *bcbuf,
                                NyArray *const_pool) override;
    virtual NyThread *NewThread(bool old) override;
private:
    NyaaCore *const core_;
    Heap *const heap_;
};

} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_OBJECT_FACTORY_IMPL_H_
