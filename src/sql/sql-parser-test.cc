#include "sql/parser.h"
#include "sql/ast.h"
#include "sql/ast-factory.h"
#include "base/standalone-arena.h"
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
    base::StandaloneArena arena_;
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
    ASSERT_TRUE(rs.ok()) <<  rs.ToString() << "\n" << result.FormatError();
    
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
    ASSERT_TRUE(rs.ok()) <<  rs.ToString() << "\n" << result.FormatError();
    
    auto stmt = DropTable::Cast(result.block->stmt(0));
    ASSERT_NE(nullptr, stmt);
    ASSERT_NE(nullptr, stmt->schema_name());
    EXPECT_EQ("t1", stmt->schema_name()->ToString());
}
    
TEST_F(SQLParserTest, CreateTable) {
    Parser::Result result;
    auto rs = Parser::Parse("CREATE TABLE t1 (a INT);", &factory_, &result);
    ASSERT_TRUE(rs.ok()) <<  rs.ToString() << "\n" << result.FormatError();
    
    auto stmt = CreateTable::Cast(result.block->stmt(0));
    ASSERT_NE(nullptr, stmt);
    ASSERT_NE(nullptr, stmt->schema_name());
    
    EXPECT_EQ("t1", stmt->schema_name()->ToString());
    ASSERT_EQ(1, stmt->columns_size());
    
    auto col = stmt->column(0);
    ASSERT_NE(nullptr, col);
    EXPECT_EQ(SQL_INT, col->type()->code());
    EXPECT_EQ(0, col->type()->fixed_size());
    EXPECT_EQ(0, col->type()->float_size());
}
    
TEST_F(SQLParserTest, CreateTableDetail1) {
    static const char *kT =
    "CREATE TABLE t1 ("
    "   a INT AUTO_INCREMENT PRIMARY KEY COMMENT 'key',"
    "   b VARCHAR(255) KEY COMMENT 'k2',"
    "   c BIGINT UNIQUE KEY"
    ");";
    
    Parser::Result result;
    auto rs = Parser::Parse(kT, &factory_, &result);
    ASSERT_TRUE(rs.ok()) <<  rs.ToString() << "\n" << result.FormatError();
    
    auto stmt = CreateTable::Cast(result.block->stmt(0));
    ASSERT_NE(nullptr, stmt);
    ASSERT_NE(nullptr, stmt->schema_name());
    
    EXPECT_EQ("t1", stmt->schema_name()->ToString());
    ASSERT_EQ(3, stmt->columns_size());
    
    auto col = stmt->column(0);
    ASSERT_NE(nullptr, col);
    EXPECT_EQ(SQL_INT, col->type()->code());
    EXPECT_EQ(0, col->type()->fixed_size());
    EXPECT_EQ(SQL_PRIMARY_KEY, col->key());
    EXPECT_EQ("key", col->comment()->ToString());
    
    col = stmt->column(1);
    ASSERT_NE(nullptr, col);
    EXPECT_EQ(SQL_VARCHAR, col->type()->code());
    EXPECT_EQ(255, col->type()->fixed_size());
    EXPECT_EQ(SQL_KEY, col->key());
    EXPECT_EQ("k2", col->comment()->ToString());
}
    
TEST_F(SQLParserTest, AlterTableAddColumn) {
    static const char *const kX =
    "ALTER TABLE t1 ADD COLUMN d SMALLINT KEY COMMENT 'new column' AFTER b;";
    
    Parser::Result result;
    auto rs = Parser::Parse(kX, &factory_, &result);
    ASSERT_TRUE(rs.ok()) <<  rs.ToString() << "\n" << result.FormatError();
    
    auto stmt = AlterTable::Cast(result.block->stmt(0));
    ASSERT_NE(nullptr, stmt);
    ASSERT_EQ(1, stmt->specs_size());
    
    auto spec = AlterTableColumn::Cast(stmt->spec(0));
    ASSERT_TRUE(spec->is_add());
    ASSERT_EQ(1, spec->columns_size());

    auto col = spec->column(0);
    ASSERT_NE(nullptr, col);
    EXPECT_EQ(SQL_SMALLINT, col->type()->code());
    EXPECT_EQ(0, col->type()->fixed_size());
    EXPECT_EQ(SQL_KEY, col->key());
    EXPECT_EQ("new column", col->comment()->ToString());
}
    
TEST_F(SQLParserTest, AlterTableAddColumns) {
    static const char *const kX =
    "ALTER TABLE t1 ADD COLUMN ("
    "   d SMALLINT KEY COMMENT 'new column',"
    "   e VARCHAR(255) COMMENT 'new varchar'"
    ");";
    
    Parser::Result result;
    auto rs = Parser::Parse(kX, &factory_, &result);
    ASSERT_TRUE(rs.ok()) <<  rs.ToString() << "\n" << result.FormatError();
    
    auto stmt = AlterTable::Cast(result.block->stmt(0));
    ASSERT_NE(nullptr, stmt);
    ASSERT_EQ(1, stmt->specs_size());
    
    auto spec = AlterTableColumn::Cast(stmt->spec(0));
    ASSERT_TRUE(spec->is_add());
    ASSERT_EQ(2, spec->columns_size());
    
    auto col = spec->column(0);
    ASSERT_NE(nullptr, col);
    EXPECT_EQ(SQL_SMALLINT, col->type()->code());
    EXPECT_EQ(0, col->type()->fixed_size());
    EXPECT_EQ(SQL_KEY, col->key());
    EXPECT_EQ("new column", col->comment()->ToString());
    
    col = spec->column(1);
    ASSERT_NE(nullptr, col);
    EXPECT_EQ(SQL_VARCHAR, col->type()->code());
    EXPECT_EQ(255, col->type()->fixed_size());
    EXPECT_EQ(SQL_NOT_KEY, col->key());
    EXPECT_EQ("new varchar", col->comment()->ToString());
}
    
TEST_F(SQLParserTest, AlterTableAddIndex) {
    static const char *kX =
    "ALTER TABLE t1 ADD INDEX name (name), ADD KEY id UNIQUE KEY (id);";
    
    Parser::Result result;
    auto rs = Parser::Parse(kX, &factory_, &result);
    ASSERT_TRUE(rs.ok()) <<  rs.ToString() << "\n" << result.FormatError();
    
    auto stmt = AlterTable::Cast(result.block->stmt(0));
    ASSERT_NE(nullptr, stmt);
    ASSERT_EQ(2, stmt->specs_size());
    
    auto spec = AlterTableIndex::Cast(stmt->spec(0));
    ASSERT_TRUE(spec->is_add());
    EXPECT_EQ("name", spec->new_name()->ToString());
    ASSERT_EQ(1, spec->col_names_size());
    EXPECT_EQ("name", spec->col_name(0)->ToString());
    EXPECT_EQ(SQL_KEY, spec->key());
    
    spec = AlterTableIndex::Cast(stmt->spec(1));
    ASSERT_TRUE(spec->is_add());
    EXPECT_EQ("id", spec->new_name()->ToString());
    ASSERT_EQ(1, spec->col_names_size());
    EXPECT_EQ("id", spec->col_name(0)->ToString());
    EXPECT_EQ(SQL_UNIQUE_KEY, spec->key());
}

} // namespace sql
    
} // namespace mai
