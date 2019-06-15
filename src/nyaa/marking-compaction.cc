#include "nyaa/marking-compaction.h"
#include "nyaa/string-pool.h"
#include "nyaa/heap.h"
#include "nyaa/spaces.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/visitors.h"
#include "nyaa/nyaa-core.h"
#include "mai-lang/nyaa.h"
#include "glog/logging.h"

namespace mai {
    
namespace nyaa {
    
MarkingCompaction::MarkingCompaction(NyaaCore *core, Heap *heap)
    : GarbageCollectionPolicy(core, heap) {
}

/*virtual*/ MarkingCompaction::~MarkingCompaction() {}

/*virtual*/ void MarkingCompaction::Run() {
    // TODO:
}
    
} // namespace nyaa
    
} // namespace mai
