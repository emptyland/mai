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
    
TEST_F(SQLParserTest, TCLStatement) {
    Parser::Result result;
    auto rs = Parser::Parse("BEGIN TRANSACTION;", &factory_, &result);
    ASSERT_TRUE(rs.ok()) <<  rs.ToString() << "\n" << result.error->ToString();
    
    auto block = result.block;
    ASSERT_NE(nullptr, block);
    ASSERT_EQ(1, block->stmts_size());
    
    auto stmt = block->stmt(0);
    ASSERT_NE(nullptr, stmt);
    ASSERT_TRUE(stmt->is_statement());
    ASSERT_EQ(AstNode::kTCLStatement, stmt->kind());
}
    
TEST_F(SQLParserTest, DropTable) {
    Parser::Result result;
    auto rs = Parser::Parse("DROP TABLE t1;", &factory_, &result);
    ASSERT_TRUE(rs.ok()) <<  rs.ToString() << "\n" << result.error->ToString();
    
    auto stmt = DropTable::Cast(result.block->stmt(0));
    ASSERT_NE(nullptr, stmt);
    ASSERT_NE(nullptr, stmt->schema_name());
    EXPECT_EQ("t1", stmt->schema_name()->ToString());
}

} // namespace sql
    
} // namespace mai
