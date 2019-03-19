#include "nyaa/heap.h"
#include "nyaa/nyaa-core.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {
    
Heap::Heap(NyaaCore *N)
    : owns_(DCHECK_NOTNULL(N)) {}

/*virtual*/ Heap::~Heap() {
    // TODO:
}

Error Heap::Prepare() {
    // TODO:
    return Error::OK();
}

std::tuple<NyObject *, Error> Heap::Allocate(size_t size, HeapArea kind) {
    // TODO:
    return {nullptr, MAI_NOT_SUPPORTED("TODO:")};
}
    
void Heap::BarrierWr(NyObject *owns, Object *arg1, Object *arg2) {
    // TODO:
}

/*virtual*/ void *Heap::Allocate(size_t size, size_t /*alignment*/) {
    NyObject *result;
    Error rs;
    std::tie(result, rs) = Allocate(size, NEW_SPACE);
    if (!rs) {
        DLOG(ERROR) << rs.ToString();
        return nullptr;
    }
    return result;
}

/*virtual*/ void Heap::Purge(bool reinit) {
    // TODO:
}

/*virtual*/ size_t Heap::memory_usage() const {
    // TODO:
    return 0;
}
    
} // namespace nyaa
    
} // namespace mai
