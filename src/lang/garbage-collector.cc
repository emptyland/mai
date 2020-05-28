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

GarbageCollector::State GarbageCollector::ShouldCollect() {
    float rate = 1.0 - isolate_->heap()->GetNewSpaceUsedRate();
    State kind = kIdle;
    if (rate < minor_gc_threshold_rate_) {
        kind = kMinorCollect;
    }
    rate = 1.0 - static_cast<float>(isolate_->heap()->GetOldSpaceUsedSize()) /
                 static_cast<float>(isolate_->old_space_limit_size());
    if (rate < major_gc_threshold_rate_) {
        if (kind == kMinorCollect) {
            kind = kFullCollect;
        } else {
            kind = kMajorCollect;
        }
    }
    if (latest_remember_set_size_ > 10240) {
        kind = kFullCollect;
    }
    switch (isolate_->gc_option()) {
        case kDebugFullGCEveryTime:
            kind = kFullCollect;
            break;
        case kDebugMinorGCEveryTime:
            kind = kMinorCollect;
            break;
        default:
            break;
    }
    return kind;
}

void GarbageCollector::CollectIfNeeded() {
    if (isolate_->scheduler()->shutting_down()) {
        return;
    }
    Machine *self = Machine::This();

    State kind = ShouldCollect();
    if (kind == kIdle) { // TODO: available is too small
        return;
    }
    if (!AcquireState(GarbageCollector::kIdle, GarbageCollector::kReady) ||
        !isolate_->scheduler()->Pause()) {
        self->Park(); // Waitting
        return;
    }
    tick_.fetch_add(1, std::memory_order_release);
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
    ok = isolate_->scheduler()->Resume();
    DCHECK(ok);
    (void)ok;
}

void GarbageCollector::MinorCollect() {
    tick_.fetch_add(1, std::memory_order_release);
    set_state(kMinorCollect);
    Scavenger scavenger(isolate_, isolate_->heap());
#if defined(DEBUG) || defined(_DEBUG)
    base::StdFilePrinter printer(stdout);
#else // !defined(DEBUG) && !defined(_DEBUG)
    base::NullPrinter printer;
#endif // defined(DEBUG) || defined(_DEBUG)
    scavenger.Run(&printer);
    set_state(kDone);
}

void GarbageCollector::MajorCollect() {
    tick_.fetch_add(1, std::memory_order_release);
    set_state(kMajorCollect);
    MarkingSweep marking_sweep(isolate_, isolate_->heap());
    #if defined(DEBUG) || defined(_DEBUG)
        base::StdFilePrinter printer(stdout);
    #else // !defined(DEBUG) && !defined(_DEBUG)
        base::NullPrinter printer;
    #endif // defined(DEBUG) || defined(_DEBUG)
    marking_sweep.set_full(false);
    marking_sweep.Run(&printer);
    set_state(kDone);
}

void GarbageCollector::FullCollect() {
    tick_.fetch_add(1, std::memory_order_release);
    set_state(kFullCollect);
    
    #if defined(DEBUG) || defined(_DEBUG)
        base::StdFilePrinter printer(stdout);
    #else // !defined(DEBUG) && !defined(_DEBUG)
        base::NullPrinter printer;
    #endif // defined(DEBUG) || defined(_DEBUG)
    
    Scavenger scavenger(isolate_, isolate_->heap());
    scavenger.Run(&printer);
    
    MarkingSweep marking_sweep(isolate_, isolate_->heap());
    marking_sweep.set_full(true);
    marking_sweep.Run(&printer);

    set_state(kDone);
}

RememberMap GarbageCollector::MergeRememberSet() {
    RememberMap remember_set;
    for (int i = 0; i < isolate_->scheduler()->concurrency(); i++) {
        Machine *m = isolate_->scheduler()->machine(i);
        for(const auto &pair : m->remember_set()) {
            auto iter = remember_set.find(pair.first);
            if (iter == remember_set.end() ||
                pair.second.seuqnce_number > iter->second.seuqnce_number) {
                remember_set[pair.first] = pair.second;
            }
        }
    }
    latest_remember_set_size_ = remember_set.size();
    return remember_set;
}

void GarbageCollector::PurgeRememberSet(const std::set<void *> &keys) {
    for (void *key : keys) {
        for (int i = 0; i < isolate_->scheduler()->concurrency(); i++) {
            Machine *m = isolate_->scheduler()->machine(i);
            m->mutable_remember_set()->erase(key);
        }
    }
}

void GarbageCollector::PurgeRememberSet() {
    for (int i = 0; i < isolate_->scheduler()->concurrency(); i++) {
        Machine *m = isolate_->scheduler()->machine(i);
        m->PurgeRememberSet();
    }
}

void GarbageCollector::InvalidateHeapGuards(Address guard0, Address guard1) {
    for (int i = 0; i < isolate_->scheduler()->concurrency(); i++) {
        Machine *m = isolate_->scheduler()->machine(i);
        //DCHECK_EQ(Machine::kStop, m->state()); // Must be stop
        m->InvalidateHeapGuards(guard0, guard1);
    }
}


RememberSet::RememberSet(Allocator *lla, size_t n_buckets)
    : arena_(lla) {
    buckets_shift_ = 1;
    for (;(1u << buckets_shift_) < n_buckets; buckets_shift_++) {}
    buckets_ = static_cast<RememberNode **>(arena_.Allocate(sizeof(RememberNode*) * buckets_size()));
    for (size_t i = 0; i < buckets_size(); i++) {
        buckets_[i] = NewBucket(kMaxHeight);
    }
}

void RememberSet::Append(Any *host, Any **address, Tag tag) {
    RememberNode *prev[kMaxHeight];
    RememberNode *bucket = buckets_[HashCode(address)];
    RememberRecord key{ record_sequance_.fetch_add(1), host, address };
    RememberNode *x = FindGreaterOrEqual(key, prev, bucket);
    
    // Our data structure does not allow duplicate insertion
    DCHECK(x == NULL || !Equal(key, x->record));

    int height = RandomHeight();
    if (height > max_height()) {
        for (int i = max_height(); i < height; i++) {
            prev[i] = bucket;
        }
        max_height_.store(height, std::memory_order_relaxed);
    }

    x = NewNode(key, tag, height);
    for (int i = 0; i < height; i++) {
        x->nobarrier_set_next(i, prev[i]->nobarrier_next(i));
        prev[i]->barrier_set_next(i, x);
    }
    size_.fetch_add(1);
}

int RememberSet::Compare(const RememberRecord &lhs, const RememberRecord &rhs) const {
    int rv = 0;
    if (lhs.address < rhs.address) {
        rv = -1;
    } else if (lhs.address > rhs.address) {
        rv = 1;
    }
    if (rv != 0) {
        return rv;
    }
    if (lhs.seuqnce_number < rhs.seuqnce_number) {
        return 1;
    } else if (lhs.seuqnce_number > rhs.seuqnce_number) {
        return -1;
    }
    return 0;
}

RememberSet::RememberNode *RememberSet::NewNode(const RememberRecord &key, Tag tag, int height) {
    size_t request_size = sizeof(RememberNode) + sizeof(std::atomic<RememberNode *>) * (height - 1);
    RememberNode *node = static_cast<RememberNode *>(arena_.Allocate(request_size));
    node->record = key;
    node->tag = tag;
    return node;
}

RememberSet::RememberNode *RememberSet::NewBucket(int height) {
    size_t request_size = sizeof(RememberNode) + sizeof(std::atomic<RememberNode *>) * (height - 1);
    RememberNode *node = static_cast<RememberNode *>(arena_.Allocate(request_size));
    ::memset(&node->record, 0, sizeof(node->record));
    node->tag = kBucket;
    for (int i = 0; i < height; i++) {
        node->barrier_set_next(i, nullptr);
    }
    return node;
}

RememberSet::RememberNode *
RememberSet::FindGreaterOrEqual(const RememberRecord &key, RememberNode** prev,
                                RememberNode *head) const {
    auto x = head;
    int level = max_height() - 1;
    while (true) {
        RememberNode* next = x->barrier_next(level);
        if (KeyIsAfterNode(key, next)) {
            // Keep searching in this list
            x = next;
        } else {
            if (prev != NULL) {
                prev[level] = x;
            }
            if (level == 0) {
                return next;
            } else {
                // Switch to next list
                level--;
            }
        }
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

            DCHECK_NE(kFreeZag, *bit_cast<uint32_t *>(obj)) << "Double free!";
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

            DCHECK_NE(kFreeZag, *bit_cast<uint32_t *>(obj)) << "Double free!";
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
        DCHECK_NE(kFreeZag, *bit_cast<uint32_t *>(obj)) << "Double free!";
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
    ObjectVisitorImpl object_visitor(this);
    return UnbreakableMark(&root_visitor, &object_visitor);
}

int PartialMarkingPolicy::UnbreakableMark(RootVisitor *root_visitor, ObjectVisitor *object_visitor) {
    isolate_->VisitRoot(root_visitor);

    int count = 0;
    while (!gray_.empty()) {
        Any *obj = gray_.top();
        gray_.pop();

        if (obj->color() == KColorGray) {
            obj->set_color(heap_->finalize_color());
            IterateObject(obj, object_visitor);
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
        std::vector<std::tuple<Any *, size_t>> purgred;
        
        LargeSpace::Iterator iter(large_space);
        for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
            Any *obj = iter.object();
            if (obj->color() == heap_->initialize_color()) {
                purgred.push_back({obj, iter.object_size()});
            }
        }
        
        for (auto [obj, size] : purgred) {
            histogram_.collected_bytes += size;
            histogram_.collected_objs++;
            large_space->Free(reinterpret_cast<Address>(obj));
            count++;
        }
    }
    return count;
}

int PartialMarkingPolicy::PurgeWeakObjects() {
    int count = 0;
    WeakVisitorImpl weak_visitor(this);
    RememberMap rset = isolate_->gc()->MergeRememberSet();

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
