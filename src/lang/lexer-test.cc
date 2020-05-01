#include "lang/lexer.h"
#include "lang/syntax-mock-test.h"
#include "base/arena-utils.h"
#include "base/arenas.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class LexerTest : public ::testing::Test {
public:
    LexerTest()
        : arena_(Env::Default()->GetLowLevelAllocator())
        , lexer_(&arena_, &feedback_) {}
    
    MockFeedback feedback_;
    base::StandaloneArena arena_;
    Lexer lexer_;
};

TEST_F(LexerTest, Sanity) {
    MockFile file("- -= + +=");
    lexer_.SwitchInputFile("demo.mai", &file);

    Token token = lexer_.Next();
    ASSERT_EQ(Token::kMinus, token.kind());
    ASSERT_EQ(1, token.source_location().begin_line);
    ASSERT_EQ(1, token.source_location().begin_row);

    token = lexer_.Next();
    ASSERT_EQ(Token::kMinusEqual, token.kind());
    ASSERT_EQ(1, token.source_location().begin_line);
    ASSERT_EQ(3, token.source_location().begin_row);
    ASSERT_EQ(1, token.source_location().end_line);
    ASSERT_EQ(5, token.source_location().end_row);

    token = lexer_.Next();
    ASSERT_EQ(Token::kPlus, token.kind());
    ASSERT_EQ(1, token.source_location().begin_line);
    ASSERT_EQ(6, token.source_location().begin_row);

    token = lexer_.Next();
    ASSERT_EQ(Token::kPlusEqual, token.kind());
    ASSERT_EQ(1, token.source_location().begin_line);
    ASSERT_EQ(8, token.source_location().begin_row);
    ASSERT_EQ(1, token.source_location().end_line);
    ASSERT_EQ(10, token.source_location().end_row);
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kEOF, token.kind());
    
    ASSERT_EQ(Token::kNil, Token::IsKeyword("nil"));
}

TEST_F(LexerTest, String) {
    MockFile file("\" aaavvv \"");
    lexer_.SwitchInputFile("demo.mai", &file);
    
    Token token = lexer_.Next();
    ASSERT_EQ(Token::kStringLine, token.kind());
    EXPECT_EQ(" aaavvv ", token.text_val()->ToString());
    EXPECT_EQ(1, token.source_location().begin_line);
    EXPECT_EQ(1, token.source_location().begin_row);
    EXPECT_EQ(1, token.source_location().end_line);
    EXPECT_EQ(10, token.source_location().end_row);
}

TEST_F(LexerTest, StringTemplate) {
    MockFile file("\"a=$bbc b=$ccd\" + -");
    lexer_.SwitchInputFile("demo.mai", &file);
    
    Token token = lexer_.Next();
    ASSERT_EQ(Token::kStringTempletePrefix, token.kind());
    EXPECT_EQ("a=", token.text_val()->ToString());
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kIdentifier, token.kind());
    EXPECT_EQ("bbc", token.text_val()->ToString());
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kStringTempletePart, token.kind());
    EXPECT_EQ(" b=", token.text_val()->ToString());
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kIdentifier, token.kind());
    EXPECT_EQ("ccd", token.text_val()->ToString());
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kStringTempleteSuffix, token.kind());
    EXPECT_EQ("", token.text_val()->ToString());
    
    token = lexer_.Next();
    EXPECT_EQ(Token::kPlus, token.kind());
    
    token = lexer_.Next();
    EXPECT_EQ(Token::kMinus, token.kind());
    
    token = lexer_.Next();
    EXPECT_EQ(Token::kEOF, token.kind());
}

TEST_F(LexerTest, StringTemplateExpression) {
    MockFile file("\"a=${b + c}km\"");
    lexer_.SwitchInputFile("demo.mai", &file);
    
    Token token = lexer_.Next();
    ASSERT_EQ(Token::kStringTempletePrefix, token.kind());
    EXPECT_EQ("a=", token.text_val()->ToString());
    EXPECT_EQ(1, token.source_location().begin_line);
    EXPECT_EQ(1, token.source_location().begin_row);
    EXPECT_EQ(1, token.source_location().end_line);
    EXPECT_EQ(4, token.source_location().end_row);
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kStringTempleteExpressBegin, token.kind());
    EXPECT_EQ(1, token.source_location().begin_line);
    EXPECT_EQ(4, token.source_location().begin_row);
    EXPECT_EQ(1, token.source_location().end_line);
    EXPECT_EQ(5, token.source_location().end_row);
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kIdentifier, token.kind());
    EXPECT_EQ("b", token.text_val()->ToString());
    EXPECT_EQ(1, token.source_location().begin_line);
    EXPECT_EQ(6, token.source_location().begin_row);
    EXPECT_EQ(1, token.source_location().end_line);
    EXPECT_EQ(6, token.source_location().end_row);
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kPlus, token.kind());
    EXPECT_EQ(1, token.source_location().begin_line);
    EXPECT_EQ(8, token.source_location().begin_row);
    EXPECT_EQ(1, token.source_location().end_line);
    EXPECT_EQ(8, token.source_location().end_row);
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kIdentifier, token.kind());
    EXPECT_EQ("c", token.text_val()->ToString());
    EXPECT_EQ(1, token.source_location().begin_line);
    EXPECT_EQ(10, token.source_location().begin_row);
    EXPECT_EQ(1, token.source_location().end_line);
    EXPECT_EQ(10, token.source_location().end_row);
    
    token = lexer_.Next();
    EXPECT_EQ(Token::kStringTempleteExpressEnd, token.kind());
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kStringTempleteSuffix, token.kind());
    EXPECT_EQ("km", token.text_val()->ToString());
    
    token = lexer_.Next();
    EXPECT_EQ(Token::kEOF, token.kind());
}

TEST_F(LexerTest, StringTemplateContinuous) {
    MockFile file("\"$a${b - c}$b\"");
    lexer_.SwitchInputFile("demo.mai", &file);
    
    Token token = lexer_.Next();
    ASSERT_EQ(Token::kStringTempletePrefix, token.kind());
    EXPECT_EQ("", token.text_val()->ToString());

    token = lexer_.Next();
    ASSERT_EQ(Token::kIdentifier, token.kind());
    EXPECT_EQ("a", token.text_val()->ToString());
    
    token = lexer_.Next();
    EXPECT_EQ(Token::kStringTempleteExpressBegin, token.kind());
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kIdentifier, token.kind());
    EXPECT_EQ("b", token.text_val()->ToString());
    
    token = lexer_.Next();
    EXPECT_EQ(Token::kMinus, token.kind());

    token = lexer_.Next();
    ASSERT_EQ(Token::kIdentifier, token.kind());
    EXPECT_EQ("c", token.text_val()->ToString());
    
    token = lexer_.Next();
    EXPECT_EQ(Token::kStringTempleteExpressEnd, token.kind());
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kIdentifier, token.kind());
    EXPECT_EQ("b", token.text_val()->ToString());
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kStringTempleteSuffix, token.kind());
    EXPECT_EQ("", token.text_val()->ToString());
    
    token = lexer_.Next();
    EXPECT_EQ(Token::kEOF, token.kind());
}

TEST_F(LexerTest, Keywords) {
    MockFile file("nil true false i8");
    lexer_.SwitchInputFile("demo.mai", &file);
    
    Token token = lexer_.Next();
    EXPECT_EQ(Token::kNil, token.kind());
    
    token = lexer_.Next();
    EXPECT_EQ(Token::kTrue, token.kind());
    
    token = lexer_.Next();
    EXPECT_EQ(Token::kFalse, token.kind());
    
    token = lexer_.Next();
    EXPECT_EQ(Token::kI8, token.kind());
    
    token = lexer_.Next();
    EXPECT_EQ(Token::kEOF, token.kind());
}

TEST_F(LexerTest, StringEscapeCharacter) {
    MockFile file1("\"\\r\\n\\t\\$\"");
    lexer_.SwitchInputFile("demo.mai", &file1);

    Token token = lexer_.Next();
    ASSERT_EQ(Token::kStringLine, token.kind());
    EXPECT_EQ("\r\n\t$", token.text_val()->ToString());
    
    MockFile file2("\"\\x00\\x01\"");
    lexer_.SwitchInputFile("demo.mai", &file2);
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kStringLine, token.kind());
    EXPECT_EQ(2, token.text_val()->size());
    EXPECT_EQ(0, token.text_val()->data()[0]);
    EXPECT_EQ(1, token.text_val()->data()[1]);
}

TEST_F(LexerTest, StringUnescapeCharacter) {
    MockFile file("'\\r\\n\\t$\\x00\\x01'");
    lexer_.SwitchInputFile("demo.mai", &file);
    
    Token token = lexer_.Next();
    ASSERT_EQ(Token::kStringLine, token.kind());
    EXPECT_EQ("\\r\\n\\t$\\x00\\x01", token.text_val()->ToString());
}

TEST_F(LexerTest, StringUtf8Character) {
    MockFile file1("\"汉字\" \"🤔\"");
    lexer_.SwitchInputFile("demo.mai", &file1);

    Token token = lexer_.Next();
    ASSERT_EQ(Token::kStringLine, token.kind());
    EXPECT_EQ("汉字", token.text_val()->ToString());
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kStringLine, token.kind());
    EXPECT_EQ("🤔", token.text_val()->ToString());
}

TEST_F(LexerTest, StringBlock) {
    MockFile file("`line1\nline2\nline3\n`");
    lexer_.SwitchInputFile("demo.mai", &file);
    
    Token token = lexer_.Next();
    ASSERT_EQ(Token::kStringLine, token.kind());
    EXPECT_EQ("line1\nline2\nline3\n", token.text_val()->ToString());
    EXPECT_EQ(1, token.source_location().begin_line);
    EXPECT_EQ(1, token.source_location().begin_row);
    EXPECT_EQ(4, token.source_location().end_line);
    EXPECT_EQ(1, token.source_location().end_row);
}

TEST_F(LexerTest, Utf8Identifier) {
    MockFile file("汉字=\"CN_zh\"");
    lexer_.SwitchInputFile("demo.mai", &file);

    Token token = lexer_.Next();
    ASSERT_EQ(Token::kIdentifier, token.kind());
    EXPECT_EQ("汉字", token.text_val()->ToString());
    
    token = lexer_.Next();
    EXPECT_EQ(Token::kAssign, token.kind());
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kStringLine, token.kind());
    EXPECT_EQ("CN_zh", token.text_val()->ToString());
}

TEST_F(LexerTest, NumberLiteral) {
    MockFile file("1 0 -1 0.0");
    lexer_.SwitchInputFile("demo.mai", &file);
    
    Token token = lexer_.Next();
    ASSERT_EQ(Token::kIntVal, token.kind());
    EXPECT_EQ(1, token.i32_val());
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kIntVal, token.kind());
    EXPECT_EQ(0, token.i32_val());
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kIntVal, token.kind());
    EXPECT_EQ(-1, token.i32_val());
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kF32Val, token.kind());
    EXPECT_EQ(0, token.f32_val());
}

TEST_F(LexerTest, NumberLiteralPostfix) {
    MockFile file("255u8 -128i8 1i16 -1i16 65535u16 1u");
    lexer_.SwitchInputFile("demo.mai", &file);
    
    Token token = lexer_.Next();
    ASSERT_EQ(Token::kU8Val, token.kind());
    EXPECT_EQ(255, token.u32_val());
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kI8Val, token.kind());
    EXPECT_EQ(-128, token.i32_val());
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kI16Val, token.kind());
    EXPECT_EQ(1, token.i32_val());
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kI16Val, token.kind());
    EXPECT_EQ(-1, token.i32_val());
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kU16Val, token.kind());
    EXPECT_EQ(65535, token.u32_val());
    
    token = lexer_.Next();
    ASSERT_EQ(Token::kUIntVal, token.kind());
    EXPECT_EQ(1, token.u32_val());
}

}

}
