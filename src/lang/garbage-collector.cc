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

GarbageCollector::GarbageCollector(Isolate *isolate, float minor_gc_threshold_rate,
                                   float major_gc_threshold_rate)
    : isolate_(isolate)
    , remember_set_(new RememberSet(1024))
    , minor_gc_threshold_rate_(minor_gc_threshold_rate)
    , major_gc_threshold_rate_(major_gc_threshold_rate) {}

GarbageCollector::~GarbageCollector() {}

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
    if (remember_set_->size() > 10240) {
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

void GarbageCollector::InvalidateHeapGuards(Address guard0, Address guard1) {
    for (int i = 0; i < isolate_->scheduler()->concurrency(); i++) {
        Machine *m = isolate_->scheduler()->machine(i);
        //DCHECK_EQ(Machine::kStop, m->state()); // Must be stop
        m->InvalidateHeapGuards(guard0, guard1);
    }
}


RememberSet::RememberSet(size_t n_buckets) {
    buckets_shift_ = 1;
    for (;(1u << buckets_shift_) < n_buckets; buckets_shift_++) {}
    buckets_ = static_cast<Bucket *>(arena_.Allocate(sizeof(Bucket) * buckets_size()));
    ::memset(buckets_, 0, sizeof(Bucket) * buckets_size());
}

void RememberSet::Insert(Any *host, Any **address, Tag tag) {
    Node *prev[kMaxHeight];
#if defined(DEBUG) || defined(_DEBUG)
    ::memset(prev, 0, sizeof(prev));
#endif // defined(DEBUG) || defined(_DEBUG)
    RememberRecord key{ record_sequance_.fetch_add(1), host, address };

    Bucket *bucket = &buckets_[HashCode(address)];
    base::SpinLock lock(&bucket->mutex);
    if (bucket->max_height == 0) {
        bucket->max_height = 1;
    }

    Node *x = FindGreaterOrEqual(key, prev, bucket);
    
    // Our data structure does not allow duplicate insertion
    DCHECK(x == NULL || !Equal(key, x->record));

    int height = RandomHeight();
    if (height > bucket->max_height) {
        for (int i = bucket->max_height; i < height; i++) {
            prev[i] = &bucket->head;
        }
        bucket->max_height = height;
    }

    x = NewNode(key, tag, height);
    for (int i = 0; i < height; i++) {
        x->next[i] = DCHECK_NOTNULL(prev[i])->next[i];
        prev[i]->next[i] = x;
    }
    size_.fetch_add(1);
}

void RememberSet::Purge(size_t n_buckets) {
    arena_.Purge(true);
    for (buckets_shift_ = 1; (1u << buckets_shift_) < n_buckets; buckets_shift_++) {}
    buckets_ = static_cast<Bucket *>(arena_.Allocate(sizeof(Bucket) * buckets_size()));
    ::memset(buckets_, 0, sizeof(Bucket) * buckets_size());
    record_sequance_ = 0;
    size_ = 0;
}

//int RememberSet::Compare(const RememberRecord &lhs, const RememberRecord &rhs) const {
//    int rv = 0;
//    if (lhs.address < rhs.address) {
//        rv = -1;
//    } else if (lhs.address > rhs.address) {
//        rv = 1;
//    }
//    if (rv != 0) {
//        return rv;
//    }
//    if (lhs.seuqnce_number < rhs.seuqnce_number) {
//        return 1;
//    } else if (lhs.seuqnce_number > rhs.seuqnce_number) {
//        return -1;
//    }
//    return 0;
//}

RememberSet::Node *RememberSet::NewNode(const RememberRecord &key, Tag tag, int height) {
    size_t request_size = sizeof(Node) + sizeof(std::atomic<Node *>) * (height - 1);
    Node *node = static_cast<Node *>(arena_.Allocate(request_size));
    node->record = key;
    node->tag = tag;
    return node;
}

RememberSet::Node *
RememberSet::FindGreaterOrEqual(const RememberRecord &key, Node** prev,
                                Bucket *bucket) const {
    auto x = &bucket->head;
    int level = bucket->max_height - 1;
    while (true) {
        Node* next = x->next[level];
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
    RememberSet::Iterator iter(isolate_->gc()->remember_set());

    std::set<Any **> for_clean;
    Any **latest_key = nullptr;
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        if (latest_key == iter->address) {
            continue;
        }
        latest_key = iter->address;
        if (iter.is_deletion()) {
            continue;
        }
        DCHECK(iter.is_record());

        Any *host = iter->host;
        weak_visitor.VisitPointer(host, &host); // Should sweep remember entries
        if (!host) {
            for_clean.insert(iter->address);
            count++;
        }
    }
    isolate_->gc()->PurgeRememberSet(for_clean);
    return count;
}

} // namespace lang

} // namespace mai
