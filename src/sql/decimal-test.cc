#include "sql/decimal.h"
#include "base/slice.h"
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
    
    for (int i = Decimal::kMinConstVal; i < Decimal::kMaxConstVal + 1; ++i) {
        EXPECT_EQ(base::Slice::Sprintf("%d", i), Decimal::GetConstant(i)->ToString());
    }
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
    EXPECT_EQ(0, d->segments_size());
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
    
    EXPECT_EQ(1, d->rdigital(8));
    EXPECT_EQ(2, d->rdigital(9));
    EXPECT_EQ(3, d->rdigital(10));
    EXPECT_EQ(0, d->rdigital(17));
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
    
TEST_F(DecimalTest, Compare) {
    auto lhs = Decimal::New(&arena_, "9999999999");
    auto rhs = Decimal::New(&arena_, "1");
    
    EXPECT_LT(0, lhs->AbsCompare(rhs));
    EXPECT_GT(0, rhs->AbsCompare(lhs));
    EXPECT_EQ(0, lhs->AbsCompare(lhs));
    EXPECT_EQ(0, rhs->AbsCompare(rhs));
    
    lhs = Decimal::New(&arena_, "1.01");
    rhs = Decimal::New(&arena_, "1.01");
    lhs = lhs->Extend(20, 10, &arena_);
    EXPECT_EQ(0, lhs->AbsCompare(rhs));
    EXPECT_EQ(0, rhs->AbsCompare(lhs));
    
    lhs = Decimal::New(&arena_, "1.01");
    rhs = Decimal::New(&arena_, "1.01001");
    EXPECT_GT(0, lhs->AbsCompare(rhs));
    EXPECT_LT(0, rhs->AbsCompare(lhs));
    
    lhs = Decimal::New(&arena_, "101001");
    rhs = Decimal::New(&arena_, "1.01001");
    EXPECT_LT(0, lhs->AbsCompare(rhs));
    EXPECT_GT(0, rhs->AbsCompare(lhs));
}

TEST_F(DecimalTest, Shl) {
    auto lhs = Decimal::New(&arena_, "9999999999");
    lhs->Shl(1);
    EXPECT_EQ("19999999998", lhs->ToString());
    
    lhs = Decimal::New(&arena_, "9999999999999999999");
    lhs->Shl(1);
    EXPECT_EQ("19999999999999999998", lhs->ToString());
    
    lhs = Decimal::New(&arena_, "9999999999999999999");
    lhs->Shl(2);
    EXPECT_EQ("39999999999999999996", lhs->ToString());
    
    lhs = Decimal::New(&arena_, "9999999999999999999");
    lhs->Shl(3);
    EXPECT_EQ("79999999999999999992", lhs->ToString());
    
    lhs = Decimal::New(&arena_, "9999999999999999999");
    lhs->Shl(10);
    EXPECT_EQ("10239999999999999998976", lhs->ToString());
    
    lhs = Decimal::New(&arena_, "9999999999999999999");
    lhs->Shl(20);
    EXPECT_EQ("10485759999999999998951424", lhs->ToString());
    
    lhs = Decimal::New(&arena_, "9999999999999999999");
    lhs->Shl(31);
    EXPECT_EQ("474836479999999997852516352", lhs->ToString());
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
    
    lhs = Decimal::New(&arena_, "0.1");
    rhs = Decimal::New(&arena_, "0.1");
    rv = lhs->Mul(rhs, &arena_);
    EXPECT_EQ("0.01", rv->ToString());
}
    
TEST_F(DecimalTest, SampleDiv) {
    auto lhs = Decimal::New(&arena_, "999999999");
    auto rhs = Decimal::kZero;
    
    Decimal *rv = nullptr, *rem = nullptr;
    // n / 0
    std::tie(rv, rem) = lhs->Div(rhs, &arena_);
    EXPECT_EQ(nullptr, rv);
    EXPECT_EQ(nullptr, rem);
    
    lhs = Decimal::kZero;
    rhs = Decimal::kOne;
    // 0 / n
    std::tie(rv, rem) = lhs->Div(rhs, &arena_);
    EXPECT_EQ("0", rv->ToString());
    EXPECT_EQ("0", rem->ToString());
    
    // m / n (n > m)
    lhs = Decimal::New(&arena_, "999999999");
    rhs = Decimal::New(&arena_, "9999999999");
    //lhs = lhs->ExtendFrom(rhs, &arena_);
    std::tie(rv, rem) = lhs->Div(rhs, &arena_);
    EXPECT_EQ("0", rv->ToString());
    EXPECT_EQ("999999999", rem->ToString());
    
    // n / n = 1
    std::tie(rv, rem) = lhs->Div(lhs, &arena_);
    EXPECT_EQ("1", rv->ToString());
    EXPECT_EQ("0", rem->ToString());
    
    lhs = Decimal::New(&arena_, "-999999999");
    rhs = Decimal::New(&arena_,  "999999999");
    // -n / n = -1
    std::tie(rv, rem) = lhs->Div(rhs, &arena_);
    EXPECT_EQ("-1", rv->ToString());
    EXPECT_EQ("0", rem->ToString());
}
    
TEST_F(DecimalTest, DivWord) {
    auto lhs = Decimal::New(&arena_, "999999999");
    auto rhs = Decimal::New(&arena_, "9");
    
    Decimal *rv = nullptr, *rem = nullptr;
    std::tie(rv, rem) = lhs->Div(rhs, &arena_);
    EXPECT_EQ("111111111", rv->ToString());
    EXPECT_EQ("0", rem->ToString());
    
    lhs = Decimal::New(&arena_, "9999999999");
    EXPECT_EQ(9, lhs->segment(0));
    EXPECT_EQ(999999999, lhs->segment(1));
    
    rhs = Decimal::New(&arena_, "11");
    std::tie(rv, rem) = lhs->Div(rhs, &arena_);
    EXPECT_EQ("909090909", rv->ToString());
    EXPECT_EQ("0", rem->ToString());
    
    lhs = Decimal::New(&arena_, "1000000000");
    rhs = Decimal::New(&arena_, "3");
    std::tie(rv, rem) = lhs->Div(rhs, &arena_);
    EXPECT_EQ("333333333", rv->ToString());
    EXPECT_EQ("1", rem->ToString());
    
    lhs = Decimal::New(&arena_, "0.001");
    rhs = Decimal::New(&arena_, "0.1");
    std::tie(rv, rem) = lhs->Div(rhs, &arena_);
    EXPECT_EQ("0.01", rv->ToString());
    EXPECT_EQ("0", rem->ToString());
    
    std::tie(rv, rem) = rhs->Div(lhs, &arena_);
    EXPECT_EQ("100", rv->ToString());
    EXPECT_EQ("0", rem->ToString());
    
    lhs = Decimal::New(&arena_, "0.100");   // 1 3
    EXPECT_EQ(1, lhs->valid_exp());
    rhs = Decimal::New(&arena_, "0.001"); // 3 3 delta_exp = -2
    EXPECT_EQ(3, rhs->valid_exp());
    std::tie(rv, rem) = lhs->Div(rhs, &arena_);
    EXPECT_EQ("100", rv->ToString());
    EXPECT_EQ("0", rem->ToString());
    
    // 17777777777
    lhs = Decimal::New(&arena_, "17777777777");
    rhs = Decimal::New(&arena_, "11");
    std::tie(rv, rem) = lhs->Div(rhs, &arena_);
    EXPECT_EQ("1616161616", rv->ToString());
    EXPECT_EQ("1", rem->ToString());
}
    
} // namespace sql
    
} // namespace mai