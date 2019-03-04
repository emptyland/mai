#include "sql/evaluation.h"
#include "sql/eval-factory.h"
#include "sql/heap-tuple.h"
#include "sql/parser.h"
#include "sql/ast.h"
#include "base/standalone-arena.h"
#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace sql {

namespace eval {
    
class EvaluationTest : public ::testing::Test {
public:
    EvaluationTest()
        : arena_(env_->GetLowLevelAllocator())
        , factory_(&arena_) {}
    
    void BuildBaseSchema(std::unique_ptr<VirtualSchema> *result) {
        VirtualSchema::Builder builder("");
        
        auto vs = builder.BeginColumn("a", SQL_BIGINT)
            .is_unsigned(true)
        .EndColumn()
        .BeginColumn("b", SQL_MEDIUMINT)
            .is_unsigned(false)
        .EndColumn()
        .BeginColumn("c", SQL_VARCHAR)
            .table_name("t1")
        .EndColumn()
        .BeginColumn("d", SQL_CHAR)
            .table_name("t2")
        .EndColumn()
        .BeginColumn("e", SQL_DOUBLE)
            .table_name("t1")
        .EndColumn()
        .Build();
        ASSERT_TRUE(builder.error().ok()) << builder.error().ToString();
        ASSERT_NE(nullptr, vs);
        result->reset(vs);
    }
    
    void ParseExpressions(const char *s, std::vector<ast::Expression *> *rv) {
        Parser::Result result;
        auto rs = Parser::Parse(s, &arena_, &result);
        ASSERT_TRUE(rs.ok()) << result.FormatError();
        
        auto columns = PeekColumns(result.block);
        
        rv->clear();
        for (auto col : *columns) {
            rv->push_back(col);
        }
    }
    
    ast::ProjectionColumnList *PeekColumns(const ast::Block *block) {
        auto stmt = ast::Select::Cast(block->stmt(0));
        if (!stmt) {
            return nullptr;
        }
        return stmt->columns();
    }
    
    Env *env_ = Env::Default();
    base::StandaloneArena arena_;
    Factory factory_;
};
    
TEST_F(EvaluationTest, Sanity) {
    auto lhs = factory_.NewConstI64(100);
    auto expr = factory_.NewUnary(SQL_MINUS, lhs);
    
    eval::Context ctx(&arena_);
    auto rs = expr->Evaluate(&ctx);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    ASSERT_EQ(eval::Value::kI64, ctx.result().kind);
    ASSERT_EQ(-100, ctx.result().i64_val);
}
    
TEST_F(EvaluationTest, UnaryOperator) {
    auto lhs = factory_.NewConstStr("-99");
    auto expr = factory_.NewUnary(SQL_MINUS, lhs);
    
    eval::Context ctx(&arena_);
    auto rs = expr->Evaluate(&ctx);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    ASSERT_EQ(eval::Value::kI64, ctx.result().kind);
    ASSERT_EQ(99, ctx.result().i64_val);
}
    
TEST_F(EvaluationTest, MinusF64) {
    auto lhs = factory_.NewConstStr("-1.99");
    auto expr = factory_.NewUnary(SQL_MINUS, lhs);
    
    eval::Context ctx(&arena_);
    auto rs = expr->Evaluate(&ctx);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    ASSERT_EQ(eval::Value::kDecimal, ctx.result().kind);
    ASSERT_EQ(1.99f, ctx.result().dec_val->ToF32());
}
    
TEST_F(EvaluationTest, PlusInt) {
    auto lhs = factory_.NewConstStr("-1.99");
    auto rhs = factory_.NewConstI64(2);
    auto expr = factory_.NewBinary(SQL_PLUS, lhs, rhs);
    
    eval::Context ctx(&arena_);
    auto rs = expr->Evaluate(&ctx);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    ASSERT_EQ(eval::Value::kDecimal, ctx.result().kind);
    ASSERT_EQ("0.01", ctx.result().dec_val->ToString());
}
    
TEST_F(EvaluationTest, PlusDateTime) {
    
}
    
TEST_F(EvaluationTest, VariableReading) {
    std::unique_ptr<VirtualSchema> vs;
    BuildBaseSchema(&vs);
    if (!vs) {
        return;
    }
    
    HeapTupleBuilder builder(vs.get(), &arena_);
    builder.SetU64(0, 99);
    builder.SetI64(1, -99);
    
    eval::Context ctx(&arena_);
    auto row = builder.Build();
    EXPECT_EQ(99,  row->GetU64(vs->column(0)));
    EXPECT_EQ(-99, row->GetI64(vs->column(1)));
    
    ctx.set_schema(vs.get());
    ctx.set_input(row);
    
    auto lhs = factory_.NewVariable(vs.get(), 0);
    auto rhs = factory_.NewVariable(vs.get(), 1);
    auto expr = factory_.NewBinary(SQL_PLUS, lhs, rhs);

    auto rs = expr->Evaluate(&ctx);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ(eval::Value::kU64, ctx.result().kind);
    ASSERT_EQ(0, ctx.result().i64_val);
}
    
TEST_F(EvaluationTest, BuildEvalExpression) {
    static const char *kX = "SELECT 1 + (1 + 200) / 2;";
    
    Parser::Result result;
    auto rs = Parser::Parse(kX, &arena_, &result);
    ASSERT_TRUE(rs.ok()) << result.FormatError();
    
    auto columns = PeekColumns(result.block);
    auto expr = Evaluation::BuildExpression(nullptr, columns->at(0), &arena_);
    ASSERT_NE(nullptr, expr);
    ASSERT_NE(nullptr, dynamic_cast<eval::Operation *>(expr));
    
    eval::Context ctx(&arena_);
    rs = expr->Evaluate(&ctx);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    ASSERT_EQ(Value::kI64, ctx.result().kind);
    ASSERT_EQ(101, ctx.result().i64_val);
}
    
TEST_F(EvaluationTest, BuildEvalStringExpression) {
    static const char *kX = "SELECT 1 + (1 + 200) / '1.2';";
    
    std::vector<ast::Expression *> columns;
    ParseExpressions(kX, &columns);
    
    auto expr = Evaluation::BuildExpression(nullptr, columns[0], &arena_);
    ASSERT_NE(nullptr, expr);
    ASSERT_NE(nullptr, dynamic_cast<eval::Operation *>(expr));
    
    eval::Context ctx(&arena_);
    auto rs = expr->Evaluate(&ctx);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    ASSERT_EQ(Value::kDecimal, ctx.result().kind);
    ASSERT_EQ("168.5", ctx.result().dec_val->ToString());
}
    
} // namespace eval

} // namespace sql

} // namespace mai
