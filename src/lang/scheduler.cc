#include "lang/scheduler.h"
#include "lang/coroutine.h"

namespace mai {

namespace lang {

Scheduler::Scheduler(int concurrency, Allocator *lla)
    : concurrency_(concurrency)
    , lla_(DCHECK_NOTNULL(lla))
    , all_machines_(new Machine*[concurrency])
    , free_dummy_(Coroutine::NewDummy())
    , stack_pool_(Stack::NewDummy()) {
    DCHECK_GT(concurrency, 0);
    
    for (int i = 0; i < concurrency_; i++) {
        all_machines_[i] = new Machine(i, this);
    }
    machine0_ = all_machines_[0];
    DCHECK_EQ(0, machine0_->id());
    // TODO:
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
        x->Dispose(lla_);
    }
    Stack::DeleteDummy(stack_pool_);
}

Coroutine *Scheduler::NewCoroutine(Closure *entry, bool co0) {
    const size_t stack_size = co0 ? kC0StackSize : kDefaultStackSize;
    Stack *stack = NewStack(stack_size);
    if (!stack) {
        return nullptr;
    }

    n_live_coroutines_.fetch_add(1);
    Machine *m = Machine::Get();
    Coroutine *co = m->TakeFreeCoroutine();
    if (co) {
        co->Reinitialize(next_coid_.fetch_add(1), entry, stack);
        return co;
    }
    {
        std::lock_guard<std::mutex> lock(free_dummy_mutex_);
        co = free_dummy_->next();
        if (co != free_dummy_) {
            QUEUE_REMOVE(co);
            n_free_--;
            co->Reinitialize(next_coid_.fetch_add(1), entry, stack);
            return co;
        }
    }
    return new Coroutine(next_coid_.fetch_add(1), entry, stack);
}

void Scheduler::PurgreCoroutine(Coroutine *co) {
    co->Dispose();
    n_live_coroutines_.fetch_sub(1);
    
    Machine *m = Machine::Get();
    if (m->n_free() + 1 <= Machine::kMaxFreeCoroutines) {
        m->InsertFreeCoroutine(co);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(free_dummy_mutex_);
        if (n_free_ + 1 <= kMaxFreeCoroutines) {
            QUEUE_INSERT_HEAD(free_dummy_, co);
            n_free_++;
            return;
        }
    }
    delete co;
}

} // namespace lang

} // namespace mai
