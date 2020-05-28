#pragma once
#ifndef MAI_LANG_GARBAGE_COLLECTOR_H_
#define MAI_LANG_GARBAGE_COLLECTOR_H_

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

using RememberMap = std::map<void *, RememberRecord>;

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
    
    GarbageCollector(Isolate *isolate, float minor_gc_threshold_rate, float major_gc_threshold_rate)
        : isolate_(isolate)
        , minor_gc_threshold_rate_(minor_gc_threshold_rate)
        , major_gc_threshold_rate_(major_gc_threshold_rate) {}

    DEF_VAL_PROP_RW(size_t, latest_minor_remaining_size);
    
    State state() const { return state_.load(std::memory_order_acquire); }
    void set_state(State state) { state_.store(state, std::memory_order_release); }
    
    uint64_t tick() const { return tick_.load(std::memory_order_acquire); }
    
    bool AcquireState(State expect, State state) {
        return state_.compare_exchange_strong(expect, state);
    }
    
    State ShouldCollect();
    
    void CollectIfNeeded();

    void MinorCollect();
    void MajorCollect();
    void FullCollect();

    uint64_t NextRememberRecordSequanceNumber() { return remember_record_sequance_.fetch_add(1); }
    
    //const RememberSet &MergeRememberSet();
    RememberMap MergeRememberSet();

    void PurgeRememberSet(const std::set<void *> &keys);
    void PurgeRememberSet();
    
    void InvalidateHeapGuards(Address guard0, Address guard1);

    DISALLOW_IMPLICIT_CONSTRUCTORS(GarbageCollector);
private:
    Isolate *const isolate_;
    // Minor GC available threshold rate
    const float minor_gc_threshold_rate_;

    // Major GC available threshold rate
    const float major_gc_threshold_rate_;
    
    // Remember set
    //RememberSet remember_set_;
    
    // Remember set record for old-generation -> new-generation
    // Remember record version number
    std::atomic<uint64_t> remember_record_sequance_ = 0;
    
    // State of GC progress
    std::atomic<State> state_ = kIdle;
    
    // Tick of GC progress
    std::atomic<uint64_t> tick_ = 0;

    // Latest minor GC remaining bytes
    size_t latest_minor_remaining_size_ = 0;
    
    // Latest remember set size
    size_t latest_remember_set_size_ = 0;
}; // class GarbageCollector

class RememberSet {
public:
    enum Tag {
        kBucket,
        kRecord,
        kDeletion,
    };
    
    struct RememberNode {
        RememberRecord record;
        Tag tag;
        std::atomic<RememberNode *> next[1];
        
        RememberNode *barrier_next(int i) const {
            return next[i].load(std::memory_order_acquire);
        }
        void barrier_set_next(int i, RememberNode *node) {
            next[i].store(node, std::memory_order_release);
        }
        RememberNode *nobarrier_next(int i) const {
            return next[i].load(std::memory_order_relaxed);
        }
        void nobarrier_set_next(int i, RememberNode *node) {
            next[i].store(node, std::memory_order_relaxed);
        }
    }; // struct RememberNode
    
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
        RememberNode *node_ = nullptr;
    }; // class Iterator
    
    static const int kMaxHeight = 12;
    static const int kBranching = 4;

    RememberSet(Allocator *lla, size_t n_buckets);
    
    void Put(Any *host, Any **address) { Append(host, address, kRecord); }

    void Delete(Any **address) { Append(nullptr, address, kDeletion); }

    size_t size() const { return size_.load(std::memory_order_acquire); }
    
    size_t buckets_size() const { return 1u <<  buckets_shift_; }

    DISALLOW_IMPLICIT_CONSTRUCTORS(RememberSet);
private:
    uintptr_t HashCode(Any **address) const {
        return (reinterpret_cast<uintptr_t>(address) >> 2) & ((1u << buckets_shift_) - 1);
    }
    
    void Append(Any *host, Any **address, Tag tag);
    
    int Compare(const RememberRecord &lhs, const RememberRecord &rhs) const;
    
    RememberNode *NewNode(const RememberRecord &key, Tag tag, int height);
    
    RememberNode *NewBucket(int height);
    
    RememberNode *FindGreaterOrEqual(const RememberRecord &key, RememberNode** prev,
                                     RememberNode *head) const;
    
    bool Equal(const RememberRecord &lhs, const RememberRecord &rhs) const {
        return Compare(lhs, rhs) == 0;
    }
    
    bool KeyIsAfterNode(const RememberRecord &key, RememberNode *n) const {
        // NULL n is considered infinite
        return (n != nullptr) && (Compare(n->record, key) < 0);
    }
    
    int RandomHeight() {
        int height = 1;
        while (height < kMaxHeight && ::rand() % 16 == 0) {
            height++;
        }
        DCHECK_GT(height, 0);
        DCHECK_LE(height, kMaxHeight);
        return height;
    }
    
    int max_height() const { return max_height_.load(std::memory_order_relaxed); }
    
    base::StandaloneArena arena_;
    
    // Remember set record for old-generation -> new-generation
    // Remember record version number
    std::atomic<uint64_t> record_sequance_ = 0;

    // Size of records
    std::atomic<size_t> size_ = 0;
    
    // Max Height for skip list
    std::atomic<int> max_height_ = 1;

    // Bucket array
    RememberNode **buckets_;
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


inline void RememberSet::Iterator::SeekToFirst() {
    for (size_t i = 0; i < owns_->buckets_size(); i++) {
        if (owns_->buckets_[i]->next[0]) {
            node_ = owns_->buckets_[i]->next[0];
            bucket_ = i;
            break;
        }
    }
}

inline void RememberSet::Iterator::Next() {
    node_ = node_->next[0];
    if (!node_) {
        while (++bucket_ < owns_->buckets_size()) {
            if (owns_->buckets_[bucket_]->next[0]) {
                node_ = owns_->buckets_[bucket_]->next[0];
                break;
            }
        }
    }
}

} // namespace lang

} // namespace mai

#endif // MAI_LANG_GARBAGE_COLLECTOR_H_
