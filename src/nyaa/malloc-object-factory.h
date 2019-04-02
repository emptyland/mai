#ifndef MAI_NYAA_MALLOC_OBJECT_FACTORY_H_
#define MAI_NYAA_MALLOC_OBJECT_FACTORY_H_

#include "nyaa/object-factory.h"

namespace mai {

namespace nyaa {
    
class NyaaCore;
    
class MallocObjectFactory : public ObjectFactory {
public:
    MallocObjectFactory(NyaaCore *core);
    virtual ~MallocObjectFactory() override;
    
    virtual NyFloat64 *NewFloat64(f64_t value, bool old) override;
    virtual NyString *NewString(const char *s, size_t n, bool old) override;
    virtual NyString *NewUninitializedString(size_t capacity, bool old) override;
    virtual NyMap *NewMap(NyObject *maybe, uint64_t kid, bool linear, bool old) override;
    virtual NyTable *NewTable(size_t capacity, uint32_t seed, NyTable *base, bool old) override;
    virtual NyByteArray *NewByteArray(size_t capacity, NyByteArray *base, bool old) override;
    virtual NyInt32Array *NewInt32Array(size_t capacity, NyInt32Array *base, bool old) override;
    virtual NyArray *NewArray(size_t capacity, NyArray *base, bool old) override;
    virtual NyDelegated *NewDelegated(DelegatedKind kind, Address fp, size_t n_upvals,
                                      bool old) override;
    virtual NyFunction *NewFunction(size_t n_params, bool vargs, NyScript *script, size_t n_upvals,
                                    bool old) override;
    virtual NyScript *NewScript(size_t max_stack_size, NyString *file_name, NyInt32Array *file_info,
                                NyByteArray *bcbuf, NyArray *const_pool, bool old) override;
    virtual NyUDO *NewUninitializedUDO(size_t size, NyMap *clazz, bool old) override;
    virtual NyThread *NewThread(bool old) override;
private:
    NyaaCore *const core_;
};

} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_MALLOC_OBJECT_FACTORY_H_