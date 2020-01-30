#include "lang/scheduler.h"
#include "lang/coroutine.h"

namespace mai {

namespace lang {

Scheduler::Scheduler(int concurrency)
    : concurrency_(concurrency)
    , all_machines_(new Machine*[concurrency])
    , free_dummy_(Coroutine::NewDummy())
    , stack_pool_(Stack::NewDummy()) {
    DCHECK_GT(concurrency, 0);
    
    for (int i = 0; i < concurrency_; i++) {
        all_machines_[i] = new Machine(i, this);
    }
    machine0_ = all_machines_[0];
    DCHECK_EQ(0, machine0_->id());
}

Scheduler::~Scheduler() {
    // TODO:
    while (!QUEUE_EMPTY(free_dummy_)) {
        auto x = free_dummy_->next();
        QUEUE_REMOVE(x);
        delete x;
    }
    Coroutine::DeleteDummy(free_dummy_);
    
    while (!QUEUE_EMPTY(stack_pool_)) {
        auto x = stack_pool_->next();
        QUEUE_REMOVE(x);
        x->Dispose(__isolate->env()->GetLowLevelAllocator());
    }
    Stack::DeleteDummy(stack_pool_);
}

Coroutine *Scheduler::NewCoroutine(Closure *entry, bool co0) {
    const size_t stack_size = co0 ? kCoroutine0StackSize : kDefaultStackSize;
    Stack *stack = NewStack(stack_size);
    if (!stack) {
        return nullptr;
    }

    Machine *m = Machine::Get();
    Coroutine *co = m->free_dummy_->next();
    if (co != m->free_dummy_) {
        QUEUE_REMOVE(co);
        co->Reinitialize(next_coid_.fetch_add(1), entry, stack);
        return co;
    }
    {
        std::lock_guard<std::mutex> lock(free_dummy_mutex_);
        co = free_dummy_->next();
        if (co != free_dummy_) {
            QUEUE_REMOVE(co);
            co->Reinitialize(next_coid_.fetch_add(1), entry, stack);
            return co;
        }
    }
    return new Coroutine(next_coid_.fetch_add(1), entry, stack);
}

} // namespace lang

} // namespace mai
