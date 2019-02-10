#include "sql/decimal.h"
#include "base/standalone-arena.h"
#include "mai/env.h"
#include "gtest/gtest.h"


namespace mai {
    
namespace sql {
    
class DecimalTest : public ::testing::Test {
public:
    DecimalTest() : arena_(env_->GetLowLevelAllocator()) {}
    
    Env *env_ = Env::Default();
    base::StandaloneArena arena_;
}; // class DecimalTest

TEST_F(DecimalTest, Sanity) {
    auto d = Decimal::New(&arena_, "123", 3);
    ASSERT_NE(nullptr, d);
    
    ASSERT_EQ("123", d->ToString());
    
    d = Decimal::New(&arena_, "0", 1);
    ASSERT_EQ("0", d->ToString());
}
    
TEST_F(DecimalTest, Constants) {
    EXPECT_EQ(Decimal::kHeaderSize, Decimal::kZero->size());
    
    EXPECT_EQ("0", Decimal::kZero->ToString());
    EXPECT_EQ("1", Decimal::kOne->ToString());
    EXPECT_EQ("-1", Decimal::kMinusOne->ToString());
}
    
TEST_F(DecimalTest, Point) {
    auto d = Decimal::New(&arena_, "12.3");
    EXPECT_EQ("12.3", d->ToString());
    EXPECT_EQ(1, d->exp());
    
    d = Decimal::New(&arena_, "123456789.3");
    EXPECT_EQ(2, d->segments_size());
    EXPECT_EQ("123456789.3", d->ToString());
    EXPECT_EQ(1, d->exp());
    
    d = Decimal::New(&arena_, "1.0000000001");
    EXPECT_EQ(2, d->segments_size());
    EXPECT_EQ("1.0000000001", d->ToString());
    EXPECT_EQ(10, d->exp());
}
    
TEST_F(DecimalTest, NewFromU64) {
    auto d = Decimal::NewU64(&arena_, 1234567890ull);
    EXPECT_EQ("1234567890", d->ToString());
    
    d = Decimal::NewU64(&arena_, 123456789123456789ull);
    EXPECT_EQ("123456789123456789", d->ToString());
    
    d = Decimal::NewU64(&arena_, 999999999999999999ull);
    EXPECT_EQ("999999999999999999", d->ToString());
}
    
TEST_F(DecimalTest, NewFromI64) {
    auto d = Decimal::NewI64(&arena_, 1234567890ll);
    EXPECT_EQ("1234567890", d->ToString());
    
    d = Decimal::NewI64(&arena_, -1234567890ll);
    EXPECT_EQ("-1234567890", d->ToString());
    
    d = Decimal::NewI64(&arena_, 0);
    EXPECT_EQ(1, d->segments_size());
    EXPECT_EQ("0", d->ToString());
    
    d = Decimal::NewI64(&arena_, -1);
    EXPECT_EQ(1, d->segments_size());
    EXPECT_EQ("-1", d->ToString());
}
    
TEST_F(DecimalTest, Digital) {
    auto d = Decimal::NewU64(&arena_, 1234567890ull);
    EXPECT_EQ(0, d->digital(0));
    EXPECT_EQ(9, d->digital(1));
    EXPECT_EQ(8, d->digital(2));
    
    EXPECT_EQ(1, d->digital(9));
}
    
TEST_F(DecimalTest, Shrink) {
    auto d = Decimal::New(&arena_, "123456789.987654321");
    EXPECT_EQ("123456789.987654321", d->ToString());
    d = d->Extend(12, 3, &arena_);
    EXPECT_EQ("123456789.987", d->ToString());
    d = d->Extend(9, 3, &arena_);
    EXPECT_EQ("456789.987", d->ToString());
    d = d->Extend(9, 0, &arena_);
    EXPECT_EQ("456789", d->ToString());
    d = d->Extend(1, 0, &arena_);
    EXPECT_EQ("9", d->ToString());
}
    
TEST_F(DecimalTest, Extend) {
    auto d = Decimal::New(&arena_, "789.987");
    EXPECT_EQ("789.987", d->ToString());
    
    d = d->Extend(12, 3, &arena_);
    EXPECT_EQ("789.987", d->ToString());
    
    d = d->Extend(65, 30, &arena_);
    EXPECT_EQ("789.987", d->ToString());
}
    
TEST_F(DecimalTest, Add) {
    auto lhs = Decimal::New(&arena_, "999999999");
    auto rhs = Decimal::New(&arena_, "1");
    lhs = lhs->Extend(10, 0, &arena_);
    rhs = rhs->ExtendFrom(lhs, &arena_);
    
    auto rv = lhs->Add(rhs, &arena_);
    EXPECT_EQ("1000000000", rv->ToString());
    
    rv = lhs->Add(lhs, &arena_);
    EXPECT_EQ("1999999998", rv->ToString());
    
    lhs = Decimal::New(&arena_, "-99999999");
    rv = lhs->Add(lhs, &arena_);
    EXPECT_EQ("-199999998", rv->ToString());
    
    lhs = Decimal::New(&arena_, "0.999999999");
    rhs = Decimal::New(&arena_, "0.000000001");
    lhs = lhs->Extend(65, 30, &arena_);
    rhs = rhs->ExtendFrom(lhs, &arena_);
    rv = lhs->Add(rhs, &arena_);
    EXPECT_EQ("1", rv->ToString());
}
    
TEST_F(DecimalTest, Sub) {
    auto lhs = Decimal::New(&arena_, "100000000");
    auto rhs = Decimal::New(&arena_, "1");
    rhs = rhs->ExtendFrom(lhs, &arena_);
    
    auto rv = lhs->Sub(rhs, &arena_);
    EXPECT_EQ("99999999", rv->ToString());
    
    lhs = Decimal::New(&arena_, "1000000000");
    rhs = Decimal::New(&arena_, "1");
    rhs = rhs->ExtendFrom(lhs, &arena_);
    rv = lhs->Sub(rhs, &arena_);
    EXPECT_EQ("999999999", rv->ToString());
    
    rv = lhs->Sub(lhs, &arena_);
    EXPECT_EQ("0", rv->ToString());
    
    lhs = Decimal::New(&arena_, "1");
    rhs = Decimal::New(&arena_, "1000000000");
    lhs = lhs->ExtendFrom(rhs, &arena_);
    rv = lhs->Sub(rhs, &arena_);
    EXPECT_EQ("-999999999", rv->ToString());
}
    
TEST_F(DecimalTest, Mul) {
    auto lhs = Decimal::New(&arena_, "999999999");
    auto rhs = Decimal::New(&arena_, "999999999");
    auto rv = lhs->Mul(rhs, &arena_);
    EXPECT_EQ("999999998000000001", rv->ToString());
    
    lhs = Decimal::New(&arena_, "9999999999");
    rhs = Decimal::New(&arena_, "9999999999");
    rv = lhs->Mul(rhs, &arena_);
    EXPECT_EQ("99999999980000000001", rv->ToString());
    
    lhs = Decimal::New(&arena_, "9999999999");
    rhs = Decimal::New(&arena_, "0");
    rv = lhs->Mul(rhs, &arena_);
    EXPECT_EQ("0", rv->ToString());
    
    lhs = Decimal::New(&arena_, "9999999999");
    rhs = Decimal::New(&arena_, "1");
    rv = lhs->Mul(rhs, &arena_);
    EXPECT_EQ("9999999999", rv->ToString());
    
    lhs = Decimal::New(&arena_, "123456789.987654321");
    rhs = Decimal::New(&arena_, "-11");
    rv = lhs->Mul(rhs, &arena_);
    EXPECT_EQ("-1358024689.864197531", rv->ToString());
    //         -1358024689.8641977
    
    
}
    
} // namespace sql
    
} // namespace mai
