#ifndef MAI_LANG_GARBAGE_COLLECTOR_H_
#define MAI_LANG_GARBAGE_COLLECTOR_H_

#include "base/base.h"
#include <string.h>
#include <map>
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
    
    GarbageCollector(Isolate *isolate): isolate_(isolate) {}

    DEF_VAL_PROP_RW(size_t, latest_minor_remaining_size);
    
    State state() const { return state_.load(std::memory_order_acquire); }
    void set_state(State state) { state_.store(state, std::memory_order_release); }
    
    bool AcquireState(State expect, State state) {
        return state_.compare_exchange_strong(expect, state);
    }
    
    void MinorCollect();
    void MajorCollect();
    void FullCollect();

    uint64_t NextRememberRecordSequanceNumber() { return remember_record_sequance_.fetch_add(1); }
    
    RememberSet MergeRememberSet(bool keep_after);
    
    void InvalidateHeapGuards(Address guard0, Address guard1);
    
    friend class SafepointScope;
    DISALLOW_IMPLICIT_CONSTRUCTORS(GarbageCollector);
private:
    void SafepointEnter();
    void SafepointExit();
    
    Isolate *const isolate_;
    // Remember set record for old-generation -> new-generation
    // Remember record version number
    std::atomic<uint64_t> remember_record_sequance_ = 0;
    
    // State of GC progress
    std::atomic<State> state_ = kIdle;

    // Latest minor GC remaining bytes
    size_t latest_minor_remaining_size_ = 0;
}; // class GarbageCollector


class SafepointScope final {
public:
    SafepointScope(GarbageCollector *gc): gc_(gc) { gc_->SafepointEnter(); }
    ~SafepointScope() { gc_->SafepointExit(); }

    DISALLOW_IMPLICIT_CONSTRUCTORS(SafepointScope);
private:
    GarbageCollector *gc_;
}; // class SafepointScope


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
