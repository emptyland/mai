#pragma once
#ifndef MAI_LANG_SCHEDULER_H_
#define MAI_LANG_SCHEDULER_H_

#include "lang/machine.h"
#include "lang/coroutine.h"
#include "lang/stack.h"
#include "glog/logging.h"
#include <mutex>

namespace mai {

namespace lang {

class Coroutine;

class Scheduler final {
public:
    static constexpr int kMaxFreeCoroutines = 80;

    Scheduler(int concurrency, Allocator *lla);
    ~Scheduler();

    DEF_VAL_GETTER(int, concurrency);
    DEF_PTR_GETTER(Machine, machine0);
    
    int shutting_down() const { return shutting_down_.load(std::memory_order_acquire); }

    size_t n_live_coroutines() const { return n_live_coroutines_.load(); }
    size_t n_live_stacks() const { return n_live_stacks_.load(); }
    size_t stack_pool_rss() const { return stack_pool_rss_.load(); }
    
    Machine *machine(int i) const {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, concurrency_);
        return all_machines_[i];
    }

    void Schedule();
    
    void Shutdown();
    
    void Start() {
        for (int i = 1; i < concurrency_; i++) {
            all_machines_[i]->Start();
        }
    }

    Coroutine *NewCoroutine(Closure *entry, bool co0);
    void PurgreCoroutine(Coroutine *co);

    inline Stack *NewStack(size_t size);
    inline bool PurgreStack(Stack *stack);

    DISALLOW_IMPLICIT_CONSTRUCTORS(Scheduler);
private:
    int concurrency_; // Number of machines
    Allocator *const lla_; // Low-Level Allocator
    std::unique_ptr<Machine *[]> all_machines_; // All machines(threads)
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
    
    std::atomic<int> shutting_down_ = 0; // Scheduling should shutdown?
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
