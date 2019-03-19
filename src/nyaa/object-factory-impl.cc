#include "nyaa/object-factory-impl.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/heap.h"

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
    ob->SetMetatable(core_->kMtString, core_);
    return ob;
}

/*virtual*/ NyTable *ObjectFactoryImpl::NewTable(uint32_t capacity, uint32_t seed, NyTable *base,
                                                 bool old) {
    void *chunk = ::malloc(NyTable::RequiredSize(capacity));
    auto ob = new (chunk) NyTable(seed, capacity);
    ob->SetMetatable(core_->kMtTable, core_);
    if (base) {
        DCHECK_GT(capacity, base->capacity());
        ob = ob->Rehash(base, core_);
    }
    return ob;
}

/*virtual*/ NyByteArray *ObjectFactoryImpl::NewByteArray(uint32_t capacity, NyByteArray *base,
                                                     bool old) {
    core_->Raisef("TODO:");
    return nullptr;
}
    
} // namespace nyaa
    
} // namespace mai
