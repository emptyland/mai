#pragma once
#ifndef MAI_LANG_GARBAGE_COLLECTOR_H_
#define MAI_LANG_GARBAGE_COLLECTOR_H_

#include "base/base.h"
#include <string.h>
#include <map>
#include <set>
#include <atomic>

namespace mai {
namespace base {
class AbstractPrinter;
} // namespace base
namespace lang {

class Isolate;
class Heap;
class Any;
class SafepointScope;

// Remember set record for old-generation -> new-generation
struct RememberRecord {
    uint64_t seuqnce_number; // Sequence number
    Any *host; // Host object pointer
    Any **address; // Write to address
}; //struct RememberRecord

using RememberSet = std::map<void *, RememberRecord>;

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
    
    void CollectIfNeeded();

    void MinorCollect();
    void MajorCollect();
    void FullCollect();

    uint64_t NextRememberRecordSequanceNumber() { return remember_record_sequance_.fetch_add(1); }
    
    const RememberSet &MergeRememberSet();

    void PurgeRememberSet(const std::set<void *> &keys) {
        for (auto key : keys) {
            remember_set_.erase(key);
        }
    }
    
    void InvalidateHeapGuards(Address guard0, Address guard1);

    DISALLOW_IMPLICIT_CONSTRUCTORS(GarbageCollector);
private:
    Isolate *const isolate_;
    // Minor GC available threshold rate
    const float minor_gc_threshold_rate_;

    // Major GC available threshold rate
    const float major_gc_threshold_rate_;
    
    // Remember set
    RememberSet remember_set_;
    
    // Remember set record for old-generation -> new-generation
    // Remember record version number
    std::atomic<uint64_t> remember_record_sequance_ = 0;
    
    // State of GC progress
    std::atomic<State> state_ = kIdle;
    
    // Tick of GC progress
    std::atomic<uint64_t> tick_ = 0;

    // Latest minor GC remaining bytes
    size_t latest_minor_remaining_size_ = 0;
}; // class GarbageCollector

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

} // namespace lang

} // namespace mai

#endif // MAI_LANG_GARBAGE_COLLECTOR_H_
