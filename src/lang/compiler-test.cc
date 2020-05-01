#include "lang/compiler.h"
#include "lang/lexer.h"
#include "lang/ast.h"
#include "lang/syntax-mock-test.h"
#include "base/arena-utils.h"
#include "base/arenas.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class CompilerTest : public ::testing::Test {
public:
    CompilerTest()
        : arena_(Env::Default()->GetLowLevelAllocator())
        , env_(Env::Default()) {}
    
    MockFeedback feedback_;
    base::StandaloneArena arena_;
    Env *env_;
};

TEST_F(CompilerTest, FindSourceFiles) {
    
    std::vector<std::string> files;
    auto rs = Compiler::FindSourceFiles("src/lang/pkg", env_, false, &files);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_GT(files.size(), 0);
}

}

}
