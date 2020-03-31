#include "lang/garbage-collector.h"
#include "lang/scavenger.h"
#include "lang/isolate-inl.h"
#include "lang/heap.h"
#include "lang/machine.h"
#include "lang/scheduler.h"
#include "base/slice.h"

namespace mai {

namespace lang {

void GarbageCollector::CollectIfNeeded() {
    Machine *self = Machine::This();

    float rate = 1.0 - isolate_->heap()->GetNewSpaceUsedRate();
    State kind = kIdle;
    if (rate < minor_gc_threshold_rate_) {
        kind = kMinorCollect;
    }
    rate = 1.0 - static_cast<float>(isolate_->heap()->GetOldSpaceUsedSize()) /
                 static_cast<float>(isolate_->old_space_limit_size());
    if (rate < major_gc_threshold_rate_ || remember_record_sequance_.load() > 10240) {
        if (kind == kMinorCollect) {
            kind = kFullCollect;
        } else {
            kind = kMajorCollect;
        }
    }

    if (kind == kIdle) { // TODO: available is too small
        return;
    }
    if (!AcquireState(GarbageCollector::kIdle, GarbageCollector::kReady)) {
        self->Park(); // Waitting
        return;
    }
    tick_.fetch_add(1, std::memory_order_release);
    isolate_->scheduler()->Pause();
    switch (kind) {
        case kMinorCollect:
            MinorCollect();
            break;
        case kMajorCollect:
            MajorCollect();
            break;
        case kFullCollect:
            FullCollect();
            break;
        default:
            NOREACHED();
            break;
    }
    bool ok = AcquireState(GarbageCollector::kDone, GarbageCollector::kIdle);
    DCHECK(ok);
    (void)ok;
    isolate_->scheduler()->Resume();
}

void GarbageCollector::MinorCollect() {
    set_state(kMinorCollect);
    Scavenger scavenger(isolate_, isolate_->heap());
    base::StdFilePrinter printer(stdout);
    scavenger.Run(&printer);
    set_state(kDone);
}

void GarbageCollector::MajorCollect() {
    // TODO:
}

void GarbageCollector::FullCollect() {
    // TODO:
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

} // namespace lang

} // namespace mai
