#pragma once
#ifndef MAI_LANG_PROFILER_H_
#define MAI_LANG_PROFILER_H_

#include "lang/bytecode.h"
#include "base/slice.h"
#include "base/base.h"

namespace mai {

namespace lang {

class Machine;

// The profiler for PGO(Profiling Guide Optimization)
class Profiler {
public:
    Profiler(int hot_threshold): hot_threshold_(hot_threshold) {}

    ~Profiler() { delete [] hot_count_slots_; }
    
    int NextHotCountSlot() { return max_hot_count_slots_++; }
    
    void Reset() {
        delete [] hot_count_slots_;
        hot_count_slots_ = new int[max_hot_count_slots_];
        base::Round32BytesFill(hot_threshold_, hot_count_slots_,
                               sizeof(hot_count_slots_[0]) * max_hot_count_slots_);
    }
private:
    const int hot_threshold_;
    int *hot_count_slots_ = nullptr;
    int max_hot_count_slots_ = 1; // 0 is invalid slot number
}; // class Profiler


class Tracing {
public:
    enum State {
        kIdle,
        kStart,
        kEnd,
        kError,
    }; // enum State
    
    static constexpr size_t kDummySize = 128;
    
    Tracing(Machine *owns)
        : owns_(owns)
        , path_(&dummy_[0]) {
        ::memset(dummy_, 0, sizeof(dummy_[0]) * kDummySize);
    }

    ~Tracing() {
        if (path_ != dummy_) { delete [] path_; }
    }

    DEF_VAL_GETTER(size_t, path_size);
    DEF_VAL_GETTER(size_t, path_capacity);
    
    BytecodeInstruction path(size_t i) const {
        DCHECK_LT(i, path_size_);
        return path_[i];
    }
private:
    Machine *owns_;
    State state_ = kIdle;
    BytecodeInstruction *path_ = nullptr;
    size_t path_size_ = 0;
    size_t path_capacity_ = 0;
    BytecodeInstruction dummy_[kDummySize];
}; // class Tracing

} // namespace lang

} // namespace mai

#endif // MAI_LANG_PROFILER_H_
