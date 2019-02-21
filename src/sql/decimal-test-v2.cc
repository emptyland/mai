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
    
TEST_F(DecimalV2Test, AbsCompare) {
    auto lhs = Decimal::NewU64(0, &arena_);
    auto rhs = Decimal::NewU64(1, &arena_);
    
    ASSERT_EQ(0, Decimal::AbsCompare(lhs, lhs));
    ASSERT_EQ(0, Decimal::AbsCompare(rhs, rhs));
    ASSERT_GT(0, Decimal::AbsCompare(lhs, rhs));
    ASSERT_LT(0, Decimal::AbsCompare(rhs, lhs));
    
    rhs = Decimal::NewU64(0x800000000, &arena_);
    ASSERT_EQ(2, rhs->segments_size());
    ASSERT_EQ(0, Decimal::AbsCompare(rhs, rhs));
    ASSERT_GT(0, Decimal::AbsCompare(lhs, rhs));
    ASSERT_LT(0, Decimal::AbsCompare(rhs, lhs));
}
    
TEST_F(DecimalV2Test, Shl) {
    auto lhs = Decimal::NewU64(1, &arena_);
    auto rv = lhs->Shl(8, &arena_);
    EXPECT_EQ("256", rv->ToString());
    
    lhs = Decimal::NewU64(0x80000000, &arena_);
    rv = lhs->Shl(1, &arena_);
    EXPECT_EQ("4294967296", rv->ToString());
}
    
TEST_F(DecimalV2Test, Shr) {
    auto lhs = Decimal::NewU64(256, &arena_);
    auto rv = lhs->Shr(8, &arena_);
    EXPECT_EQ("1", rv->ToString());
    
    lhs = Decimal::NewU64(0x100000000ull, &arena_);
    rv = lhs->Shr(1, &arena_);
    EXPECT_EQ("2147483648", rv->ToString());
    
    uint32_t n1[] = {0x80000000, 0, 0};
    lhs = Decimal::NewCopied(MakeView(n1, 3), &arena_);
    EXPECT_EQ("39614081257132168796771975168", lhs->ToString());
    rv = lhs->Shr(33, &arena_);
    EXPECT_EQ("4611686018427387904", rv->ToString());
}
    
TEST_F(DecimalV2Test, Add) {
    static const uint32_t n1[] = {1, 0xffffffff, 0xffffffff};
    auto lhs = Decimal::NewCopied(MakeView(n1, 3), &arena_);
    EXPECT_EQ("36893488147419103231", lhs->ToString());
    
    auto rhs = Decimal::NewU64(1, &arena_);
    auto rv = lhs->Add(rhs, &arena_);
    EXPECT_EQ("36893488147419103232", rv->ToString());
    
    rv = lhs->Add(lhs, &arena_);
    EXPECT_EQ("73786976294838206462", rv->ToString());
    
    static const uint32_t n2[] = {0x80000000, 0xffffffff, 0xffffffff};
    lhs = Decimal::NewCopied(MakeView(n2, 3), &arena_);
    EXPECT_EQ("39614081275578912870481526783", lhs->ToString());
    
    static const uint32_t n3[] = {0x77777777, 0x80000000, 0xffffffff, 0xffffffff};
    rhs = Decimal::NewCopied(MakeView(n3, 4), &arena_);
    EXPECT_EQ("158798437899078888385163705461652848639", rhs->ToString());
    
    rv = lhs->Add(rhs, &arena_);
    EXPECT_EQ("158798437938692969660742618332134375422", rv->ToString());
}
    
} // namespace v2
    
} // namespace sql
    
} // namespace mai
