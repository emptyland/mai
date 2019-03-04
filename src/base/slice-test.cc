#include "base/slice.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace base {
    
TEST(SliceTest, LikeNumber) {
    EXPECT_EQ(0, Slice::LikeNumber("-"));
    EXPECT_EQ(0, Slice::LikeNumber("oo"));
    EXPECT_EQ('d', Slice::LikeNumber("1000"));
    EXPECT_EQ('d', Slice::LikeNumber("99"));
    EXPECT_EQ('d', Slice::LikeNumber("0"));
    
    EXPECT_EQ('s', Slice::LikeNumber("-1"));
    EXPECT_EQ('s', Slice::LikeNumber("-1999"));
    
    EXPECT_EQ('o', Slice::LikeNumber("0777"));
    EXPECT_EQ('o', Slice::LikeNumber("00"));
    
    EXPECT_EQ('h', Slice::LikeNumber("0x100"));
    EXPECT_EQ('h', Slice::LikeNumber("0xffe"));
    
    EXPECT_EQ('f', Slice::LikeNumber(".001"));
    EXPECT_EQ(0, Slice::LikeNumber("."));
    EXPECT_EQ('f', Slice::LikeNumber("-.1"));
    EXPECT_EQ('f', Slice::LikeNumber("+.1"));
    EXPECT_EQ('f', Slice::LikeNumber("1.000"));
    
    EXPECT_EQ('e', Slice::LikeNumber(".001e2"));
    EXPECT_EQ('e', Slice::LikeNumber("0e0"));
    EXPECT_EQ(0, Slice::LikeNumber("e"));
    EXPECT_EQ('e', Slice::LikeNumber("+.1e-1"));
    EXPECT_EQ('e', Slice::LikeNumber("1.000e+10"));
}

TEST(SliceTest, ParseI64) {
    int64_t val;
    EXPECT_EQ(0, Slice::ParseI64("0", &val));
    EXPECT_EQ(0, val);
    
    EXPECT_EQ(0, Slice::ParseI64("1", &val));
    EXPECT_EQ(1, val);
    
    EXPECT_EQ(0, Slice::ParseI64("-1", &val));
    EXPECT_EQ(-1, val);
    
    EXPECT_EQ(0, Slice::ParseI64("9223372036854775807", &val));
    EXPECT_EQ(INT64_MAX, val);
    
    EXPECT_EQ(1, Slice::ParseI64("9223372036854775808", &val));
    
    EXPECT_EQ(0, Slice::ParseI64("-9223372036854775807", &val));
    EXPECT_EQ(-9223372036854775807LL, val);
    
    EXPECT_EQ(0, Slice::ParseI64("-9223372036854775808", &val));
    EXPECT_EQ(INT64_MIN, val);
    
    EXPECT_EQ(1, Slice::ParseI64("-9223372036854775809", &val));
}
    
TEST(SliceTest, ParseI32) {
    int32_t val;
    EXPECT_EQ(0, Slice::ParseI32("0", &val));
    EXPECT_EQ(0, val);
    
    EXPECT_EQ(0, Slice::ParseI32("1", &val));
    EXPECT_EQ(1, val);
    
    EXPECT_EQ(0, Slice::ParseI32("-1", &val));
    EXPECT_EQ(-1, val);
    
    EXPECT_EQ(0, Slice::ParseI32("2147483647", &val));
    EXPECT_EQ(INT32_MAX, val);
    
    EXPECT_EQ(1, Slice::ParseI32("2147483648", &val));
    
    EXPECT_EQ(0, Slice::ParseI32("-2147483648", &val));
    EXPECT_EQ(INT32_MIN, val);
    
    EXPECT_EQ(1, Slice::ParseI32("-2147483649", &val));
    
    EXPECT_EQ(-1, Slice::ParseI32("-", &val));
}
    
TEST(SliceTest, ParseH32) {
    uint64_t val = 1;
    EXPECT_EQ(0, Slice::ParseH64("0x0", &val));
    EXPECT_EQ(0, val);
    
    EXPECT_EQ(0, Slice::ParseH64("0xff", &val));
    EXPECT_EQ(0xff, val);
}
    
} // namespace base
    
} // namespace mai
