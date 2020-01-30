#include "lang/coroutine.h"
#include "lang/isolate-inl.h"
#include "lang/scheduler.h"
#include "lang/heap.h"
#include "lang/stack.h"

namespace mai {

namespace lang {

void Coroutine::Reinitialize(uint64_t coid, Closure *entry, Stack *stack) {
    // queue header:
    next_ = this;
    prev_ = this;

    coid_      = coid;
    state_     = kDead;
    entry_     = entry;
    owner_     = nullptr;
    exception_ = nullptr;
    caught_    = nullptr;
    yield_     = 0;

    new_guard0_ = __isolate->heap()->new_space()->original_chunk();
    new_guard1_ = __isolate->heap()->new_space()->original_limit();
    DCHECK_LT(new_guard0_, new_guard1_);

    sp_   = stack->stack_hi();
    bp_   = stack->stack_hi();
    acc_  = 0;
    facc_ = .0;

    // TODO:
    stack_ = DCHECK_NOTNULL(stack);
    stack_guard0_ = stack->guard0();
    stack_guard1_ = stack->guard1();
}

Coroutine::~Coroutine() {
    // Free stack
    __isolate->scheduler()->PurgreStack(stack_);
}

} // namespace lang

} // namespace mai
