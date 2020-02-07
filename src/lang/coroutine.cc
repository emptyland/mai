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

const int32_t Coroutine::kOffsetState = MEMBER_OFFSET_OF(state_);
const int32_t Coroutine::kOffsetCaught = MEMBER_OFFSET_OF(caught_);
const int32_t Coroutine::kOffsetSysBP = MEMBER_OFFSET_OF(sys_bp_);
const int32_t Coroutine::kOffsetSysSP = MEMBER_OFFSET_OF(sys_sp_);
const int32_t Coroutine::kOffsetSysPC = MEMBER_OFFSET_OF(sys_pc_);
const int32_t Coroutine::kOffsetBP = MEMBER_OFFSET_OF(saved_state0_);
const int32_t Coroutine::kOffsetSP = kOffsetBP + kPointerSize;
const int32_t Coroutine::kOffsetPC = kOffsetSP + kPointerSize;
const int32_t Coroutine::kOffsetACC = kOffsetPC + kPointerSize;
const int32_t Coroutine::kOffsetFACC = kOffsetACC + kPointerSize;
const int32_t Coroutine::kOffsetBP1 = MEMBER_OFFSET_OF(saved_state1_);
const int32_t Coroutine::kOffsetSP1 = kOffsetBP1 + kPointerSize;
const int32_t Coroutine::kOffsetPC1 = kOffsetSP1 + kPointerSize;
const int32_t Coroutine::kOffsetYield = MEMBER_OFFSET_OF(yield_);
const int32_t Coroutine::kOffsetReentrant = MEMBER_OFFSET_OF(reentrant_);
const int32_t Coroutine::kOffsetEntry = MEMBER_OFFSET_OF(entry_);
const int32_t Coroutine::kOffsetException = MEMBER_OFFSET_OF(exception_);

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

    heap_guard0_ = STATE->heap()->new_space()->original_chunk();
    heap_guard1_ = STATE->heap()->new_space()->original_limit();
    DCHECK_LT(heap_guard0_, heap_guard1_);

    sys_bp_ = nullptr;
    sys_sp_ = nullptr;
    sys_pc_ = nullptr;
    saved_state0_[kSPIndex]   = bit_cast<intptr_t>(stack->stack_hi());
    saved_state0_[kBPIndex]   = bit_cast<intptr_t>(stack->stack_hi());
    saved_state0_[kACCIndex]  = 0;
    saved_state0_[kFACCIndex] = 0;

    // TODO:
    stack_ = DCHECK_NOTNULL(stack);
    stack_guard0_ = stack->guard0();
    stack_guard1_ = stack->guard1();
}

void Coroutine::Dispose() {
    DCHECK_EQ(kDead, state_);
    // Free stack
    if (stack_) {
        STATE->scheduler()->PurgreStack(stack_);
        stack_ = nullptr;
    }
}

void Coroutine::Uncaught(Any *expection) {
    TODO();
    // TODO:
}

void Coroutine::Suspend(intptr_t /*acc*/, double /*facc*/) {
    // TODO:
}

} // namespace lang

} // namespace mai
