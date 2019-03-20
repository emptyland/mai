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

/*virtual*/ NyString *ObjectFactoryImpl::NewString(const char *s, size_t n, bool old) {
    void *chunk = ::malloc(NyString::RequiredSize(static_cast<uint32_t>(n)));
    auto ob = new (chunk) NyString(s, n);
    ob->SetMetatable(core_->kmt_pool()->kString, core_);
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
    
/*virtual*/ NyDelegated *ObjectFactoryImpl::NewDelegated(DelegatedKind kind, Address fp, bool old) {
    void *chunk = ::malloc(sizeof(NyDelegated));
    auto ob = new (chunk) NyDelegated(kind, fp);
    ob->SetMetatable(core_->kmt_pool()->kDelegated, core_);
    return ob;
}
    
/*virtual*/ NyThread *ObjectFactoryImpl::NewThread(bool old) {
    auto ob = new NyThread(core_);
    ob->SetMetatable(core_->kmt_pool()->kThread, core_);
    return ob;
}
    
} // namespace nyaa
    
} // namespace mai
