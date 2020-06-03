#pragma once
#ifndef MAI_LANG_GARBAGE_COLLECTOR_H_
#define MAI_LANG_GARBAGE_COLLECTOR_H_

#include "base/spin-locking.h"
#include "base/arenas.h"
#include "base/base.h"
#include <string.h>
#include <map>
#include <set>
#include <atomic>
#include <stack>

namespace mai {
namespace base {
class AbstractPrinter;
} // namespace base
namespace lang {

class Isolate;
class Heap;
class Any;
class SafepointScope;
class RootVisitor;
class ObjectVisitor;
class RememberSet;

// Remember set record for old-generation -> new-generation
struct RememberRecord {
    uint64_t seuqnce_number; // Sequence number
    Any *host; // Host object pointer
    Any **address; // Write to address
}; //struct RememberRecord

struct GarbageCollectionHistogram {
    size_t collected_bytes  = 0;
    size_t collected_objs   = 0;
    int64_t micro_time_cost = 0;
}; // struct GarbageCollectionHistogram


class GarbageCollector final {
public:
    enum State {
        kIdle,
        kReady,
        kMinorCollect,
        kMajorCollect,
        kFullCollect,
        kDone,
    };
    
    GarbageCollector(Isolate *isolate, float minor_gc_threshold_rate, float major_gc_threshold_rate);
    ~GarbageCollector();

    DEF_VAL_PROP_RW(size_t, latest_minor_remaining_size);
    
    State state() const { return state_.load(std::memory_order_acquire); }
    void set_state(State state) { state_.store(state, std::memory_order_release); }
    
    uint64_t tick() const { return tick_.load(std::memory_order_acquire); }

    RememberSet *remember_set() const { return remember_set_.get(); }
    
    bool AcquireState(State expect, State state) {
        return state_.compare_exchange_strong(expect, state);
    }
    
    State ShouldCollect();
    
    void CollectIfNeeded();

    void MinorCollect();
    void MajorCollect();
    void FullCollect();

    inline void PurgeRememberSet(const std::set<Any **> &keys);
    inline void PurgeRememberSet();
    
    void InvalidateHeapGuards(Address guard0, Address guard1);

    DISALLOW_IMPLICIT_CONSTRUCTORS(GarbageCollector);
private:
    Isolate *const isolate_;
    // Minor GC available threshold rate
    const float minor_gc_threshold_rate_;

    // Major GC available threshold rate
    const float major_gc_threshold_rate_;
    
    // Remember set
    std::unique_ptr<RememberSet> remember_set_;
    
    // State of GC progress
    std::atomic<State> state_ = kIdle;
    
    // Tick of GC progress
    std::atomic<uint64_t> tick_ = 0;

    // Latest minor GC remaining bytes
    size_t latest_minor_remaining_size_ = 0;
}; // class GarbageCollector

class RememberSet {
public:
    enum Tag {
        kBucket,
        kRecord,
        kDeletion,
    };
    
    static const int kMaxHeight = 12;
    static const int kBranching = 4;
    
    struct Node {
        RememberRecord record;
        Tag tag;
        std::atomic<Node *> next;
    }; // struct Node

    class Iterator {
    public:
        Iterator(const RememberSet *owns): owns_(owns) {}
        inline void SeekToFirst();
        inline void Next();
        bool Valid() const { return bucket_ < owns_->buckets_size() && node_ != nullptr; }
        RememberRecord *operator -> () const { return record(); }
        RememberRecord *record() const { return &DCHECK_NOTNULL(node_)->record; }
        Tag tag() const { return DCHECK_NOTNULL(node_)->tag; }
        bool is_bucket() const { return tag() == kBucket; }
        bool is_record() const { return tag() == kRecord; }
        bool is_deletion() const { return tag() == kDeletion; }
    private:
        const RememberSet *const owns_;
        size_t bucket_ = 0;
        Node *node_ = nullptr;
    }; // class Iterator

    RememberSet(size_t n_buckets);
    ~RememberSet() = default;
    
    void Put(Any *host, Any **address) { Insert(host, address, kRecord); }

    void Delete(Any **address) { Insert(nullptr, address, kDeletion); }
    
    void Purge(size_t n_buckets);

    size_t size() const { return size_.load(std::memory_order_acquire); }
    
    size_t buckets_size() const { return 1u <<  buckets_shift_; }

    DISALLOW_IMPLICIT_CONSTRUCTORS(RememberSet);
private:
    uintptr_t HashCode(Any **address) const {
        return (reinterpret_cast<uintptr_t>(address) >> 2) & ((1u << buckets_shift_) - 1);
    }
    
    void Insert(Any *host, Any **address, Tag tag);
    
    intptr_t Compare(const RememberRecord &lhs, const RememberRecord &rhs) const {
        intptr_t diff = reinterpret_cast<intptr_t>(lhs.address) -
                        reinterpret_cast<intptr_t>(rhs.address);
        return !diff ? static_cast<intptr_t>(rhs.seuqnce_number) -
                       static_cast<intptr_t>(lhs.seuqnce_number) : diff;
    }
    
    Node *NewNode(const RememberRecord &key, Tag tag) {
        Node *node = arena_.New<Node>();
        node->record = key;
        node->tag = tag;
        node->next.store(nullptr, std::memory_order_relaxed);
        return node;
    }

    Node *FindGreaterOrEqual(const RememberRecord &key, Node** prev, Node *bucket) const {
        Node *p = DCHECK_NOTNULL(bucket);
        Node *x = p->next.load(std::memory_order_acquire);
        while (x) {
            if (Compare(x->record, key) >= 0) {
                break;
            }
            p = x;
            x = x->next.load(std::memory_order_acquire);
        }
        *prev = p;
        return x;
    }

    base::StandaloneArena arena_;
    
    // Remember set record for old-generation -> new-generation
    // Remember record version number
    std::atomic<uint64_t> record_sequance_ = 0;

    // Size of records
    std::atomic<size_t> size_ = 0;

    // Bucket array
    Node *buckets_;
    int buckets_shift_;
}; // class RememberSet

class GarbageCollectionPolicy {
public:
    GarbageCollectionPolicy(Isolate *isolate, Heap *heap)
        : isolate_(isolate)
        , heap_(heap) {}
    virtual ~GarbageCollectionPolicy() = default;
    
    DEF_PTR_GETTER(Isolate, isolate);
    DEF_PTR_GETTER(Heap, heap);
    DEF_VAL_GETTER(GarbageCollectionHistogram, histogram);
    
    virtual void Run(base::AbstractPrinter *logger) = 0;

    virtual void Reset() { ::memset(&histogram_, 0, sizeof(histogram_)); }

    DISALLOW_IMPLICIT_CONSTRUCTORS(GarbageCollectionPolicy);
protected:
    Isolate *const isolate_;
    Heap *const heap_;
    GarbageCollectionHistogram histogram_;
}; // class GarbageCollectionPolicy


class PartialMarkingPolicy : public GarbageCollectionPolicy {
public:
    PartialMarkingPolicy(Isolate *isolate, Heap *heap): GarbageCollectionPolicy(isolate, heap) {}
    ~PartialMarkingPolicy() override = default;
    
    DEF_VAL_PROP_RW(bool, full);
protected:
    class RootVisitorImpl;
    class ObjectVisitorImpl;
    class WeakVisitorImpl;
    
    int UnbreakableMark();
    int UnbreakableMark(RootVisitor *root_visitor, ObjectVisitor *object_visitor);
    int UnbreakableSweepNewSpace();
    int SweepLargeSpace();
    int PurgeWeakObjects();

    bool full_ = false; // full gc?
    std::stack<Any *> gray_;
}; // class PartialMarkingPolicy

inline void GarbageCollector::PurgeRememberSet(const std::set<Any **> &keys) {
    for (Any **key : keys) { remember_set_->Delete(key); }
}

inline void GarbageCollector::PurgeRememberSet() {
    remember_set_->Purge(remember_set_->buckets_size());
}

inline void RememberSet::Iterator::SeekToFirst() {
    for (size_t i = 0; i < owns_->buckets_size(); i++) {
        if (owns_->buckets_[i].next.load(std::memory_order_relaxed)) {
            node_ = owns_->buckets_[i].next.load(std::memory_order_relaxed);
            bucket_ = i;
            break;
        }
    }
}

inline void RememberSet::Iterator::Next() {
    node_ = node_->next.load(std::memory_order_relaxed);
    if (!node_) {
        while (++bucket_ < owns_->buckets_size()) {
            if (owns_->buckets_[bucket_].next.load(std::memory_order_relaxed)) {
                node_ = owns_->buckets_[bucket_].next.load(std::memory_order_relaxed);
                break;
            }
        }
    }
}

} // namespace lang

} // namespace mai

#endif // MAI_LANG_GARBAGE_COLLECTOR_H_
