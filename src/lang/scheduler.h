#pragma once
#ifndef MAI_LANG_SCHEDULER_H_
#define MAI_LANG_SCHEDULER_H_

#include "lang/machine.h"
#include "lang/coroutine.h"
#include "lang/stack.h"
#include "lang/object-visitor.h"
#include "base/bit-ops.h"
#include "glog/logging.h"
#include <mutex>

namespace mai {

namespace lang {

class Coroutine;
class CompilationWorker;

class Scheduler final {
public:
    static constexpr int kMaxFreeCoroutines = 80;
    static constexpr int kBitmapShift = 5;
    static constexpr int kBitmapBits = 1 << kBitmapShift;
    static constexpr size_t kBitmapMask = kBitmapBits - 1;
    
    enum State {
        kRunable,
        kRunning,
        kPause,
        kShutdown,
    };

    Scheduler(int concurrency, Allocator *lla, CompilationWorker *compilation_worker,
              bool enable_jit);
    ~Scheduler();

    DEF_VAL_GETTER(int, concurrency);
    DEF_PTR_GETTER(Machine, machine0);
    DEF_VAL_MUTABLE_GETTER(std::mutex, mutex);
    
    State state() const { return state_.load(); }
    
    int MarkShuttingDown() { return shutting_down_.fetch_add(1); }

    int shutting_down() const { return shutting_down_.load(std::memory_order_acquire); }
    int pause_request() const { return pause_request_.load(std::memory_order_acquire); }

    size_t n_live_coroutines() const { return n_live_coroutines_.load(); }
    size_t n_live_stacks() const { return n_live_stacks_.load(); }
    size_t stack_pool_rss() const { return stack_pool_rss_.load(); }
    
    CompilationWorker *compilation_worker() const { return compilation_worker_.get(); }
    
    Machine *machine(int i) const {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, concurrency_);
        return all_machines_[i];
    }
    
    int GetNumberOfSuspend() const;

    // Re-Schedule coroutines with machine
    void Schedule();

    // Shutdown all machines, and waitting for all machines done
    void Shutdown();
    
    // Start all machines thread, expect main machine(machine0)
    void Start() {
        for (int i = 1; i < concurrency_; i++) {
            all_machines_[i]->Start();
        }
        State expect = kRunable;
        bool ok = state_.compare_exchange_strong(expect, kRunning);
        DCHECK(ok);
        (void)ok;
    }

    // Pause all machines exclude self one
    bool Pause();

    // Resume all machines exclude self one
    bool Resume();

    // Balanced post a runnable coroutine to machine
    void PostRunnableBalanced(Coroutine *co, bool now);
    
    // Mark a machine is idle
    void MarkIdle(Machine *m) {
        DCHECK_EQ(m, Machine::This());
        DCHECK_EQ(Machine::kIdle, m->state());
        DCHECK_LT(m->id(), concurrency_);
        machine_bitmap_[m->id() >> kBitmapShift].fetch_or(1u << (m->id() & kBitmapMask));
    }

    // Clear a machine idle marked
    void ClearIdle(Machine *m) {
        DCHECK_EQ(Machine::kRunning, m->state());
        DCHECK_LT(m->id(), concurrency_);
        machine_bitmap_[m->id() >> kBitmapShift].fetch_and(~(1u << (m->id() & kBitmapMask)));
    }

    Coroutine *NewCoroutine(Closure *entry, bool co0);
    void PurgreCoroutine(Machine *m, Coroutine *co);
    void PurgreCoroutine(Coroutine *co) { PurgreCoroutine(Machine::This(), co); }

    inline Stack *NewStack(size_t size);
    inline bool PurgreStack(Stack *stack);
    
    // For GC
    void VisitRoot(RootVisitor *visitor) {
        for (int i = 0; i < concurrency_; i++) {
            machine(i)->VisitRoot(visitor);
        }
    }

    DISALLOW_IMPLICIT_CONSTRUCTORS(Scheduler);
private:
    size_t machine_bitmap_size() const { return (concurrency_ + 31) >> kBitmapShift; }

//    Machine *FindFirstIdleMachine() {
//        int i = 0;
//        for (i = 0; i < machine_bitmap_size(); i++) {
//            if (machine_bitmap_[0].load() != 0) {
//                break;
//            }
//        }
//        if (i == machine_bitmap_size()) {
//            return nullptr;
//        }
//        int bit = base::Bits::FindFirstOne32(machine_bitmap_[0].load());
//        int index = (i << kBitmapShift) + bit;
//    }
    
    int concurrency_; // Number of machines
    Allocator *const lla_; // Low-Level Allocator
    std::atomic<State> state_ = kRunable;
    std::unique_ptr<Machine *[]> all_machines_; // All machines(threads)
    
    std::unique_ptr<CompilationWorker> compilation_worker_;
    
    // The one bits means: Machine is idle
    // The zero bits means: Machine is running
    std::unique_ptr<std::atomic<uint32_t>[]> machine_bitmap_; // Bitmap of machines

    Coroutine *free_dummy_; // Free coroutines
    std::mutex free_dummy_mutex_; // mutex of free-dummy
    int n_free_ = 0; // Number free coroutines
    std::atomic<size_t> n_live_coroutines_ = 0; // Number of all coroutines

    Stack *stack_pool_; // Stack pool
    std::atomic<size_t> stack_pool_rss_ = 0; // RSS size of stack-pool
    std::atomic<size_t> n_live_stacks_ = 0; // Number of live stacks
    std::mutex stack_pool_mutex_; // mutex of stack-pool
    Machine *machine0_ = nullptr; // Machine No.0 (main thread)
    std::atomic<uint64_t> next_coid_ = 0; // Next coroutine-id
    
    std::atomic<int> pause_request_ = 0; // Pause request number
    std::atomic<int> shutting_down_ = 0; // Scheduling should shutdown?
    
    std::mutex mutex_;
    std::condition_variable cond_var_; // Condition variable for machines pause
}; // class Scheduler

inline Stack *Scheduler::NewStack(size_t size) {
    {
        std::lock_guard<std::mutex> lock(stack_pool_mutex_);
        for (Stack *stack = stack_pool_->next(); stack != stack_pool_; stack = stack->next()) {
            if (stack->GetAvailableSize() < size) {
                n_live_stacks_.fetch_add(1);
                return stack;
            }
        }
    }
    Stack *stack = Stack::New(size, lla_);
    n_live_stacks_.fetch_add(1);
    stack_pool_rss_.fetch_add(stack->size());
    return stack;
}

inline bool Scheduler::PurgreStack(Stack *stack) {
    n_live_stacks_.fetch_sub(1);
    std::lock_guard<std::mutex> lock(stack_pool_mutex_);
    if (stack_pool_rss_ + stack->size() > kMaxStackPoolRSS) {
        stack_pool_rss_.fetch_sub(stack->size());
        stack->Dispose(lla_);
        return true;
    }
    stack->Reinitialize();
    QUEUE_INSERT_HEAD(stack_pool_, stack);
    return false;
}

} // namespace lang

} // namespace mai

#endif // MAI_LANG_SCHEDULER_H_
