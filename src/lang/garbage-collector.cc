#include "lang/garbage-collector.h"
#include "lang/scavenger.h"
#include "lang/isolate-inl.h"
#include "lang/heap.h"
#include "lang/machine.h"
#include "lang/scheduler.h"
#include "base/slice.h"

namespace mai {

namespace lang {

void GarbageCollector::MinorCollect() {
    set_state(kMinorCollect);
    Scavenger scavenger(isolate_, isolate_->heap());
    base::StdFilePrinter printer(stdout);
    scavenger.Run(&printer);
    set_state(kDone);
}

RememberSet GarbageCollector::MergeRememberSet(bool keep_after) {
    RememberSet rset;
    for (int i = 0; i < isolate_->scheduler()->concurrency(); i++) {
        Machine *m = isolate_->scheduler()->machine(i);
        //DCHECK_EQ(Machine::kStop, m->state()); // Must be stop
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
    for (int i = 0; i < isolate_->scheduler()->concurrency(); i++) {
        Machine *m = isolate_->scheduler()->machine(i);
        //DCHECK_EQ(Machine::kStop, m->state()); // Must be stop
        m->InvalidateHeapGuards(guard0, guard1);
    }
}

void GarbageCollector::SafepointEnter() {
    
}

void GarbageCollector::SafepointExit() {
    
}

} // namespace lang

} // namespace mai
