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
    count = SweepOldSpace(heap()->old_space(), count);
    count = SweepOldSpace(heap()->code_space(), count);
    
//    if (OldSpace *old_space = heap_->old_space()) {
//        OldSpace::Iterator iter(old_space);
//        for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
//            Any *obj = iter.object();
//            if (obj->color() == heap_->initialize_color()) {
//                histogram_.collected_bytes += iter.object_size();
//                histogram_.collected_objs++;
//                old_space->Free(iter.address(), true/*should_merge*/);
//                count++;
//            }
//        }
//        // TODO: merge chunks
//        old_space->PurgeIfNeeded();
//    }
    
    
    logger->Println("[Major] Collected %d old objects", count);

    // Sweep large objects
    count = SweepLargeSpace();
    logger->Println("[Major] Collected %d large objects", count);

    // Weak references sweeping:
    count = PurgeWeakObjects();
    logger->Println("[Major] Purge %d weak objects", count);

    heap_->SwapColors();
    histogram_.micro_time_cost = env->CurrentTimeMicros() - jiffy;
    logger->Println("[Major] Marking-sweep done, collected: %zd, cost: %" PRIi64 ,
                    histogram_.collected_bytes, histogram_.micro_time_cost);
}

int MarkingSweep::SweepOldSpace(OldSpace *old_space, int count) {
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
    old_space->PurgeIfNeeded();
    return count;
}

} // namespace lang

} // namespace mai
