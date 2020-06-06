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
class Coroutine;
class Function;
class CompilationInfo;

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
    
    struct InvokeInfo {
        Function *fun;
        int slot;
        size_t position;
    };
    
    // Size of static dummy buffer
    static constexpr size_t kDummySize = 128;
    
    // How many repeated tracing
    static constexpr int kRepeatedCount = 5;
    
    static const int32_t kOffsetState;
    static const int32_t kOffsetPath;
    static const int32_t kOffsetPC;
    static const int32_t kOffsetPathSize;
    static const int32_t kOffsetPathCapacity;
    static const int32_t kOffsetRepeatedCount;
    static const int32_t kOffsetLimitSize;
    static const int32_t kOffsetGuardSlot;

    Tracer(Machine *owns)
        : owns_(owns)
        , path_(&dummy_path_[0])
        , pc_(&dummy_pc_[0]){
        ::memset(dummy_path_, 0, sizeof(dummy_path_[0]) * kDummySize);
        ::memset(dummy_pc_, 0, sizeof(dummy_pc_[0]) * kDummySize);
    }

    ~Tracer() {
        if (path_ != dummy_path_) { delete [] path_; }
        if (pc_ != dummy_pc_) { delete [] pc_; }
    }
    
    void Start(Function *fun, int slot, int pc);
    
    void Abort();
    
    void Finalize() {
        DCHECK_EQ(state_, kPending);
        state_ = kEnd;
    }

    void Invoke(Function *fun, int slot) {
        DCHECK_GT(path_size_, 1);
        invoke_info_.push_back({fun, slot, path_size_ - 1});
    }
    
    void Repeat() {
        path_size_ = 0;
        invoke_info_.clear();
    }
    
    void GrowTracingPath();
    
    CompilationInfo *MakeCompilationInfo(Machine *mach, Coroutine *owns) const;

    DEF_VAL_GETTER(State, state);
    DEF_VAL_GETTER(size_t, path_size);
    DEF_VAL_GETTER(size_t, path_capacity);
    DEF_VAL_GETTER(int, guard_slot);
    DEF_VAL_GETTER(int, guard_pc);

    BytecodeInstruction path(size_t i) const {
        DCHECK_LT(i, path_size_);
        return path_[i];
    }
    
    uint32_t pc(size_t i) const {
        DCHECK_LT(i, path_size_);
        return pc_[i];
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Tracer)
private:
    Machine *owns_;
    State state_ = kIdle;
    BytecodeInstruction *path_;
    uint32_t *pc_;
    size_t path_capacity_ = kDummySize;
    size_t limit_size_ = 1024;
    Function *guard_fun_ = nullptr;
    int guard_slot_ = 0;
    int guard_pc_ = 0;
    int repeated_count_ = kRepeatedCount;
    size_t path_size_ = 0;
    std::vector<InvokeInfo> invoke_info_;
    BytecodeInstruction dummy_path_[kDummySize];
    uint32_t dummy_pc_[kDummySize];
}; // class Tracer

class TracingHook {
public:
    TracingHook() = default;
    virtual ~TracingHook() = default;
    
    virtual void DidStart(Tracer *sender) = 0;
    virtual void DidAbort(Tracer *sender) = 0;
    virtual void DidRepeat(Tracer *sender) = 0;
    virtual void DidFinailize(Tracer *sender, CompilationInfo *) = 0;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(TracingHook)
}; // class TracingHook

} // namespace lang

} // namespace mai

#endif // MAI_LANG_PROFILER_H_
