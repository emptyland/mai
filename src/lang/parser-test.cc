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
    
    ASSERT_EQ(1, ast->import_packages_size());
    auto stmt = ast->import_package(0);
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
    
    ASSERT_EQ(2, ast->import_packages_size());
    auto stmt = ast->import_package(0);
    ASSERT_EQ("testing.foo", stmt->original_package_name()->ToString());
    ASSERT_EQ("Foo", stmt->alias()->ToString());
    
    stmt = ast->import_package(1);
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
    MockFile file("(a -> b)\n");
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
    
    MockFile file1("(.. -> b)\n");
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

TEST_F(ParserTest, IncrementExpression) {
    MockFile file("a + a++\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseExpression(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);

    auto bin = ast->AsBinaryExpression();
    ASSERT_NE(nullptr, bin);
    ASSERT_STREQ("a", bin->lhs()->AsIdentifier()->name()->data());
    ASSERT_EQ(Operator::kAdd, bin->op().kind);

    auto una = bin->rhs()->AsUnaryExpression();
    ASSERT_NE(nullptr, una);
    ASSERT_STREQ("a", una->operand()->AsIdentifier()->name()->data());
    ASSERT_EQ(Operator::kIncrementPost, una->op().kind);
}

TEST_F(ParserTest, AssignmentStatement) {
    MockFile file("a += (.. -> 100)\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseAssignmentOrExpression(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    auto ass = ast->AsAssignmentStatement();
    ASSERT_NE(nullptr, ass);
    
    ASSERT_EQ(Operator::kAdd, ass->assignment_op().kind);
    ASSERT_STREQ("a", ass->lval()->AsIdentifier()->name()->data());
    auto pair = ass->rval()->AsPairExpression();
    ASSERT_NE(nullptr, pair);
    ASSERT_TRUE(pair->addition_key());
    ASSERT_EQ(100, pair->value()->AsIntLiteral()->value());
}

TEST_F(ParserTest, NativeFunctionDeclaration) {
    MockFile file("native fun foo(a:int, b:int)\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseFunctionDefinition(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_TRUE(ast->native());
    ASSERT_STREQ("foo", ast->identifier()->data());
    ASSERT_EQ(2, ast->prototype()->parameters_size());
    ASSERT_STREQ("a", ast->prototype()->parameter(0).name->data());
    ASSERT_EQ(Token::kInt, ast->prototype()->parameter(0).type->id());
    ASSERT_STREQ("b", ast->prototype()->parameter(1).name->data());
    ASSERT_EQ(Token::kInt, ast->prototype()->parameter(1).type->id());
    ASSERT_EQ(Token::kVoid, ast->prototype()->return_type()->id());
}

TEST_F(ParserTest, FunctionDefinition) {
    MockFile file("fun foo(a:int, b:int) = a + b\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseFunctionDefinition(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_EQ(1, ast->statements_size());
    auto ret = ast->statement(0)->AsBreakableStatement();
    ASSERT_TRUE(ret->IsReturn());
    auto bin = ret->value()->AsBinaryExpression();
    ASSERT_NE(nullptr, bin);
    ASSERT_STREQ("a", bin->lhs()->AsIdentifier()->name()->data());
    ASSERT_STREQ("b", bin->rhs()->AsIdentifier()->name()->data());
}

TEST_F(ParserTest, ArrayInitializer) {
    MockFile file("array[int](100)\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);
    
    bool ok = true;
    auto ast = parser_.ParseArrayInitializer(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_FALSE(ast->mutable_container());
    ASSERT_EQ(Token::kInt, ast->element_type()->id());
    ASSERT_EQ(100, ast->reserve()->AsIntLiteral()->value());
    
    MockFile file1("mutable_array{\"ok\", \"no\", \"over\"}\n");
    parser_.SwitchInputFile("demos/demo.mai", &file1);
    
    ok = true;
    ast = parser_.ParseArrayInitializer(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_TRUE(ast->mutable_container());
    ASSERT_EQ(nullptr, ast->element_type());
    ASSERT_EQ(3, ast->operands_size());
    ASSERT_STREQ("ok", ast->operand(0)->AsStringLiteral()->value()->data());
    ASSERT_STREQ("no", ast->operand(1)->AsStringLiteral()->value()->data());
    ASSERT_STREQ("over", ast->operand(2)->AsStringLiteral()->value()->data());
}

TEST_F(ParserTest, MapInitializer) {
    MockFile file("map[string, int](100, 1.2)\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);
    
    bool ok = true;
    auto ast = parser_.ParseMapInitializer(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_FALSE(ast->mutable_container());
    ASSERT_EQ(Token::kString, ast->key_type()->id());
    ASSERT_EQ(Token::kInt, ast->value_type()->id());
    ASSERT_EQ(100, ast->reserve()->AsIntLiteral()->value());
    ASSERT_NEAR(1.2, ast->load_factor()->AsF32Literal()->value(), 0.01);
    
    MockFile file1("map{a+b->1, \"ok\"->a/b}\n");
    parser_.SwitchInputFile("demos/demo.mai", &file1);

    ok = true;
    ast = parser_.ParseMapInitializer(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);

    ASSERT_FALSE(ast->mutable_container());
    ASSERT_EQ(nullptr, ast->key_type());
    ASSERT_EQ(nullptr, ast->value_type());
    ASSERT_EQ(2, ast->operands_size());
    ASSERT_TRUE(ast->operand(0)->IsPairExpression());
    ASSERT_TRUE(ast->operand(1)->IsPairExpression());
}

TEST_F(ParserTest, ChannelRecvSend) {
    MockFile file("<- ch\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseExpression(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);

    auto unary = ast->AsUnaryExpression();
    ASSERT_NE(nullptr, unary);
    ASSERT_EQ(Operator::kRecv, unary->op().kind);
    ASSERT_STREQ("ch", unary->operand()->AsIdentifier()->name()->data());

    MockFile file1("ch <- data\n");
    parser_.SwitchInputFile("demos/demo.mai", &file1);

    ok = true;
    ast = parser_.ParseExpression(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);

    auto bin = ast->AsBinaryExpression();
    ASSERT_NE(nullptr, bin);
    ASSERT_EQ(Operator::kSend, bin->op().kind);
    ASSERT_STREQ("ch", bin->lhs()->AsIdentifier()->name()->data());
    ASSERT_STREQ("data", bin->rhs()->AsIdentifier()->name()->data());
}

}

}
