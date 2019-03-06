#include "sql/evaluation.h"
#include "sql/eval-factory.h"
#include "sql/heap-tuple.h"
#include "sql/parser.h"
#include "sql/ast.h"
#include "core/decimal-v2.h"
#include "base/arenas.h"
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
        
        auto vs = builder.BeginColumn("a", SQL_BIGINT) // 0
            .is_unsigned(true)
        .EndColumn()
        .BeginColumn("b", SQL_MEDIUMINT) // 1
            .is_unsigned(false)
        .EndColumn()
        .BeginColumn("c", SQL_VARCHAR) // 2
            .table_name("t1")
        .EndColumn()
        .BeginColumn("d", SQL_CHAR) // 3
            .table_name("t2")
        .EndColumn()
        .BeginColumn("e", SQL_DOUBLE) // 4
            .table_name("t1")
        .EndColumn()
        .BeginColumn("f", SQL_DECIMAL) // 5
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
    
TEST_F(EvaluationTest, I64ToNumeric) {
    Value v;
    v.kind = Value::kI64;
    v.i64_val = 991;
    
    Value r = v.ToNumeric(&arena_);
    ASSERT_EQ(Value::kI64, r.kind);
    ASSERT_EQ(991, r.i64_val);
    
    r = v.ToNumeric(&arena_, Value::kU64);
    ASSERT_EQ(Value::kU64, r.kind);
    ASSERT_EQ(991, r.u64_val);
}

TEST_F(EvaluationTest, U64ToNumeric) {
    Value v;
    v.kind = Value::kU64;
    v.u64_val = 991;
    
    Value r = v.ToNumeric(&arena_);
    ASSERT_EQ(Value::kU64, r.kind);
    ASSERT_EQ(991, r.u64_val);
    
    r = v.ToNumeric(&arena_, Value::kI64);
    ASSERT_EQ(Value::kI64, r.kind);
    ASSERT_EQ(991, r.i64_val);
}
    
TEST_F(EvaluationTest, F64ToNumeric) {
    Value v;
    v.kind = Value::kF64;
    v.f64_val = 991.123;
    
    Value r = v.ToNumeric(&arena_);
    ASSERT_EQ(Value::kF64, r.kind);
    ASSERT_EQ(991.123, r.f64_val);
}
    
TEST_F(EvaluationTest, StringToNumeric) {
    Value v;
    v.kind = Value::kString;
    v.str_val = AstString::New(&arena_, "123456");
    
    Value r = v.ToNumeric(&arena_);
    ASSERT_EQ(Value::kU64, r.kind);
    ASSERT_EQ(123456, r.u64_val);
    
    v.str_val = AstString::New(&arena_, "-123456");
    r = v.ToNumeric(&arena_);
    ASSERT_EQ(Value::kI64, r.kind);
    ASSERT_EQ(-123456, r.i64_val);
    
    v.str_val = AstString::New(&arena_, "hello");
    r = v.ToNumeric(&arena_);
    ASSERT_EQ(Value::kU64, r.kind);
    ASSERT_EQ(0, r.u64_val);
    
    //UINT64_MAX
    v.str_val = AstString::New(&arena_, "18446744073709551615");
    r = v.ToNumeric(&arena_);
    ASSERT_EQ(Value::kU64, r.kind);
    ASSERT_EQ(18446744073709551615ULL, r.u64_val);
    
    v.str_val = AstString::New(&arena_, "18446744073709551616");
    r = v.ToNumeric(&arena_);
    ASSERT_EQ(Value::kDecimal, r.kind);
    ASSERT_EQ("18446744073709551616", r.dec_val->ToString());
    
    v.str_val = AstString::New(&arena_, "0xfedcba3210");
    r = v.ToNumeric(&arena_);
    ASSERT_EQ(Value::kU64, r.kind);
    ASSERT_EQ(0xfedcba3210, r.u64_val);
    
    v.str_val = AstString::New(&arena_, "0x10000000000000000");
    r = v.ToNumeric(&arena_);
    ASSERT_EQ(Value::kDecimal, r.kind);
    ASSERT_EQ("10000000000000000", r.dec_val->ToString(16));
    
    v.str_val = AstString::New(&arena_, "017777777");
    r = v.ToNumeric(&arena_);
    ASSERT_EQ(Value::kU64, r.kind);
    ASSERT_EQ(017777777, r.u64_val);
    
    v.str_val = AstString::New(&arena_, "0.00123");
    r = v.ToNumeric(&arena_);
    ASSERT_EQ(Value::kDecimal, r.kind);
    ASSERT_EQ("0.00123", r.dec_val->ToString());
    
    v.str_val = AstString::New(&arena_, "-1.23e-3");
    r = v.ToNumeric(&arena_);
    ASSERT_EQ(Value::kDecimal, r.kind);
    ASSERT_EQ("-0.00123", r.dec_val->ToString());
}
    
TEST_F(EvaluationTest, StringToNumericHint) {
    Value v;
    v.kind = Value::kString;
    v.str_val = AstString::New(&arena_, "12:34:55.1");
    Value r = v.ToNumeric(&arena_, Value::kTime);
    ASSERT_EQ(Value::kDecimal, r.kind);
    ASSERT_EQ("123455.1", r.dec_val->ToString());
    
    v.str_val = AstString::New(&arena_, "12:34:55");
    r = v.ToNumeric(&arena_, Value::kTime);
    ASSERT_EQ(Value::kU64, r.kind);
    ASSERT_EQ(123455, r.u64_val);
    
    v.str_val = AstString::New(&arena_, "2019-02-14");
    r = v.ToNumeric(&arena_, Value::kDate);
    ASSERT_EQ(Value::kU64, r.kind);
    ASSERT_EQ(20190214, r.u64_val);
    
    v.str_val = AstString::New(&arena_, "2019-02-14 12:34:55.1");
    r = v.ToNumeric(&arena_, Value::kDateTime);
    ASSERT_EQ(Value::kDecimal, r.kind);
    ASSERT_EQ("20190214123455.1", r.dec_val->ToString());
    
    v.str_val = AstString::New(&arena_, "2019-02-14 12:34:55");
    r = v.ToNumeric(&arena_, Value::kDateTime);
    ASSERT_EQ(Value::kU64, r.kind);
    ASSERT_EQ(20190214123455ULL, r.u64_val);
    
    v.str_val = AstString::New(&arena_, "-1.23e-3");
    r = v.ToNumeric(&arena_);
    ASSERT_EQ(Value::kDecimal, r.kind);
    ASSERT_EQ("-0.00123", r.dec_val->ToString());
}
    
TEST_F(EvaluationTest, DecimalToNumeric) {
    Value v;
    v.kind = Value::kDecimal;
    v.dec_val = SQLDecimal::NewI64(-1, &arena_);
    
    Value r = v.ToNumeric(&arena_);
    ASSERT_EQ(Value::kDecimal, r.kind);
    ASSERT_EQ(-1, r.dec_val->ToI64());
}
    
TEST_F(EvaluationTest, DateToNumeric) {
    Value v;
    v.kind = Value::kDate;
    v.dt_val.date = {2019, 2, 14};
    Value r = v.ToNumeric(&arena_);
    ASSERT_EQ(Value::kU64, r.kind);
    ASSERT_EQ(20190214, r.u64_val);
    
    r = v.ToNumeric(&arena_, Value::kTime);
    ASSERT_EQ(Value::kU64, r.kind);
    ASSERT_EQ(0, r.u64_val);
    
    r = v.ToNumeric(&arena_, Value::kDateTime);
    ASSERT_EQ(Value::kU64, r.kind);
    ASSERT_EQ(20190214000000, r.u64_val);
}
    
TEST_F(EvaluationTest, TimeToNumeric) {
    Value v;
    v.kind = Value::kTime;
    v.dt_val.time = {12, 32, 60, 0};
    Value r = v.ToNumeric(&arena_);
    ASSERT_EQ(Value::kU64, r.kind);
    ASSERT_EQ(123260, r.u64_val);
    
    r = v.ToNumeric(&arena_, Value::kDate);
    ASSERT_EQ(Value::kU64, r.kind);
    ASSERT_EQ(SQLDate::Now().ToU64(), r.u64_val);
    
    r = v.ToNumeric(&arena_, Value::kDateTime);
    ASSERT_EQ(Value::kU64, r.kind);
    ASSERT_EQ(SQLDate::Now().ToU64() * 1000000 + 123260, r.u64_val);
    
    v.dt_val.time = {12, 32, 60, 0, 999999};
    r = v.ToNumeric(&arena_);
    ASSERT_EQ(Value::kDecimal, r.kind);
    ASSERT_EQ("123260.999999", r.dec_val->ToString());
}
    
TEST_F(EvaluationTest, DateTimeToNumeric) {
    Value v;
    v.kind = Value::kDateTime;
    v.dt_val.date = {2019, 2, 14};
    v.dt_val.time = {12, 32, 60, 0};
    Value r = v.ToNumeric(&arena_);
    ASSERT_EQ(Value::kU64, r.kind);
    ASSERT_EQ(20190214123260, r.u64_val);
    
    r = v.ToNumeric(&arena_, Value::kTime);
    ASSERT_EQ(Value::kU64, r.kind);
    ASSERT_EQ(123260, r.u64_val);
    
    r = v.ToNumeric(&arena_, Value::kDate);
    ASSERT_EQ(Value::kU64, r.kind);
    ASSERT_EQ(20190214, r.u64_val);
    
    v.dt_val.date = {2019, 2, 14};
    v.dt_val.time = {12, 32, 60, 0, 100000};
    r = v.ToNumeric(&arena_);
    ASSERT_EQ(Value::kDecimal, r.kind);
    ASSERT_EQ("20190214123260.1", r.dec_val->ToString());
}
    
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
    
TEST_F(EvaluationTest, EvalVariableExpression) {
    static const char *kX = "SELECT ABS(b) AS ab, ABS(t1.f) AS af;";
    
    std::vector<ast::Expression *> columns;
    ParseExpressions(kX, &columns);
    
    std::unique_ptr<VirtualSchema> vs;
    BuildBaseSchema(&vs);
    
    auto expr0 = Evaluation::BuildExpression(vs.get(), columns[0], &arena_);
    ASSERT_NE(nullptr, expr0);
    ASSERT_NE(nullptr, dynamic_cast<eval::Invocation *>(expr0));
    
    auto expr1 = Evaluation::BuildExpression(vs.get(), columns[1], &arena_);
    ASSERT_NE(nullptr, expr1);
    ASSERT_NE(nullptr, dynamic_cast<eval::Invocation *>(expr1));
    
    HeapTupleBuilder builder(vs.get(), &arena_);
    builder.SetI64(1, -991);
    builder.SetDecimal(5, SQLDecimal::NewParsed("-1.0125", &arena_));
    
    eval::Context ctx(&arena_);
    ctx.set_input(builder.Build());
    ctx.set_schema(vs.get());
    
    auto rs = expr0->Evaluate(&ctx);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ(Value::kI64, ctx.result().kind);
    ASSERT_EQ(991, ctx.result().i64_val);
    
    rs = expr1->Evaluate(&ctx);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ(Value::kDecimal, ctx.result().kind);
    ASSERT_EQ("1.0125", ctx.result().dec_val->ToString());
}
    
} // namespace eval

} // namespace sql

} // namespace mai
