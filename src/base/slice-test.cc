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
}
    
} // namespace base
    
} // namespace mai
