#include "lang/token.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

TEST(TokenTest, Sanity) {
    Token token(Token::kNil, {1,1,1,3});
    ASSERT_EQ(Token::kNil, token.kind());
    ASSERT_EQ(1, token.source_location().begin_line);
    ASSERT_EQ(1, token.source_location().begin_row);
    ASSERT_EQ(1, token.source_location().end_line);
    ASSERT_EQ(3, token.source_location().end_row);
    ASSERT_EQ(0, token.f32_val());
}

TEST(TokenTest, WithI32) {
    Token token = Token(Token::kI32Val, {}).With(100);
    ASSERT_EQ(Token::kI32Val, token.kind());
    ASSERT_EQ(100, token.i32_val());
}

}

}
