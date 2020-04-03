#include "lang/marking-sweep.h"
#include "lang/isolate-inl.h"
#include "lang/heap.h"
#include "lang/object-visitor.h"
#include "base/slice.h"

namespace mai {

namespace lang {

void MarkingSweep::Run(base::AbstractPrinter *logger) /*override*/ {
    Env *env = isolate_->env();
    uint64_t jiffy = env->CurrentTimeMicros();

    // Marking phase:
    logger->Println("[Major] Marking phase start, full gc: %d", full_);
    int count = UnbreakableMark();
    logger->Println("[Major] Marked %d gray objects", count);

    // Sweeping phase:
    count = 0;
    logger->Println("[Major] Sweeping phase start");
    if (OldSpace *old_space = heap_->old_space()) {
        OldSpace::Iterator iter(old_space);
        for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
            Any *obj = iter.object();
            if (obj->color() == heap_->initialize_color()) {
                histogram_.collected_bytes += iter.object_size();
                histogram_.collected_objs++;
                old_space->Free(iter.address(), true/*should_merge*/);
                count++;
            }
        }
        // TODO: merge chunks
    }
    logger->Println("[Major] Collected %d old objects", count);

    // Sweep large objects
    count = SweepLargeSpace();
    logger->Println("[Major] Collected %d large objects", count);
    // Sweep new objects
    if (full_) {
        count = UnbreakableSweepNewSpace();
        logger->Println("[Major] Collected %d new objects", count);
    }
    // Weak references sweeping:
    count = PurgeWeakObjects();
    logger->Println("[Major] Purge %d weak objects", count);

    heap_->SwapColors();
    histogram_.micro_time_cost = env->CurrentTimeMicros() - jiffy;
    logger->Println("[Major] Marking-sweep done, collected: %zd, cost: %" PRIi64 ,
                    histogram_.collected_bytes, histogram_.micro_time_cost);
}

} // namespace lang

} // namespace mai
