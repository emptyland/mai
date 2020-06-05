#pragma once
#ifndef MAI_LANG_PROFILER_H_
#define MAI_LANG_PROFILER_H_

#include "lang/bytecode.h"
#include "lang/mm.h"
#include "base/slice.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {

namespace lang {

class Machine;
class Function;

// The profiler for PGO(Profiling Guide Optimization)
class Profiler {
public:
    Profiler(int hot_threshold): hot_threshold_(hot_threshold) {}
    ~Profiler() { delete [] hot_count_slots_; }
    
    DEF_PTR_GETTER(int, hot_count_slots);
    
    int NextHotCountSlot() { return max_hot_count_slots_++; }
    
    void Restore(int slot, int factor) {
        DCHECK_GE(slot, 0);
        DCHECK_LT(slot, max_hot_count_slots_);
        OfAtmoic(hot_count_slots_ + slot)->store(hot_threshold_ * factor);
    }

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


class Tracer {
public:
    enum State {
        kIdle,
        kPending,
        kEnd,
        kError,
    }; // enum State
    
    static constexpr size_t kDummySize = 128;
    
    static const int32_t kOffsetState;
    static const int32_t kOffsetPath;
    static const int32_t kOffsetPathSize;
    static const int32_t kOffsetPathCapacity;
    static const int32_t kOffsetLimitSize;
    static const int32_t kOffsetGuardSlot;

    Tracer(Machine *owns)
        : owns_(owns)
        , path_(&dummy_[0]) {
        ::memset(dummy_, 0, sizeof(dummy_[0]) * kDummySize);
    }

    ~Tracer() {
        if (path_ != dummy_) { delete [] path_; }
    }
    
    void Start(Function *fun, int slot, int pc);
    
    void Abort() {
        DCHECK_EQ(state_, kPending);
        if (path_ != dummy_) {
            delete[] path_;
            path_capacity_ = kDummySize;
        }
        path_size_ = 0;
        state_ = kError;
    }
    
    void Finalize() {
        DCHECK_EQ(state_, kPending);
        state_ = kEnd;
    }

    void GrowTracingPath();

    DEF_VAL_GETTER(State, state);
    DEF_VAL_GETTER(size_t, path_size);
    DEF_VAL_GETTER(size_t, path_capacity);
    DEF_VAL_GETTER(int, guard_slot);
    DEF_VAL_GETTER(int, guard_pc);

    BytecodeInstruction path(size_t i) const {
        DCHECK_LT(i, path_size_);
        return path_[i];
    }
private:
    Machine *owns_;
    State state_ = kIdle;
    BytecodeInstruction *path_;
    size_t path_size_ = 0;
    size_t path_capacity_ = kDummySize;
    size_t limit_size_ = 1024;
    Function *guard_fun_ = nullptr;
    int guard_slot_ = 0;
    int guard_pc_ = 0;
    BytecodeInstruction dummy_[kDummySize];
}; // class Tracing

} // namespace lang

} // namespace mai

#endif // MAI_LANG_PROFILER_H_
