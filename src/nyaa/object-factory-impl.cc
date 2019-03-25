#include "nyaa/object-factory-impl.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/thread.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/heap.h"
#include "nyaa/builtin.h"

namespace mai {
    
namespace nyaa {
    
ObjectFactoryImpl::ObjectFactoryImpl(NyaaCore *core, Heap *heap)
    : core_(DCHECK_NOTNULL(core))
    , heap_(heap) {
}

/*virtual*/ ObjectFactoryImpl::~ObjectFactoryImpl() {
}
    
/*virtual*/ NyFloat64 *ObjectFactoryImpl::NewFloat64(f64_t value, bool old) {
    auto ob = new NyFloat64(value);
    ob->SetMetatable(core_->kmt_pool()->kFloat64, core_);
    return ob;
}

/*virtual*/ NyString *ObjectFactoryImpl::NewString(const char *s, size_t n, bool old) {
    void *chunk = ::malloc(NyString::RequiredSize(static_cast<uint32_t>(n)));
    auto ob = new (chunk) NyString(s, n, core_);
    ob->SetMetatable(core_->kmt_pool()->kString, core_);
    return ob;
}
    
/*virtual*/ NyString *ObjectFactoryImpl::NewUninitializedString(size_t capacity, bool old) {
    void *chunk = ::malloc(NyString::RequiredSize(static_cast<uint32_t>(capacity)));
    auto ob = new (chunk) NyString(capacity);
    ob->SetMetatable(core_->kmt_pool()->kString, core_);
    return ob;
}
    
/*virtual*/ NyMap *ObjectFactoryImpl::NewMap(NyObject *maybe, uint64_t kid, bool linear, bool old) {
    auto ob = new NyMap(maybe, kid, linear, core_);
    ob->SetMetatable(core_->kmt_pool()->kMap, core_);
    return ob;
}

/*virtual*/ NyTable *ObjectFactoryImpl::NewTable(uint32_t capacity, uint32_t seed, NyTable *base,
                                                 bool old) {
    void *chunk = ::malloc(NyTable::RequiredSize(capacity));
    auto ob = new (chunk) NyTable(seed, capacity);
    ob->SetMetatable(core_->kmt_pool()->kTable, core_);
    if (base) {
        DCHECK_GT(capacity, base->capacity());
        ob = ob->Rehash(base, core_);
    }
    return ob;
}

/*virtual*/ NyByteArray *ObjectFactoryImpl::NewByteArray(uint32_t capacity, NyByteArray *base,
                                                         bool old) {
    void *chunk = ::malloc(NyByteArray::RequiredSize(capacity));
    auto ob = new (chunk) NyByteArray(capacity);
    ob->SetMetatable(core_->kmt_pool()->kByteArray, core_);
    if (base) {
        DCHECK_GT(capacity, base->capacity());
        ob->Refill(base);
    }
    return ob;
}
    
/*virtual*/ NyInt32Array *ObjectFactoryImpl::NewInt32Array(uint32_t capacity, NyInt32Array *base,
                                                           bool old) {
    void *chunk = ::malloc(NyInt32Array::RequiredSize(capacity));
    auto ob = new (chunk) NyInt32Array(capacity);
    ob->SetMetatable(core_->kmt_pool()->kInt32Array, core_);
    if (base) {
        DCHECK_GT(capacity, base->capacity());
        ob->Refill(base);
    }
    return ob;
}
    
/*virtual*/ NyArray *ObjectFactoryImpl::NewArray(uint32_t capacity, NyArray *base, bool old) {
    void *chunk = ::malloc(NyArray::RequiredSize(capacity));
    auto ob = new (chunk) NyArray(capacity);
    ob->SetMetatable(core_->kmt_pool()->kArray, core_);
    if (base) {
        DCHECK_GT(capacity, base->capacity());
        ob->Refill(base, core_);
    }
    return ob;
}
    
/*virtual*/ NyScript *ObjectFactoryImpl::NewScript(NyString *file_name, NyInt32Array *file_info,
                                                   NyByteArray *bcbuf, NyArray *const_pool) {
    auto ob = new NyScript(file_name, file_info, DCHECK_NOTNULL(bcbuf), const_pool, core_);
    ob->SetMetatable(core_->kmt_pool()->kScript, core_);
    return ob;
}
    
/*virtual*/ NyFunction *ObjectFactoryImpl::NewFunction(size_t n_params, bool vargs,
                                                       size_t max_stack_size, NyScript *script) {
    DCHECK_LE(n_params, UINT8_MAX);
    DCHECK_LE(max_stack_size, UINT32_MAX);
    auto ob = new NyFunction(static_cast<uint8_t>(n_params), vargs,
                             static_cast<uint32_t>(max_stack_size), script, core_);
    ob->SetMetatable(core_->kmt_pool()->kFunction, core_);
    return ob;
}
    
/*virtual*/ NyDelegated *ObjectFactoryImpl::NewDelegated(DelegatedKind kind, Address fp, bool old) {
    void *chunk = ::malloc(sizeof(NyDelegated));
    auto ob = new (chunk) NyDelegated(kind, fp);
    ob->SetMetatable(core_->kmt_pool()->kDelegated, core_);
    return ob;
}
    
/*virtual*/ NyUDO *ObjectFactoryImpl::NewUninitializedUDO(size_t size, NyMap *clazz, bool old) {
    DCHECK_GE(size, sizeof(NyUDO));
    void *chunk = ::malloc(size);
    auto ob = new (chunk) NyUDO(nullptr);
    ob->SetMetatable(clazz, core_);
    return ob;
}
    
/*virtual*/ NyThread *ObjectFactoryImpl::NewThread(bool old) {
    auto ob = new NyThread(core_);
    ob->SetMetatable(core_->kmt_pool()->kThread, core_);
    return ob;
}
    
} // namespace nyaa
    
} // namespace mai