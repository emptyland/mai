#include "sql/evaluation.h"
#include "sql/eval-factory.h"
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
    
} // namespace eval

} // namespace sql

} // namespace mai
