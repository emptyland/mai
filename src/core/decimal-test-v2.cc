#include "core/decimal-v2.h"
#include "base/slice.h"
#include "base/arenas.h"
#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace core {
    
namespace v2 {

class DecimalV2Test : public ::testing::Test {
public:
    DecimalV2Test() {}
    
    Env *env_ = Env::Default();
    base::StandaloneArena arena_;
};
    
static const double kAbsErr = 1e-7f;

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
    
    d = Decimal::NewU64(0xfff, &arena_);
    ASSERT_EQ("fff", d->ToString(16));
    ASSERT_EQ("4095", d->ToString(10));
    ASSERT_EQ("7777", d->ToString(8));
    ASSERT_EQ("111111111111", d->ToString(2));
}
    
TEST_F(DecimalV2Test, NewParsed) {
    auto d = Decimal::NewParsed("0xffff", &arena_);
    ASSERT_EQ("ffff", d->ToString(16));
    ASSERT_EQ("65535", d->ToString(10));
    ASSERT_EQ("177777", d->ToString(8));
    ASSERT_EQ("1111111111111111", d->ToString(2));
    
    d = Decimal::NewParsed("0x1234567890abcdef01234567890", &arena_);
    ASSERT_NE(nullptr, d);
    ASSERT_EQ(4, d->segments_size());
    ASSERT_EQ("1234567890abcdef01234567890", d->ToString(16));
    
    d = Decimal::NewParsed("1e-7", &arena_);
    ASSERT_NE(nullptr, d);
    ASSERT_EQ("0.0000001", d->ToString());
    
    d = Decimal::NewParsed("1e+7", &arena_);
    ASSERT_NE(nullptr, d);
    ASSERT_EQ("10000000", d->ToString());
    
    d = Decimal::NewParsed("-1.075e+7", &arena_);
    ASSERT_NE(nullptr, d);
    ASSERT_EQ("-10750000", d->ToString());
    
    d = Decimal::NewParsed("1.075e2", &arena_);
    ASSERT_NE(nullptr, d);
    ASSERT_EQ("107.5", d->ToString());
}

TEST_F(DecimalV2Test, NewParsedOctal) {
    auto d = Decimal::NewParsed("0777", &arena_);
    ASSERT_NE(nullptr, d);
    EXPECT_EQ(1, d->segments_size());
    EXPECT_EQ("511", d->ToString(10));
    EXPECT_EQ("777", d->ToString(8));
    
    d = Decimal::NewParsed("077777777", &arena_);
    ASSERT_NE(nullptr, d);
    EXPECT_EQ(1, d->segments_size());
    EXPECT_EQ("16777215", d->ToString(10));
    EXPECT_EQ("77777777", d->ToString(8));
    
    d = Decimal::NewParsed("0777777777777", &arena_);
    ASSERT_NE(nullptr, d);
    EXPECT_EQ(2, d->segments_size());
    EXPECT_EQ("68719476735", d->ToString(10));
    EXPECT_EQ("777777777777", d->ToString(8));
    
    d = Decimal::NewParsed("07777777777777777777777", &arena_);
    ASSERT_NE(nullptr, d);
    EXPECT_EQ(3, d->segments_size());
    EXPECT_EQ("73786976294838206463", d->ToString(10));
    EXPECT_EQ("7777777777777777777777", d->ToString(8));
    
    d = Decimal::NewParsed("012345670123456701234567", &arena_);
    ASSERT_NE(nullptr, d);
    EXPECT_EQ(3, d->segments_size());
    EXPECT_EQ("96374504495306324343", d->ToString(10));
    EXPECT_EQ("12345670123456701234567", d->ToString(8));
    
    d = Decimal::NewParsed("01223334444555556666667777777", &arena_);
    ASSERT_NE(nullptr, d);
    EXPECT_EQ(3, d->segments_size());
    EXPECT_EQ("3114073927131075058860031", d->ToString(10));
    EXPECT_EQ("1223334444555556666667777777", d->ToString(8));
}
    
TEST_F(DecimalV2Test, NewParsedDecimal) {
    auto d = Decimal::NewParsed("65535", &arena_);
    ASSERT_NE(nullptr, d);
    EXPECT_EQ(1, d->segments_size());
    EXPECT_EQ("65535", d->ToString(10));
    EXPECT_EQ("ffff", d->ToString(16));
    
    d = Decimal::NewParsed("99999999999", &arena_);
    ASSERT_NE(nullptr, d);
    EXPECT_EQ(2, d->segments_size());
    EXPECT_EQ("1011101001000011101101110011111111111", d->ToString(2));
    EXPECT_EQ("99999999999", d->ToString(10));
    EXPECT_EQ("174876e7ff", d->ToString(16));
    
    d = Decimal::NewParsed("-99999999999", &arena_);
    EXPECT_EQ("-99999999999", d->ToString(10));
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
    
TEST_F(DecimalV2Test, Pow10Consts) {
    ASSERT_EQ(1, Decimal::GetFastPow10(0)->segments_size());
    
    static const char *kz[] = {
        "1",
        "10",
        "100",
        "1000",
        "10000",
        "100000",
        "1000000",
        "10000000",
        "100000000",
        "1000000000",
        "10000000000",
        "100000000000",
        "1000000000000",
        "10000000000000",
        "100000000000000",
        "1000000000000000",
        "10000000000000000",
        "100000000000000000",
        "1000000000000000000",
        "10000000000000000000",
        "100000000000000000000",
        "1000000000000000000000",
        "10000000000000000000000",
        "100000000000000000000000",
        "1000000000000000000000000",
        "10000000000000000000000000",
        "100000000000000000000000000",
        "1000000000000000000000000000",
        "10000000000000000000000000000",
        "100000000000000000000000000000",
        "1000000000000000000000000000000",
    };
    
    for (int i = 0; i < Decimal::kMaxExp; ++i) {
        ASSERT_EQ(kz[i], Decimal::GetFastPow10(i)->ToString());
    }
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
    
    lhs = Decimal::NewParsed("0.5", &arena_);
    rhs = Decimal::NewParsed("0.5", &arena_);
    rv = lhs->Add(rhs, &arena_);
    EXPECT_EQ(1, rv->ToI64());
}
    
TEST_F(DecimalV2Test, AddSelf) {
    size_t cap = (Decimal::GetNumberOfBits(65, 10) + 31) / 32;
    auto lhs = Decimal::NewUninitialized(cap, &arena_);
    lhs->set_offset(cap);
    lhs->set_negative_and_exp(false, 0);
    
    
    auto rhs = Decimal::NewU64(UINT64_MAX, &arena_);
    rhs->set_negative_and_exp(false, 0);
    lhs->Add(rhs);
    EXPECT_EQ("18446744073709551615", lhs->ToString());
    
    lhs->Add(rhs);
    EXPECT_EQ("36893488147419103230", lhs->ToString());
    
    lhs->Add(rhs);
    EXPECT_EQ("55340232221128654845", lhs->ToString());
    
    lhs->Add(rhs);
    EXPECT_EQ("73786976294838206460", lhs->ToString());
}

TEST_F(DecimalV2Test, Sub) {
    static const uint32_t n1[] = {0x80000000, 0, 0};
    auto lhs = Decimal::NewCopied(MakeView(n1, 3), &arena_);
    EXPECT_EQ("39614081257132168796771975168", lhs->ToString());
    
    auto rhs = Decimal::NewU64(1, &arena_);
    auto rv = lhs->Sub(rhs, &arena_);
    EXPECT_EQ("39614081257132168796771975167", rv->ToString());
    
    static const uint32_t n2[] = {0x22222222, 0x22222222, 0x22222222};
    lhs = Decimal::NewCopied(MakeView(n2, 3), &arena_);
    
    static const uint32_t n3[] = {0x11111111, 0x11111111, 0x11111111};
    rhs = Decimal::NewCopied(MakeView(n3, 3), &arena_);
    
    rv = lhs->Sub(rhs, &arena_);
    EXPECT_EQ("5281877500950955839569596689", rv->ToString(10));
    EXPECT_EQ("111111111111111111111111", rv->ToString(16));
}
    
TEST_F(DecimalV2Test, Mul) {
    static const uint32_t n1[] = {0x80000000, 0, 0};
    auto lhs = Decimal::NewCopied(MakeView(n1, 3), &arena_);
    EXPECT_EQ("39614081257132168796771975168", lhs->ToString());
    
    auto rhs = Decimal::NewU64(1, &arena_);
    auto rv = lhs->Mul(rhs, &arena_);
    EXPECT_EQ("39614081257132168796771975168", rv->ToString());
    
    static const uint32_t n2[] = {0x22222222, 0x22222222, 0x22222222};
    rhs = Decimal::NewCopied(MakeView(n2, 3), &arena_);
    EXPECT_EQ("10563755001901911679139193378", rhs->ToString());
    rv = lhs->Mul(rhs, &arena_);
    EXPECT_EQ("418473449025778717589052628208562550239206073791366037504", rv->ToString());
    EXPECT_EQ("111111111111111111111111000000000000000000000000", rv->ToString(16));
    
    lhs = Decimal::NewU64(7, &arena_);
    rhs = Decimal::NewU64(8, &arena_);
    rv = lhs->Mul(rhs, &arena_);
    EXPECT_EQ("56", rv->ToString());
}
    
TEST_F(DecimalV2Test, SimpleDiv) {
    static const uint32_t n1[] = {0x80000000, 0, 0};
    auto lhs = Decimal::NewCopied(MakeView(n1, 3), &arena_);
    
    Decimal *rv, *re;
    std::tie(rv, re) = lhs->Div(lhs, &arena_);
    ASSERT_EQ(1, rv->ToI64());
    ASSERT_EQ(0, re->ToI64());
    
    lhs->set_negative(true);
    std::tie(rv, re) = lhs->Div(lhs, &arena_);
    ASSERT_EQ(1, rv->ToI64());
    ASSERT_EQ(0, re->ToI64());
    
    auto rhs = Decimal::NewCopied(MakeView(n1, 3), &arena_);
    std::tie(rv, re) = lhs->Div(rhs, &arena_);
    ASSERT_EQ(-1, rv->ToI64());
    ASSERT_EQ(0, re->ToI64());
    
    lhs = Decimal::NewI64(1, &arena_);
    std::tie(rv, re) = lhs->Div(rhs, &arena_);
    ASSERT_EQ(0, rv->ToI64());
    ASSERT_EQ(1, re->ToI64());
    
    rhs = Decimal::NewI64(0, &arena_);
    std::tie(rv, re) = lhs->Div(rhs, &arena_);
    ASSERT_EQ(nullptr, rv);
    ASSERT_EQ(nullptr, re);
}
    
TEST_F(DecimalV2Test, DivOneWord) {
    static const uint32_t n1[] = {0x80000000, 0, 0};
    auto lhs = Decimal::NewCopied(MakeView(n1, 3), &arena_);
    auto rhs = Decimal::NewI64(2, &arena_);
    Decimal *rv, *re;
    std::tie(rv, re) = lhs->Div(rhs, &arena_);
    ASSERT_EQ("400000000000000000000000", rv->ToString(16));
    
    rhs = Decimal::NewI64(1234567, &arena_);
    std::tie(rv, re) = lhs->Div(rhs, &arena_);
    ASSERT_EQ("32087429242100403458679", rv->ToString());
    
    lhs = Decimal::NewParsed("1001.000", &arena_);
    rhs = Decimal::NewParsed(   "0.100", &arena_);
    std::tie(rv, re) = lhs->Div(rhs, &arena_);
    ASSERT_EQ("10010", rv->ToString());
    
    rhs = Decimal::NewParsed(   "0.200", &arena_);
    std::tie(rv, re) = lhs->Div(rhs, &arena_);
    ASSERT_EQ("5005", rv->ToString());
}
    
TEST_F(DecimalV2Test, Div) {
    static const uint32_t n1[] = {0x80000000, 0, 0};
    auto lhs = Decimal::NewCopied(MakeView(n1, 3), &arena_);
    
    static const uint32_t n2[] = {1, 0};
    auto rhs = Decimal::NewCopied(MakeView(n2, 2), &arena_);
    Decimal *rv, *re;
    std::tie(rv, re) = lhs->Div(rhs, &arena_);
    EXPECT_EQ("8000000000000000", rv->ToString(16));
    EXPECT_EQ("0", re->ToString(16));

    lhs = Decimal::NewParsed("9999999999", &arena_);
    rhs = Decimal::NewParsed("9000000000", &arena_);
    std::tie(rv, re) = lhs->Div(rhs, &arena_);
    EXPECT_EQ("1", rv->ToString());
    EXPECT_EQ("999999999", re->ToString());
    
    lhs = Decimal::NewParsed("1234567890.000", &arena_);
    rhs = Decimal::NewParsed(         "0.001", &arena_);
    std::tie(rv, re) = lhs->Div(rhs, &arena_);
    EXPECT_EQ("1234567890000", rv->ToString());
    EXPECT_EQ("0", re->ToString());
    
    lhs = Decimal::NewParsed("1.0000000", &arena_);
    rhs = Decimal::NewParsed("3.0000000", &arena_);
    std::tie(rv, re) = lhs->Div(rhs, &arena_);
    EXPECT_EQ("0.3333333", rv->ToString());
    EXPECT_EQ("1", re->ToString());
}
    
TEST_F(DecimalV2Test, DivSelf) {
    static const char *kz[] = {
        "13204693752377389598923991722",
        "4401564584125796532974663907",
        "1467188194708598844324887969",
        "489062731569532948108295989",
        "163020910523177649369431996",
        "54340303507725883123143998",
        "18113434502575294374381332",
        "6037811500858431458127110",
        "2012603833619477152709036",
        "670867944539825717569678",
        "223622648179941905856559",
        "74540882726647301952186",
        "24846960908882433984062",
        "8282320302960811328020",
        "2760773434320270442673",
        "920257811440090147557",
        "306752603813363382519",
        "102250867937787794173",
        "34083622645929264724",
        "11361207548643088241",
        "3787069182881029413",
        "1262356394293676471",
        "420785464764558823",
        "140261821588186274",
        "46753940529395424",
        "15584646843131808",
        "5194882281043936",
        "1731627427014645",
        "577209142338215",
        "192403047446071",
        "64134349148690",
        "21378116382896",
        "7126038794298",
        "2375346264766",
        "791782088255",
        "263927362751",
        "87975787583",
        "29325262527",
        "9775087509",
        "3258362503",
        "1086120834",
        "362040278",
        "120680092",
        "40226697",
        "13408899",
        "4469633",
        "1489877",
        "496625",
        "165541",
        "55180",
        "18393",
        "6131",
        "2043",
        "681",
        "227",
        "75",
        "25",
        "8",
        "2",
        "0",
    };
    
    static const uint32_t n1[] = {0x80000000, 0, 0};
    auto lhs = Decimal::NewCopied(MakeView(n1, 3), &arena_);
    
    for (size_t i = 0; i < arraysize(kz); ++i) {
        lhs->Div(3ll);
        EXPECT_EQ(kz[i], lhs->ToString());
    }
}
    
TEST_F(DecimalV2Test, Sqrt) {
    auto lhs = Decimal::NewParsed("81", &arena_);
    auto rv = lhs->Sqrt(&arena_);
    EXPECT_EQ("9", rv->ToString());
    
    lhs = Decimal::NewParsed("8.1", &arena_);
    rv = lhs->Sqrt(&arena_);
    EXPECT_EQ("2.8", rv->ToString());
    
    lhs = Decimal::NewParsed("8.100", &arena_);
    rv = lhs->Sqrt(&arena_);
    EXPECT_EQ("2.84", rv->ToString());
    
    lhs = Decimal::NewParsed("8.1000", &arena_);
    rv = lhs->Sqrt(&arena_);
    EXPECT_EQ("2.84", rv->ToString());
    
    lhs = Decimal::NewParsed("8.1999", &arena_);
    rv = lhs->Sqrt(&arena_);
    EXPECT_EQ("2.86", rv->ToString());
    
    lhs = Decimal::NewParsed("1234567890123456789.00000000", &arena_);
    rv = lhs->Sqrt(&arena_);
    EXPECT_EQ("1111111106.1111", rv->ToString());
}
    
TEST_F(DecimalV2Test, ToPrimitiveNumber) {
    auto d = Decimal::NewParsed("0.1", &arena_);
    EXPECT_NEAR(0.1, d->ToF32(), kAbsErr);
    EXPECT_NEAR(0.1, d->ToF64(), kAbsErr);
    
    d->set_negative(true);
    EXPECT_NEAR(-0.1, d->ToF32(), kAbsErr);
    EXPECT_NEAR(-0.1, d->ToF64(), kAbsErr);
    
    d = Decimal::NewParsed("123456789.987654321", &arena_);
    EXPECT_NEAR(123456789.987654321f, d->ToF32(), kAbsErr);
    EXPECT_NEAR(123456789.987654321, d->ToF64(), kAbsErr);
    EXPECT_EQ(123456789, d->ToI64());
}
    
TEST_F(DecimalV2Test, FixedPointParsing) {
    auto d = Decimal::NewParsed("0.1", &arena_);
    ASSERT_EQ("0.1", d->ToString());
    
    d = Decimal::NewParsed("100.000", &arena_);
    ASSERT_EQ(3, d->exp());
    ASSERT_EQ("1000", d->scale()->ToString());
    ASSERT_EQ("100", d->ToString());
}
    
} // namespace v2
    
} // namespace core
    
} // namespace mai
