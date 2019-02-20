#include "sql/decimal-v2.h"
#include "base/slice.h"
#include "base/standalone-arena.h"
#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace sql {
    
namespace v2 {


class DecimalV2Test : public ::testing::Test {
public:
    DecimalV2Test() : arena_(env_->GetLowLevelAllocator()) {}
    
    Env *env_ = Env::Default();
    base::StandaloneArena arena_;
};

TEST_F(DecimalV2Test, Sanity) {
    Decimal *d = Decimal::NewUninitialized(1, &arena_);
    ASSERT_EQ(0, d->offset());
    ASSERT_EQ(1, d->segments_size());
    ASSERT_EQ(1, d->segments_capacity());
    ASSERT_EQ(0, d->exp());
    ASSERT_TRUE(d->positive());
    ASSERT_FALSE(d->zero());
}
    
TEST_F(DecimalV2Test, ToString) {
    auto d = Decimal::NewU64(0, &arena_);
    ASSERT_TRUE(d->zero());
    ASSERT_EQ("0", d->ToString());
    
    d = Decimal::NewU64(1, &arena_);
    ASSERT_EQ(1, d->segment(0));
    ASSERT_FALSE(d->zero());
    ASSERT_EQ("1", d->ToString());
    
    d = Decimal::NewU64(12345678901234567890ull, &arena_);
    ASSERT_EQ("12345678901234567890", d->ToString());
}
    
} // namespace v2
    
} // namespace sql
    
} // namespace mai
