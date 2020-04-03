#include "lang/scheduler.h"
#include "lang/coroutine.h"

namespace mai {

namespace lang {

Scheduler::Scheduler(int concurrency, Allocator *lla)
    : concurrency_(concurrency)
    , lla_(DCHECK_NOTNULL(lla))
    , all_machines_(new Machine*[concurrency])
    , machine_bitmap_(new std::atomic<uint32_t>[(concurrency + 31)/32])
    , free_dummy_(Coroutine::NewDummy())
    , stack_pool_(Stack::NewDummy()) {
    DCHECK_GT(concurrency, 0);
    
    for (int i = 0; i < concurrency_; i++) {
        all_machines_[i] = new Machine(i, this);
    }
    machine0_ = all_machines_[0];
    DCHECK_EQ(0, machine0_->id());

    ::memset(&machine_bitmap_[0], 0, machine_bitmap_size() * sizeof(uint32_t));
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

void Scheduler::Schedule() {
    Machine *m = DCHECK_NOTNULL(Machine::This());

    switch (DCHECK_NOTNULL(m->running())->state()) {
        case Coroutine::kPanic:
            m->running()->SwitchState(Coroutine::kPanic, Coroutine::kDead);
            [[fallthrough]];
        case Coroutine::kDead: {
            // TODO: works steal
            PurgreCoroutine(m->running());
            m->running_ = nullptr;
            TLS_STORAGE->coroutine = nullptr;
        } break;
            
        case Coroutine::kInterrupted: {
            if (m->running()->waitting()) {
                m->running()->set_state(Coroutine::kWaitting);
                Coroutine *co = m->running();
                m->running_ = nullptr;
                TLS_STORAGE->coroutine = nullptr;
                m->PostWaitting(co); // Post for waitting
                break;
            }

            int n = all_machines_[0]->GetNumberOfRunnable();
            Machine *target = all_machines_[0];
            for (int i = 1; i < concurrency_; i++) {
                if (all_machines_[i]->state() == Machine::kIdle) {
                    target = all_machines_[i];
                    break;
                }
                if (all_machines_[i]->GetNumberOfRunnable() < n) {
                    n = all_machines_[i]->GetNumberOfRunnable();
                    target = all_machines_[i];
                }
            }

            Coroutine *co = m->running();
            m->running_ = nullptr;
            TLS_STORAGE->coroutine = nullptr;
            target->PostRunnable(co, false/**/);
        } break;
            
        default:
            NOREACHED();
            break;
    }
}

void Scheduler::Shutdown() {
    DCHECK_LE(0, shutting_down_.load());
    shutting_down_.fetch_add(1);

    for (int i = 1; i < concurrency_; i++) {
        Machine *m = all_machines_[i];
        m->Touch();
        m->thread_.join();
        DCHECK_EQ(Machine::kDead, m->state());
    }
}

void Scheduler::Pause() {
    Machine *self = Machine::This();
    pause_request_.store(1, std::memory_order_relaxed);
    for (int i = 1; i < concurrency_; i++) {
        Machine *m = all_machines_[i];
        if (m == self) {
            continue;
        }
        switch (m->state()) {
            case Machine::kIdle:
                m->RequestSuspend(true/*now*/);
                pause_request_.fetch_add(1, std::memory_order_release);
                break;
            case Machine::kRunning:
                m->RequestSuspend(false/*now*/);
                //printf("running...!!!\n");
                break;
            case Machine::kSuspend:
            case Machine::kDead:
            case Machine::kPanic:
                // Ignore
                break;
            default:
                NOREACHED();
                break;
        }
    }
    //std::unique_lock<std::mutex> lock(mutex_);
    while (pause_request_.load(std::memory_order_acquire) > 1) {
        // Waiting for others machine paused
        //cond_var_.wait(lock);
        std::this_thread::yield();
    }
    self->set_state(Machine::kSuspend); // Then stop self
}

void Scheduler::Resume() {
    Machine *self = Machine::This();
    pause_request_.store(0, std::memory_order_release);
    for (int i = 1; i < concurrency_; i++) {
        Machine *m = all_machines_[i];
        if (m == self) {
            continue;
        }
        switch (m->state()) {
            case Machine::kSuspend:
                m->Touch();
                break;
            case Machine::kIdle:
            case Machine::kDead:
            case Machine::kPanic:
                // Ignore
                break;
            case Machine::kRunning:
                while (m->state() == Machine::kRunning) {
                    //printf("span...\n");
                    std::this_thread::yield();
                }
                break;
            default:
                NOREACHED();
                break;
        }
    }
    self->set_state(Machine::kRunning);
}

void Scheduler::PostRunnableBalanced(Coroutine *co, bool now) {
    Machine *target = machine(0);
    int load = target->GetNumberOfRunnable();
    for (int i = 1; i < concurrency_; i++) {
        Machine *m = all_machines_[i];
        if (int n = m->GetNumberOfRunnable(); n < load) {
            load = n;
            target = m;
        }
    }
    target->PostRunnable(co, now);
}

Coroutine *Scheduler::NewCoroutine(Closure *entry, bool co0) {
    const size_t stack_size = co0 ? kC0StackSize : kDefaultStackSize;
    Stack *stack = NewStack(stack_size);
    if (!stack) {
        return nullptr;
    }

    n_live_coroutines_.fetch_add(1);
    Machine *m = Machine::This();
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
    
    Machine *m = Machine::This();
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
