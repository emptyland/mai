#include "lang/garbage-collector.h"
#include "lang/scavenger.h"
#include "lang/isolate-inl.h"
#include "lang/heap.h"
#include "lang/machine.h"
#include "lang/scheduler.h"

namespace mai {

namespace lang {

void GarbageCollector::MinorCollect() {
    Scavenger scavenger(isolate_, isolate_->heap());
    scavenger.Run();
}

RememberSet GarbageCollector::MergeRememberSet(bool keep_after) {
    RememberSet rset;
    for (int i = 0; i < isolate_->scheduler()->concurrency(); i++) {
        Machine *m = isolate_->scheduler()->machine(i);
        for(const auto &pair : m->remember_set()) {
            auto iter = rset.find(pair.first);
            if (iter == rset.end() ||
                pair.second.seuqnce_number > iter->second.seuqnce_number) {
                rset[pair.first] = pair.second;
            }
        }
        if (!keep_after) {
            m->PurgeRememberSet();
        }
    }
    if (!keep_after) {
        remember_record_sequance_.store(0, std::memory_order_relaxed);
    }
    return rset;
}

void GarbageCollector::InvalidateHeapGuards(Address guard0, Address guard1) {
    
}

} // namespace lang

} // namespace mai
