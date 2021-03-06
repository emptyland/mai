#include "sql/parser.h"
#include "sql/ast.h"
#include "sql/ast-factory.h"
#include "core/decimal-v2.h"
#include "base/arenas.h"
#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace sql {
    
using namespace ast;

class SQLParserTest : public ::testing::Test {
public:
    SQLParserTest()
        : arena_(env_->GetLowLevelAllocator()) {}
    
    Env *env_ = Env::Default();
    base::StandaloneArena arena_;
    //Factory factory_;
};
    
TEST_F(SQLParserTest, YaccTest) {
    Parser::Result result;
    auto rs = Parser::Parse("()", &arena_, &result);
    ASSERT_FALSE(rs.ok()) <<  rs.ToString();
}
    
TEST_F(SQLParserTest, TCLStatement) {
    Parser::Result result;
    auto rs = Parser::Parse("BEGIN TRANSACTION;", &arena_, &result);
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
    auto rs = Parser::Parse("DROP TABLE t1;", &arena_, &result);
    ASSERT_TRUE(rs.ok()) <<  rs.ToString() << "\n" << result.FormatError();
    
    auto stmt = DropTable::Cast(result.block->stmt(0));
    ASSERT_NE(nullptr, stmt);
    ASSERT_NE(nullptr, stmt->table_name());
    EXPECT_EQ("t1", stmt->table_name()->ToString());
}
    
TEST_F(SQLParserTest, CreateTable) {
    Parser::Result result;
    auto rs = Parser::Parse("CREATE TABLE t1 (a INT);", &arena_, &result);
    ASSERT_TRUE(rs.ok()) <<  rs.ToString() << "\n" << result.FormatError();
    
    auto stmt = CreateTable::Cast(result.block->stmt(0));
    ASSERT_NE(nullptr, stmt);
    ASSERT_NE(nullptr, stmt->table_name());
    
    EXPECT_EQ("t1", stmt->table_name()->ToString());
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
    auto rs = Parser::Parse(kT, &arena_, &result);
    ASSERT_TRUE(rs.ok()) <<  rs.ToString() << "\n" << result.FormatError();
    
    auto stmt = CreateTable::Cast(result.block->stmt(0));
    ASSERT_NE(nullptr, stmt);
    ASSERT_NE(nullptr, stmt->table_name());
    
    EXPECT_EQ("t1", stmt->table_name()->ToString());
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
    auto rs = Parser::Parse(kX, &arena_, &result);
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
    auto rs = Parser::Parse(kX, &arena_, &result);
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
    auto rs = Parser::Parse(kX, &arena_, &result);
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
    
TEST_F(SQLParserTest, SampleSelect) {
    static const char *kX = "SELECT * FROM t1;";
    
    Parser::Result result;
    auto rs = Parser::Parse(kX, &arena_, &result);
    ASSERT_TRUE(rs.ok()) <<  rs.ToString() << "\n" << result.FormatError();
    
    auto stmt = Select::Cast(result.block->stmt(0));
    ASSERT_NE(nullptr, stmt->columns());
    ASSERT_EQ(1, stmt->columns()->size());
    
    auto column = stmt->columns()->at(0);
    ASSERT_TRUE(column->alias()->empty());
    ASSERT_TRUE(column->expr()->IsPlaceholder());
    
    auto from = NameRelation::Cast(stmt->from_clause());
    ASSERT_NE(nullptr, from);
    ASSERT_STREQ("t1", from->name()->data());
    ASSERT_TRUE(from->prefix()->empty());
}
    
TEST_F(SQLParserTest, WhereClause) {
    static const char *kX = "SELECT * FROM t1 WHERE a > 0 AND a < 100;";
    
    Parser::Result result;
    auto rs = Parser::Parse(kX, &arena_, &result);
    ASSERT_TRUE(rs.ok()) <<  rs.ToString() << "\n" << result.FormatError();
    
    auto stmt = Select::Cast(result.block->stmt(0));
    ASSERT_NE(nullptr, stmt);
    
    auto where = stmt->where_clause();
    ASSERT_NE(nullptr, where);
    ASSERT_TRUE(where->is_binary());
    
    auto top = BinaryExpression::Cast(where);
    ASSERT_NE(nullptr, top);
    EXPECT_EQ(SQL_AND, top->op());
    
    auto child = Comparison::Cast(top->lhs());
    ASSERT_NE(nullptr, child);
    EXPECT_EQ(SQL_CMP_GT, child->op());
    
    auto id = Identifier::Cast(child->lhs());
    ASSERT_NE(nullptr, id);
    EXPECT_STREQ("a", id->name()->data());
    auto n = Literal::Cast(child->rhs());
    ASSERT_NE(nullptr, n);
    EXPECT_EQ(0, n->integer_val());

    child = Comparison::Cast(top->rhs());
    ASSERT_NE(nullptr, child);
    EXPECT_EQ(SQL_CMP_LT, child->op());
}
    
TEST_F(SQLParserTest, Decimal) {
    static const char *kX = "SELECT 1.00001;";
    
    Parser::Result result;
    auto rs = Parser::Parse(kX, &arena_, &result);
    ASSERT_TRUE(rs.ok()) <<  rs.ToString() << "\n" << result.FormatError();
    
    auto stmt = Select::Cast(result.block->stmt(0));
    ASSERT_NE(nullptr, stmt);
    ASSERT_EQ(1, stmt->columns()->size());
    
    auto literal = Literal::Cast(stmt->columns()->at(0)->expr());
    ASSERT_TRUE(literal->is_decimal_val());
    ASSERT_EQ("1.00001", literal->decimal_val()->ToString());
}
    
TEST_F(SQLParserTest, DateTimeLiteral) {
    static const char *kX = "SELECT {d'2019-10-11'}, {t'-999:12:34.9'}, {ts'2019-10-11 09:12:34.998'};";
    
    Parser::Result result;
    auto rs = Parser::Parse(kX, &arena_, &result);
    ASSERT_TRUE(rs.ok()) <<  rs.ToString() << "\n" << result.FormatError();
    
    auto stmt = Select::Cast(result.block->stmt(0));
    ASSERT_NE(nullptr, stmt);
    ASSERT_EQ(3, stmt->columns()->size());
    
    auto literal = Literal::Cast(stmt->columns()->at(0)->expr());
    ASSERT_TRUE(literal->is_date_val());
    SQLDateTime dt = literal->time_class_val();
    ASSERT_EQ(2019, dt.date.year);
    ASSERT_EQ(10, dt.date.month);
    ASSERT_EQ(11, dt.date.day);
    
    literal = Literal::Cast(stmt->columns()->at(1)->expr());
    ASSERT_TRUE(literal->is_time_val());
    dt = literal->time_class_val();
    ASSERT_EQ(999, dt.time.hour);
    ASSERT_TRUE(dt.time.negative);
    ASSERT_EQ(12, dt.time.minute);
    ASSERT_EQ(34, dt.time.second);
    ASSERT_EQ(900000, dt.time.micros);
    
    literal = Literal::Cast(stmt->columns()->at(2)->expr());
    ASSERT_TRUE(literal->is_datetime_val());
    dt = literal->time_class_val();
    ASSERT_EQ(2019, dt.date.year);
    ASSERT_EQ(10, dt.date.month);
    ASSERT_EQ(11, dt.date.day);
    ASSERT_EQ(9, dt.time.hour);
    ASSERT_FALSE(dt.time.negative);
    ASSERT_EQ(12, dt.time.minute);
    ASSERT_EQ(34, dt.time.second);
    ASSERT_EQ(998000, dt.time.micros);
}

} // namespace sql
    
} // namespace mai
