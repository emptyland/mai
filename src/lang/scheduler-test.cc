#include "lang/scheduler.h"
#include "lang/value-inl.h"
#include "lang/heap.h"
#include "test/isolate-initializer.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class SchedulerTest : public test::IsolateInitializer {
public:
    // Dummy
};

TEST_F(SchedulerTest, Sanity) {
    ASSERT_NE(nullptr, STATE->scheduler());
    ASSERT_LT(0, STATE->scheduler()->concurrency());
}

TEST_F(SchedulerTest, NewStack) {
    auto scheduler = STATE->scheduler();
    
    auto stack = scheduler->NewStack(kDefaultStackSize);
    ASSERT_NE(nullptr, stack);
    EXPECT_EQ(stack->size(), scheduler->stack_pool_rss());
    EXPECT_EQ(1, scheduler->n_live_stacks());
    EXPECT_EQ(0x1400000, kDefaultStackSize);
    EXPECT_EQ(0x1401000, stack->size());
    EXPECT_EQ(0x1400cc0, stack->GetAvailableSize());
    EXPECT_EQ(0, reinterpret_cast<uintptr_t>(stack->stack_hi()) % kStackAligmentSize);
    EXPECT_EQ(0, reinterpret_cast<uintptr_t>(stack->stack_lo()) % kStackAligmentSize);
    EXPECT_LT(stack->guard0(), stack->stack_lo());
    EXPECT_GT(stack->guard1(), stack->stack_hi());

    scheduler->PurgreStack(stack);
    EXPECT_EQ(0x1401000, scheduler->stack_pool_rss());
    EXPECT_EQ(0, scheduler->n_live_stacks());
}

TEST_F(SchedulerTest, CacheStack) {
    auto scheduler = STATE->scheduler();

    size_t n = 0;
    Stack *stack = nullptr;
    do {
        stack = scheduler->NewStack(kDefaultStackSize);
        n++;
    } while (!scheduler->PurgreStack(stack));
    
    EXPECT_EQ(19, n);
    EXPECT_EQ(0, scheduler->n_live_stacks());
    EXPECT_EQ(0x16812000, scheduler->stack_pool_rss());
}

TEST_F(SchedulerTest, NewCoroutine) {
    auto scheduler = STATE->scheduler();
    auto heap = STATE->heap();
    EXPECT_EQ(0, scheduler->n_live_coroutines());
    
    auto co = scheduler->NewCoroutine(nullptr, false);
    ASSERT_NE(nullptr, co);
    EXPECT_EQ(Coroutine::kDead, co->state());
    EXPECT_EQ(0, co->coid());
    EXPECT_NE(nullptr, co->stack());
    EXPECT_EQ(co->stack()->guard0(), co->stack_guard0());
    EXPECT_EQ(co->stack()->guard1(), co->stack_guard1());
    EXPECT_EQ(co->bp(), co->stack()->stack_hi());
    EXPECT_EQ(co->sp(), co->stack()->stack_hi());
    EXPECT_EQ(heap->new_space()->original_chunk(), co->heap_guard0());
    EXPECT_EQ(heap->new_space()->original_limit(), co->heap_guard1());
    
    EXPECT_EQ(1, scheduler->n_live_coroutines());
    
    scheduler->PurgreCoroutine(co);
    EXPECT_EQ(0, scheduler->n_live_coroutines());
}

} // namespace lang

} // namespace mai
