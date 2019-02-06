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
    
} // namespace sql
    
} // namespace mai
