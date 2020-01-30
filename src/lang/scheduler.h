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
    Scheduler(int concurrency);
    ~Scheduler();
    
    DEF_VAL_GETTER(int, concurrency);
    DEF_PTR_GETTER(Machine, machine0);
    
    Coroutine *NewCoroutine(Closure *entry, bool co0);
    
    Stack *NewStack(size_t size) {
        {
            std::lock_guard<std::mutex> lock(stack_pool_mutex_);
            for (Stack *stack = stack_pool_->next(); stack != stack_pool_; stack = stack->next()) {
                if (stack->GetAvailableSize() < size) {
                    return stack;
                }
            }
        }
        return Stack::New(size, __isolate->env()->GetLowLevelAllocator());
    }
    
    void PurgreStack(Stack *stack) {
        std::lock_guard<std::mutex> lock(stack_pool_mutex_);
        if (stack_pool_rss_ + stack->size() > kMaxStackPoolRSS) {
            stack->Dispose(__isolate->env()->GetLowLevelAllocator());
            return;
        }
        stack->Reinitialize();
        stack_pool_rss_ += stack->size();
        QUEUE_INSERT_HEAD(stack_pool_, stack);
    }

    DISALLOW_IMPLICIT_CONSTRUCTORS(Scheduler);
private:
    int concurrency_;
    std::unique_ptr<Machine *[]> all_machines_;
    Coroutine *free_dummy_; // Free coroutines
    std::mutex free_dummy_mutex_; // mutex of free-dummy
    
    Stack *stack_pool_; // Stack pool
    size_t stack_pool_rss_ = 0; // RSS size of stack-pool
    std::mutex stack_pool_mutex_; // mutex of stack-pool
    Machine *machine0_ = nullptr;
    std::atomic<uint64_t> next_coid_ = 0;
}; // class Scheduler

} // namespace lang

} // namespace mai

#endif // MAI_LANG_SCHEDULER_H_
