#include "sql/parser.h"
#include "sql/ast.h"
#include "sql/ast-factory.h"
#include "base/arena.h"
#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace sql {

class SQLParserTest : public ::testing::Test {
public:
    SQLParserTest()
        : arena_(env_->GetLowLevelAllocator())
        , factory_(&arena_) {}
    
    Env *env_ = Env::Default();
    base::Arena arena_;
    AstFactory factory_;
};
    
TEST_F(SQLParserTest, YaccTest) {
    Parser::Result result;
    auto rs = Parser::Parse("()", &factory_, &result);
    ASSERT_FALSE(rs.ok()) <<  rs.ToString();
}

} // namespace sql
    
} // namespace mai
