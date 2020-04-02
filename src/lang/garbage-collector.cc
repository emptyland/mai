#include "lang/garbage-collector.h"
#include "lang/scavenger.h"
#include "lang/marking-sweep.h"
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
    set_state(kMajorCollect);
    MarkingSweep marking_sweep(isolate_, isolate_->heap());
    base::StdFilePrinter printer(stdout);
    marking_sweep.set_full(false);
    marking_sweep.Run(&printer);
    set_state(kDone);
}

void GarbageCollector::FullCollect() {
    set_state(kMajorCollect);
    MarkingSweep marking_sweep(isolate_, isolate_->heap());
    base::StdFilePrinter printer(stdout);
    marking_sweep.set_full(true);
    marking_sweep.Run(&printer);
    set_state(kDone);
}

const RememberSet &GarbageCollector::MergeRememberSet() {
    for (int i = 0; i < isolate_->scheduler()->concurrency(); i++) {
        Machine *m = isolate_->scheduler()->machine(i);
        for(const auto &pair : m->remember_set()) {
            auto iter = remember_set_.find(pair.first);
            if (iter == remember_set_.end() ||
                pair.second.seuqnce_number > iter->second.seuqnce_number) {
                remember_set_[pair.first] = pair.second;
            }
        }
        m->PurgeRememberSet();
    }
    return remember_set_;
}

void GarbageCollector::InvalidateHeapGuards(Address guard0, Address guard1) {
    for (int i = 0; i < isolate_->scheduler()->concurrency(); i++) {
        Machine *m = isolate_->scheduler()->machine(i);
        //DCHECK_EQ(Machine::kStop, m->state()); // Must be stop
        m->InvalidateHeapGuards(guard0, guard1);
    }
}


class PartialMarkingPolicy::RootVisitorImpl final : public RootVisitor {
public:
    RootVisitorImpl(PartialMarkingPolicy *owns) : owns_(owns) {}
    ~RootVisitorImpl() override = default;
    
    void VisitRootPointers(Any **begin, Any **end) override {
        for (Any **i = begin; i < end; i++) {
            Any *obj = *i;
            if (!obj) {
                continue;
            }
            
            DCHECK(obj->is_directly());
            DCHECK_NE(owns_->heap_->finalize_color(), obj->color());
            obj->set_color(KColorGray);
            owns_->gray_.push(obj);
        }
    }
private:
    PartialMarkingPolicy *const owns_;
}; // class PartialMarkingPolicy::RootVisitorImpl


class PartialMarkingPolicy::ObjectVisitorImpl final : public ObjectVisitor {
public:
    ObjectVisitorImpl(PartialMarkingPolicy *owns): owns_(owns) {}
    ~ObjectVisitorImpl() override = default;

    void VisitPointers(Any *host, Any **begin, Any **end) override {
        for (Any **i = begin; i < end; i++) {
            Any *obj = *i;
            if (!obj) {
                continue;
            }

            DCHECK(obj->is_directly());
            // Transform white to gray
            if (obj->color() == owns_->heap_->initialize_color()) {
                obj->set_color(KColorGray);
                owns_->gray_.push(obj);
            }
        }
    }
private:
    PartialMarkingPolicy *const owns_;
}; // class PartialMarkingPolicy::ObjectVisitorImpl

class PartialMarkingPolicy::WeakVisitorImpl final : public ObjectVisitor {
public:
    WeakVisitorImpl(PartialMarkingPolicy *owns): owns_(owns) {}
    ~WeakVisitorImpl() override = default;
    
    void VisitPointers(Any *host, Any **begin, Any **end) override {
        for (Any **i = begin; i < end; i++) {
            VisitPointer(host, i);
        }
    }

    void VisitPointer(Any *host, Any **p) override {
        Any *obj = *p;
        DCHECK(!owns_->full_ || obj->is_directly());
        if (Any *forward = obj->forward()) {
            DCHECK_EQ(owns_->heap_->finalize_color(), forward->color());
            *p = forward;
        } else if (obj->color() != owns_->heap_->finalize_color()) {
            DCHECK_NE(KColorGray, obj->color());
            *p = nullptr;
        }
    }

private:
    PartialMarkingPolicy *const owns_;
}; // class PartialMarkingPolicy::WeakVisitorImpl

int PartialMarkingPolicy::UnbreakableMark() {
    RootVisitorImpl root_visitor(this);
    isolate_->VisitRoot(&root_visitor);
    
    int count = 0;
    ObjectVisitorImpl object_visitor(this);
    while (!gray_.empty()) {
        Any *obj = gray_.top();
        gray_.pop();

        if (obj->color() == KColorGray) {
            obj->set_color(heap_->finalize_color());
            IterateObject(obj, &object_visitor);
        } else {
            DCHECK_EQ(obj->color(), heap_->finalize_color());
        }
        count++;
    }
    return count;
}

int PartialMarkingPolicy::UnbreakableSweepNewSpace() {
    int count = 0;
    SemiSpace::Iterator iter(heap_->new_space()->original_area());
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        Any *obj = iter.object();
        if (obj->color() == heap_->initialize_color()) {
            histogram_.collected_bytes += iter.object_size();
            histogram_.collected_objs++;
            count++;
        } else {
            // Keep the lived object
            heap_->MoveNewSpaceObject(obj, false/*promote*/);
        }
    }
    
    size_t remaining = heap_->new_space()->Flip(false/*reinit*/);
    // After new space flip, should update all coroutine's heap guards
    SemiSpace *original_area = heap_->new_space()->original_area();
    isolate_->gc()->InvalidateHeapGuards(original_area->chunk(), original_area->limit());
    isolate_->gc()->set_latest_minor_remaining_size(remaining);
    return count;
}

int PartialMarkingPolicy::SweepLargeSpace() {
    int count = 0;
    if (LargeSpace *large_space = heap_->large_space()) {
        LargeSpace::Iterator iter(large_space);
        for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
            Any *obj = iter.object();
            if (obj->color() == heap_->initialize_color()) {
                histogram_.collected_bytes += iter.object_size();
                histogram_.collected_objs++;
                large_space->Free(iter.address());
                count++;
            }
        }
    }
    return count;
}

int PartialMarkingPolicy::PurgeWeakObjects() {
    int count = 0;
    WeakVisitorImpl weak_visitor(this);
    const RememberSet &rset = isolate_->gc()->MergeRememberSet();
    std::set<void *> for_clean;
    for (auto rd : rset) {
        weak_visitor.VisitPointer(rd.second.host, &rd.second.host);
        if (!rd.second.host) { // Should sweep remember entries
            for_clean.insert(rd.first);
            count++;
        }
    }
    isolate_->gc()->PurgeRememberSet(for_clean);
    return count;
}

} // namespace lang

} // namespace mai
