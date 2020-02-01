#include "lang/coroutine.h"
#include "lang/isolate-inl.h"
#include "lang/scheduler.h"
#include "lang/heap.h"
#include "lang/stack.h"
#include "asm/utils.h"

namespace mai {

namespace lang {

#define MEMBER_OFFSET_OF(field) \
    arch::ObjectTemplate<Coroutine, int32_t>::OffsetOf(&Coroutine :: field)

const int32_t Coroutine::kOffsetSysBP = MEMBER_OFFSET_OF(sys_bp_);
const int32_t Coroutine::kOffsetSysSP = MEMBER_OFFSET_OF(sys_sp_);
const int32_t Coroutine::kOffsetSysPC = MEMBER_OFFSET_OF(sys_pc_);
const int32_t Coroutine::kOffsetBP = MEMBER_OFFSET_OF(bp_);
const int32_t Coroutine::kOffsetSP = MEMBER_OFFSET_OF(sp_);
const int32_t Coroutine::kOffsetPC = MEMBER_OFFSET_OF(pc_);
const int32_t Coroutine::kOffsetYield = MEMBER_OFFSET_OF(yield_);
const int32_t Coroutine::kOffsetReentrant = MEMBER_OFFSET_OF(reentrant_);
const int32_t Coroutine::kOffsetEntry = MEMBER_OFFSET_OF(entry_);

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
    reentrant_ = 0;

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

void Coroutine::Dispose() {
    DCHECK_EQ(kDead, state_);
    // Free stack
    if (stack_) {
        __isolate->scheduler()->PurgreStack(stack_);
        stack_ = nullptr;
    }
}

} // namespace lang

} // namespace mai
