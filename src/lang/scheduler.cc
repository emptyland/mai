#include "lang/scheduler.h"
#include "lang/coroutine.h"

namespace mai {

namespace lang {

Scheduler::Scheduler(int concurrency, Allocator *lla, bool enable_jit)
    : concurrency_(concurrency)
    , lla_(DCHECK_NOTNULL(lla))
    , all_machines_(new Machine*[concurrency])
    , machine_bitmap_(new std::atomic<uint32_t>[(concurrency + 31)/32])
    , free_dummy_(Coroutine::NewDummy())
    , stack_pool_(Stack::NewDummy()) {
    DCHECK_GT(concurrency, 0);
    
    for (int i = 0; i < concurrency_; i++) {
        all_machines_[i] = new Machine(i, this, enable_jit);
    }
    machine0_ = all_machines_[0];
    DCHECK_EQ(0, machine0_->id());

    ::memset(&machine_bitmap_[0], 0, machine_bitmap_size() * sizeof(uint32_t));
}

Scheduler::~Scheduler() {
    for (int i = 0; i < concurrency_; i++) {
        all_machines_[i]->Finalize();
    }
    
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
    
    for (int i = 0; i < concurrency_; i++) {
        delete all_machines_[i];
    }
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
            DCHECK(m->running()->waitting() != nullptr);
            if (Coroutine *co = m->running(); co->waitting()) {
                m->running_ = nullptr;
                TLS_STORAGE->coroutine = nullptr;
                m->PostWaitting(co);
                co->SwitchState(Coroutine::kInterrupted, Coroutine::kWaitting);
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
    
    while (state() == kPause) {
        std::this_thread::yield();
    }
    for (int i = 1; i < concurrency_; i++) {
        Machine *m = all_machines_[i];
        do {
            m->Touch();
        } while (m->state() != Machine::kDead);
        m->thread_.join();
        DCHECK_EQ(Machine::kDead, m->state());
    }
    DCHECK_EQ(kRunning, state());
    State expect = kRunning;
    bool ok = state_.compare_exchange_strong(expect, kShutdown);
    DCHECK(ok);
    (void)ok;
}

bool Scheduler::Pause() {
    State expect = kRunning;
    if (!state_.compare_exchange_strong(expect, kPause)) {
        return false;
    }
    Machine *self = Machine::This();
    pause_request_.store(concurrency_);
    int pause_count = 0;
    while (pause_count < concurrency_ && !shutting_down()) {
        pause_count = 0;
        for (int i = 0; i < concurrency_; i++) {
            Machine *m = all_machines_[i];
            if (m == self) {
                pause_count++;
                continue;
            }
            switch (m->state()) {
                case Machine::kIdle:
                    m->RequestSuspend(true/*now*/);
                    break;
                case Machine::kRunning:
                    m->RequestSuspend(false/*now*/);
                    break;
                case Machine::kSuspend:
                    pause_count++;
                    break;
                case Machine::kDead:
                case Machine::kPanic:
                    // Ignore
                    break;
                default:
                    NOREACHED();
                    break;
            }
        }
        std::this_thread::yield();
    }
    self->set_state(Machine::kSuspend); // Then stop self
    return true;
}

bool Scheduler::Resume() {
    DCHECK_EQ(kPause, state());
    Machine *self = Machine::This();
    pause_request_.store(0, std::memory_order_release);
    
    while (GetNumberOfSuspend() > 0 && !shutting_down()) {
        for (int i = 0; i < concurrency_; i++) {
            Machine *m = all_machines_[i];
            if (m == self) {
                m->set_state(Machine::kRunning);
                continue;
            }
            switch (m->state()) {
                case Machine::kSuspend:
                    m->Touch();
                    break;
                case Machine::kIdle:
                    m->Touch();
                    break;
                case Machine::kDead:
                case Machine::kPanic:
                    // Ignore
                    break;
                case Machine::kRunning:
                    while (m->state() == Machine::kRunning && !shutting_down()) {
                        //printf("span...\n");
                        //std::this_thread::yield();
                        __asm__ ("pause");
                    }
                    break;
                default:
                    NOREACHED();
                    break;
            }
        }
        //printf("paused: %d\n", GetNumberOfSuspend());
        //std::this_thread::yield();
        __asm__ ("pause");
    }
    //DCHECK_EQ(0, GetNumberOfSuspend());
    State expect = kPause;
    return state_.compare_exchange_strong(expect, kRunning);
}

void Scheduler::PostRunnableBalanced(Coroutine *co, bool now) {
    Machine *target = machine(0);
    int load = target == Machine::This() ? std::numeric_limits<int>::max() :
        target->GetNumberOfRunnable();
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

void Scheduler::PurgreCoroutine(Machine *m, Coroutine *co) {
    co->Dispose();
    n_live_coroutines_.fetch_sub(1);

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

int Scheduler::GetNumberOfSuspend() const {
    int n = 0;
    for (int i = 0; i < concurrency_; i++) {
        if (auto m = machine(i)) {
            switch (m->state()) {
                //case Machine::kIdle:
                case Machine::kDead:
                case Machine::kPanic:
                case Machine::kSuspend:
                    n++;
                    break;
                default:
                    break;
            }
        }
    }
    return n;
}

} // namespace lang

} // namespace mai
