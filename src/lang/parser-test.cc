#include "lang/parser.h"
#include "lang/lexer.h"
#include "lang/ast.h"
#include "lang/syntax-mock-test.h"
#include "base/arena-utils.h"
#include "base/arenas.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class ParserTest : public ::testing::Test {
public:
    ParserTest()
        : arena_(Env::Default()->GetLowLevelAllocator())
        , parser_(&arena_, &feedback_) {}
    
    void SetUp() override {
        InitializeSyntaxLibrary();
    }
    
    void TearDown() override {
        FreeSyntaxLibrary();
    }
    
    MockFeedback feedback_;
    base::StandaloneArena arena_;
    Parser parser_;
};

TEST_F(ParserTest, Sanity) {
    MockFile file("package demo");
    parser_.SwitchInputFile("demos/demo.mai", &file);
    
    bool ok = true;
    FileUnit *ast = parser_.Parse(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_EQ("demos/demo.mai", ast->file_name()->ToString());
    ASSERT_EQ("demo", ast->package_name()->ToString());
}

TEST_F(ParserTest, Import) {
    MockFile file("package demo\n"
                  "import testing.foo.bar.baz\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    FileUnit *ast = parser_.Parse(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_EQ(1, ast->import_packages().size());
    auto stmt = ast->import_packages()[0];
    ASSERT_EQ("testing.foo.bar.baz", stmt->original_package_name()->ToString());
    ASSERT_EQ(nullptr, stmt->alias());
}

TEST_F(ParserTest, ImportBlock) {
    MockFile file("package demo\n"
                  "import {\n"
                  "    testing.foo as Foo\n"
                  "    testing.bar as Bar\n"
                  "}\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    FileUnit *ast = parser_.Parse(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_EQ(2, ast->import_packages().size());
    auto stmt = ast->import_packages()[0];
    ASSERT_EQ("testing.foo", stmt->original_package_name()->ToString());
    ASSERT_EQ("Foo", stmt->alias()->ToString());
    
    stmt = ast->import_packages()[1];
    ASSERT_EQ("testing.bar", stmt->original_package_name()->ToString());
    ASSERT_EQ("Bar", stmt->alias()->ToString());
}

TEST_F(ParserTest, ValDeclaration) {
    MockFile file("var a: int\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseVariableDeclaration(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);

    ASSERT_STREQ("a", ast->identifier()->data());
    ASSERT_EQ(VariableDeclaration::VAR, ast->kind());
    ASSERT_EQ(Token::kInt, ast->type()->id());

    MockFile file1("val a = 1\n");
    parser_.SwitchInputFile("demos/demo.mai", &file1);
    // var a: func (int, int): void
    ast = parser_.ParseVariableDeclaration(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);

    ASSERT_STREQ("a", ast->identifier()->data());
    ASSERT_EQ(VariableDeclaration::VAL, ast->kind());
    ASSERT_EQ(nullptr, ast->type());
    ASSERT_NE(nullptr, ast->initializer());
    ASSERT_TRUE(ast->initializer()->IsIntLiteral());
    ASSERT_EQ(1, ast->initializer()->AsIntLiteral()->value());
}

TEST_F(ParserTest, FunctionTypeSign) {
    MockFile file("fun (int, ...): string\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseTypeSign(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);

    auto proto = ast->prototype();
    ASSERT_TRUE(proto->vargs());
    ASSERT_EQ(Token::kString, proto->return_type()->id());
    ASSERT_EQ(1, proto->parameters_size());
    ASSERT_EQ(Token::kInt, proto->parameter(0).type->id());
}

TEST_F(ParserTest, PairExpression) {
    MockFile file("(a <- b)\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseExpression(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    auto pair = ast->AsPairExpression();
    ASSERT_NE(nullptr, pair);
    ASSERT_FALSE(pair->addition_key());
    ASSERT_STREQ("a", pair->key()->AsIdentifier()->name()->data());
    ASSERT_STREQ("b", pair->value()->AsIdentifier()->name()->data());
    
    MockFile file1("(.. <- b)\n");
    parser_.SwitchInputFile("demos/demo.mai", &file1);

    ast = parser_.ParseExpression(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    pair = ast->AsPairExpression();
    ASSERT_NE(nullptr, pair);
    ASSERT_TRUE(pair->addition_key());
    ASSERT_STREQ("b", pair->value()->AsIdentifier()->name()->data());
}

TEST_F(ParserTest, ParenExpression) {
    MockFile file("c * (a + b)\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseExpression(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    auto bin = ast->AsBinaryExpression();
    ASSERT_NE(nullptr, bin);
    ASSERT_STREQ("c", bin->lhs()->AsIdentifier()->name()->data());
    ASSERT_EQ(Operator::kMul, bin->op().kind);
    
    bin = bin->rhs()->AsBinaryExpression();
    ASSERT_NE(nullptr, bin);
    ASSERT_STREQ("a", bin->lhs()->AsIdentifier()->name()->data());
    ASSERT_STREQ("b", bin->rhs()->AsIdentifier()->name()->data());
    ASSERT_EQ(Operator::kAdd, bin->op().kind);
}

}

}
