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
    
TEST_F(DecimalTest, Point) {
    auto d = Decimal::New(&arena_, "12.3");
    EXPECT_EQ("12.3", d->ToString());
    EXPECT_EQ(1, d->exp());
    
    d = Decimal::New(&arena_, "123456789.3");
    EXPECT_EQ(2, d->segment_size());
    EXPECT_EQ("123456789.3", d->ToString());
    EXPECT_EQ(1, d->exp());
    
    d = Decimal::New(&arena_, "1.0000000001");
    EXPECT_EQ(2, d->segment_size());
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
    EXPECT_EQ(1, d->segment_size());
    EXPECT_EQ("0", d->ToString());
    
    d = Decimal::NewI64(&arena_, -1);
    EXPECT_EQ(1, d->segment_size());
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
    
} // namespace sql
    
} // namespace mai
