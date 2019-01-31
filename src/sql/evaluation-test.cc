#include "sql/evaluation.h"
#include "sql/eval-factory.h"
#include "sql/heap-tuple.h"
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
    
    Env *env_ = Env::Default();
    base::StandaloneArena arena_;
    EvalFactory factory_;
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
    
    ASSERT_EQ(eval::Value::kF64, ctx.result().kind);
    ASSERT_EQ(1.99, ctx.result().f64_val);
}
    
TEST_F(EvaluationTest, PlusInt) {
    auto lhs = factory_.NewConstStr("-1.99");
    auto rhs = factory_.NewConstI64(2);
    auto expr = factory_.NewBinary(SQL_PLUS, lhs, rhs);
    
    eval::Context ctx(&arena_);
    auto rs = expr->Evaluate(&ctx);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    ASSERT_EQ(eval::Value::kF64, ctx.result().kind);
    ASSERT_NEAR(0.01, ctx.result().f64_val, 0.0000001);
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
    
} // namespace eval

} // namespace sql

} // namespace mai
