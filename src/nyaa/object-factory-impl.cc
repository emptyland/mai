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
    ob->SetColor(heap_->initial_color()); \
    ob->SetType(kType##type)
    
/*virtual*/ NyFloat64 *ObjectFactoryImpl::NewFloat64(f64_t value, bool old) {
    NEW_OBJECT(sizeof(NyFloat64), NyFloat64(value), Float64);
    DCHECK_EQ(sizeof(NyFloat64), ob->PlacedSize());
    return ob;
}
    
/*virtual*/ NyInt *ObjectFactoryImpl::NewUninitializedInt(size_t capacity, bool old) {
    size_t required_size = NyInt::RequiredSize(static_cast<uint32_t>(capacity));
    NEW_OBJECT(required_size, NyInt(static_cast<uint32_t>(capacity)), Int);
    DCHECK_EQ(required_size, ob->PlacedSize());
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

/*virtual*/ NyFunction *
ObjectFactoryImpl::NewFunction(NyString *name, size_t n_params, bool vargs, size_t n_upvals,
                               size_t max_stack, NyString *file_name, NyInt32Array *file_info,
                               NyObject *exec, NyArray *proto_pool, NyArray *const_pool,
                               bool old) {
    DCHECK_LE(n_params, UINT8_MAX);
    DCHECK_LE(n_upvals, UINT32_MAX);
    DCHECK_LE(max_stack, UINT32_MAX);
    size_t required_size = NyFunction::RequiredSize(static_cast<uint32_t>(n_upvals));
    NEW_OBJECT(required_size,
               NyFunction(name,
                          static_cast<uint8_t>(n_params),
                          vargs,
                          static_cast<uint32_t>(n_upvals),
                          static_cast<uint32_t>(max_stack),
                          file_name,
                          file_info,
                          exec,
                          proto_pool,
                          const_pool,
                          core_),
               Function);
    DCHECK_EQ(ob->PlacedSize(), required_size);
    return ob;
}

/*virtual*/ NyCode *ObjectFactoryImpl::NewCode(int kind, const uint8_t *instructions,
                                               size_t instructions_bytes_size) {
    DCHECK_LE(instructions_bytes_size, UINT32_MAX);
    size_t required_size = NyCode::RequiredSize(static_cast<uint32_t>(instructions_bytes_size));
    void *blk = heap_->Allocate(required_size, kCodeSpace);
    if (!blk) {
        return nullptr;
    }
    NyCode *ob = new (blk) NyCode(static_cast<NyCode::Kind>(kind),
                                  instructions,
                                  static_cast<uint32_t>(instructions_bytes_size));
    ob->SetMetatable(core_->kmt_pool()->kCode, core_);
    ob->SetColor(heap_->initial_color());
    ob->SetType(kTypeCode);
    DCHECK_EQ(ob->PlacedSize(), required_size);
    return ob;
}
    
/*virtual*/ NyClosure *ObjectFactoryImpl::NewClosure(NyFunction *proto, bool old) {
    size_t required_size = NyClosure::RequiredSize(static_cast<uint32_t>(proto->n_upvals()));
    NEW_OBJECT(required_size, NyClosure(proto, core_), Closure);
    DCHECK_EQ(required_size, ob->PlacedSize());
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
    size = RoundUp(size, kPointerSize);
    size_t n_fields = NyUDO::GetNFiedls(size);
    void *blk = heap_->Allocate(size, old ? kOldSpace : kNewSpace);
    NyUDO *ob = new (blk) NyUDO(n_fields, ignore_managed);
    ob->SetMetatable(clazz, core_);
    ob->SetColor(heap_->initial_color());
    ob->SetType(kTypeUdo);
    DCHECK_EQ(size, ob->PlacedSize());
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
