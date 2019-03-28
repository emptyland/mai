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
    size_t required_size = NyString::RequiredSize(static_cast<uint32_t>(n));
    void *chunk = ::malloc(required_size);
    auto ob = new (chunk) NyString(s, n, core_);
    DCHECK_EQ(ob->PlacedSize(), required_size);
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

/*virtual*/ NyTable *ObjectFactoryImpl::NewTable(size_t capacity, uint32_t seed, NyTable *base,
                                                 bool old) {
    DCHECK_LE(capacity, UINT32_MAX);
    void *chunk = ::malloc(NyTable::RequiredSize(static_cast<uint32_t>(capacity)));
    auto ob = new (chunk) NyTable(seed, static_cast<uint32_t>(capacity));
    DCHECK_NOTNULL(core_->kmt_pool());
    ob->SetMetatable(core_->kmt_pool()->kTable, core_);
    if (base) {
        DCHECK_GT(capacity, base->capacity());
        ob = ob->Rehash(base, core_);
    }
    DCHECK_EQ(ob->PlacedSize(), NyTable::RequiredSize(static_cast<uint32_t>(capacity)));
    return ob;
}

/*virtual*/ NyByteArray *ObjectFactoryImpl::NewByteArray(size_t capacity, NyByteArray *base,
                                                         bool old) {
    DCHECK_LE(capacity, UINT32_MAX);
    void *chunk = ::malloc(NyByteArray::RequiredSize(static_cast<uint32_t>(capacity)));
    auto ob = new (chunk) NyByteArray(static_cast<uint32_t>(capacity));
    ob->SetMetatable(core_->kmt_pool()->kByteArray, core_);
    if (base) {
        DCHECK_GT(capacity, base->capacity());
        ob->Refill(base);
    }
    DCHECK_EQ(ob->PlacedSize(), NyByteArray::RequiredSize(static_cast<uint32_t>(capacity)));
    return ob;
}
    
/*virtual*/ NyInt32Array *ObjectFactoryImpl::NewInt32Array(size_t capacity, NyInt32Array *base,
                                                           bool old) {
    DCHECK_LE(capacity, UINT32_MAX);
    void *chunk = ::malloc(NyInt32Array::RequiredSize(static_cast<uint32_t>(capacity)));
    auto ob = new (chunk) NyInt32Array(static_cast<uint32_t>(capacity));
    ob->SetMetatable(core_->kmt_pool()->kInt32Array, core_);
    if (base) {
        DCHECK_GT(capacity, base->capacity());
        ob->Refill(base);
    }
    DCHECK_EQ(ob->PlacedSize(), NyInt32Array::RequiredSize(static_cast<uint32_t>(capacity)));
    return ob;
}
    
/*virtual*/ NyArray *ObjectFactoryImpl::NewArray(size_t capacity, NyArray *base, bool old) {
    DCHECK_LE(capacity, UINT32_MAX);
    void *chunk = ::malloc(NyArray::RequiredSize(static_cast<uint32_t>(capacity)));
    auto ob = new (chunk) NyArray(static_cast<uint32_t>(capacity));
    ob->SetMetatable(core_->kmt_pool()->kArray, core_);
    if (base) {
        DCHECK_GT(capacity, base->capacity());
        ob->Refill(base, core_);
    }
    DCHECK_EQ(ob->PlacedSize(), NyArray::RequiredSize(static_cast<uint32_t>(capacity)));
    return ob;
}
    
/*virtual*/ NyScript *ObjectFactoryImpl::NewScript(NyString *file_name, NyInt32Array *file_info,
                                                   NyByteArray *bcbuf, NyArray *const_pool) {
    auto ob = new NyScript(file_name, file_info, DCHECK_NOTNULL(bcbuf), const_pool, core_);
    ob->SetMetatable(core_->kmt_pool()->kScript, core_);
    return ob;
}
    
/*virtual*/ NyFunction *ObjectFactoryImpl::NewFunction(size_t n_params, bool vargs,
                                                       size_t max_stack_size, NyScript *script,
                                                       size_t n_upvals, bool old) {
    DCHECK_LE(n_params, UINT8_MAX);
    DCHECK_LE(max_stack_size, UINT32_MAX);
    DCHECK_LE(n_upvals, UINT32_MAX);
    void *chunk = ::malloc(NyFunction::RequiredSize(static_cast<uint32_t>(n_upvals)));
    auto ob = new (chunk) NyFunction(static_cast<uint8_t>(n_params), vargs,
                                     static_cast<uint32_t>(max_stack_size),
                                     static_cast<uint32_t>(n_upvals),
                                     script, core_);
    ob->SetMetatable(core_->kmt_pool()->kFunction, core_);
    DCHECK_EQ(ob->PlacedSize(), NyFunction::RequiredSize(static_cast<uint32_t>(n_upvals)));
    return ob;
}
    
/*virtual*/
NyDelegated *ObjectFactoryImpl::NewDelegated(DelegatedKind kind, Address fp, size_t n_upvals,
                                             bool old) {
    DCHECK_LE(n_upvals, UINT32_MAX);
    void *chunk = ::malloc(NyDelegated::RequiredSize(static_cast<uint32_t>(n_upvals)));
    auto ob = new (chunk) NyDelegated(kind, fp, static_cast<uint32_t>(n_upvals));
    ob->SetMetatable(core_->kmt_pool()->kDelegated, core_);
    DCHECK_EQ(ob->PlacedSize(), NyDelegated::RequiredSize(static_cast<uint32_t>(n_upvals)));
    return ob;
}
    
/*virtual*/ NyUDO *ObjectFactoryImpl::NewUninitializedUDO(size_t size, NyMap *clazz, bool old) {
    DCHECK_GE(size, sizeof(NyUDO));
    void *chunk = ::malloc(size);
    auto ob = new (chunk) NyUDO();
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
