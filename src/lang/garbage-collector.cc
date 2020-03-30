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

RememberSet GarbageCollector::MergeRememberSet(bool keep_after) const {
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
    return rset;
}

} // namespace lang

} // namespace mai
