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


}

}
