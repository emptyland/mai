#include "nyaa/object-factory-impl.h"
#include "nyaa/string-pool.h"
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
    
#define NEW_OBJECT(size, expr, type) \
    auto blk = heap_->Allocate(size, old ? kOldSpace : kNewSpace); \
    if (!blk) { return nullptr; } \
    auto ob = new (blk) expr; \
    ob->SetMetatable(core_->kmt_pool()->k##type, core_); \
    ob->SetColor(heap_->initial_color());
    
/*virtual*/ NyFloat64 *ObjectFactoryImpl::NewFloat64(f64_t value, bool old) {
    NEW_OBJECT(sizeof(NyFloat64), NyFloat64(value), Float64);
    DCHECK_EQ(sizeof(NyFloat64), ob->PlacedSize());
    return ob;
}

/*virtual*/ NyString *ObjectFactoryImpl::NewString(const char *s, size_t n, bool old) {
    if (core_->kz_pool()) {
        NyString *kz = core_->kz_pool()->GetOrNull(s, n);
        if (kz) {
            return kz;
        }
    }
    size_t required_size = NyString::RequiredSize(static_cast<uint32_t>(n));
    NEW_OBJECT(required_size, NyString(s, n, core_), String);
    DCHECK_EQ(required_size, ob->PlacedSize());
    return ob;
}
    
/*virtual*/ NyString *ObjectFactoryImpl::NewUninitializedString(size_t capacity, bool old) {
    size_t required_size = NyString::RequiredSize(static_cast<uint32_t>(capacity));
    NEW_OBJECT(required_size, NyString(capacity), String);
    DCHECK_EQ(required_size, ob->PlacedSize());
    return ob;
}
    
/*virtual*/ NyMap *ObjectFactoryImpl::NewMap(NyObject *maybe, uint64_t kid, bool linear, bool old) {
    NEW_OBJECT(sizeof(NyMap), NyMap(maybe, kid, linear, core_), Map);
    DCHECK_EQ(sizeof(NyMap), ob->PlacedSize());
    return ob;
}

/*virtual*/ NyTable *ObjectFactoryImpl::NewTable(size_t capacity, uint32_t seed, NyTable *base,
                                                 bool old) {
    DCHECK_LE(capacity, UINT32_MAX);
    size_t required_size = NyTable::RequiredSize(static_cast<uint32_t>(capacity));
    NEW_OBJECT(required_size, NyTable(seed, static_cast<uint32_t>(capacity)), Table);
    if (base) {
        DCHECK_GT(capacity, base->capacity());
        ob = ob->Rehash(base, core_);
    }
    DCHECK_EQ(ob->PlacedSize(), required_size);
    return ob;
}

/*virtual*/ NyByteArray *ObjectFactoryImpl::NewByteArray(size_t capacity, NyByteArray *base,
                                                         bool old) {
    DCHECK_LE(capacity, UINT32_MAX);
    size_t required_size = NyByteArray::RequiredSize(static_cast<uint32_t>(capacity));
    NEW_OBJECT(required_size, NyByteArray(static_cast<uint32_t>(capacity)), ByteArray);
    if (base) {
        DCHECK_GT(capacity, base->capacity());
        ob->Refill(base);
    }
    DCHECK_EQ(ob->PlacedSize(), required_size);
    return ob;
}
    
/*virtual*/ NyInt32Array *ObjectFactoryImpl::NewInt32Array(size_t capacity, NyInt32Array *base,
                                                           bool old) {
    DCHECK_LE(capacity, UINT32_MAX);
    size_t required_size = NyInt32Array::RequiredSize(static_cast<uint32_t>(capacity));
    NEW_OBJECT(required_size, NyInt32Array(static_cast<uint32_t>(capacity)), Int32Array);
    if (base) {
        DCHECK_GT(capacity, base->capacity());
        ob->Refill(base);
    }
    DCHECK_EQ(ob->PlacedSize(), required_size);
    return ob;
}
    
/*virtual*/ NyArray *ObjectFactoryImpl::NewArray(size_t capacity, NyArray *base, bool old) {
    DCHECK_LE(capacity, UINT32_MAX);
    size_t required_size = NyArray::RequiredSize(static_cast<uint32_t>(capacity));
    NEW_OBJECT(required_size, NyArray(static_cast<uint32_t>(capacity)), Array);
    if (base) {
        DCHECK_GT(capacity, base->capacity());
        ob->Refill(base, core_);
    }
    DCHECK_EQ(ob->PlacedSize(), required_size);
    return ob;
}
    
/*virtual*/ NyScript *ObjectFactoryImpl::NewScript(size_t max_stack_size, NyString *file_name,
                                                   NyInt32Array *file_info, NyByteArray *bcbuf,
                                                   NyArray *const_pool, bool old) {
    DCHECK_LE(max_stack_size, UINT32_MAX);
    NEW_OBJECT(sizeof(NyScript),
               NyScript(static_cast<uint32_t>(max_stack_size), file_name, file_info,
                        DCHECK_NOTNULL(bcbuf), const_pool, core_),
               Script);
    DCHECK_EQ(sizeof(NyScript), ob->PlacedSize());
    return ob;
}
    
/*virtual*/ NyFunction *ObjectFactoryImpl::NewFunction(size_t n_params, bool vargs, NyScript *script,
                                                       size_t n_upvals, bool old) {
    DCHECK_LE(n_params, UINT8_MAX);
    DCHECK_LE(n_upvals, UINT32_MAX);
    size_t required_size = NyFunction::RequiredSize(static_cast<uint32_t>(n_upvals));
    NEW_OBJECT(required_size,
               NyFunction(static_cast<uint8_t>(n_params), vargs, static_cast<uint32_t>(n_upvals),
                          script, core_),
               Function);
    DCHECK_EQ(ob->PlacedSize(), required_size);
    return ob;
}
    
/*virtual*/
NyDelegated *ObjectFactoryImpl::NewDelegated(DelegatedKind kind, Address fp, size_t n_upvals,
                                             bool old) {
    DCHECK_LE(n_upvals, UINT32_MAX);
    size_t required_size = NyDelegated::RequiredSize(static_cast<uint32_t>(n_upvals));
    NEW_OBJECT(required_size, NyDelegated(kind, fp, static_cast<uint32_t>(n_upvals)), Delegated);
    DCHECK_EQ(ob->PlacedSize(), required_size);
    return ob;
}
    
/*virtual*/ NyUDO *ObjectFactoryImpl::NewUninitializedUDO(size_t size, NyMap *clazz,
                                                          bool ignore_managed, bool old) {
    DCHECK_GE(size, sizeof(NyUDO));
    void *blk = heap_->Allocate(size, old ? kOldSpace : kNewSpace);
    NyUDO *ob = new (blk) NyUDO(ignore_managed);
    ob->SetMetatable(clazz, core_);
    ob->SetColor(heap_->initial_color());
    return ob;
}
    
/*virtual*/ NyThread *ObjectFactoryImpl::NewThread(bool old) {
    NEW_OBJECT(sizeof(NyThread), NyThread(core_), Thread);
    DCHECK_EQ(sizeof(NyThread), ob->PlacedSize());
    //core_->InsertThread(ob);
    return ob;
}
    
} // namespace nyaa
    
} // namespace mai
