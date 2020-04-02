#include "lang/marking-compact.h"
#include "lang/isolate-inl.h"
#include "lang/heap.h"
#include "lang/object-visitor.h"
#include "base/slice.h"

namespace mai {

namespace lang {

void MarkingCompact::Run(base::AbstractPrinter *logger) {
    Env *env = isolate_->env();
    uint64_t jiffy = env->CurrentTimeMicros();

    // Marking phase:
    logger->Println("[Major] Marking phase start, full gc: %d", full_);
    int count = UnbreakableMark();
    logger->Println("[Major] Marked %d gray objects", count);


    // Compacting phase:
    logger->Println("[Major] Compacting phase start");
    count = 0;
    
    
    //logger->Println("[Major] Collected %d old objects", count);

    count = SweepLargeSpace();
    logger->Println("[Major] Collected %d large objects", count);

    if (full_) {
        count = UnbreakableSweepNewSpace();
        logger->Println("[Major] Collected %d new objects", count);
    }

    // Weak references sweeping:
    count = PurgeWeakObjects();
    logger->Println("[Major] Purge %d weak objects", count);

    heap_->SwapColors();
    histogram_.micro_time_cost = env->CurrentTimeMicros() - jiffy;
    logger->Println("[Major] Marking-compact done, collected: %zd, cost: %" PRIi64 ,
                    histogram_.collected_bytes,
                    histogram_.micro_time_cost);
}


} // namespace lang

} // namespace mai
