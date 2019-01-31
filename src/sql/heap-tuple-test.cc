#include "sql/heap-tuple.h"
#include "base/standalone-arena.h"
#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace sql {

class HeapTupleTest : public ::testing::Test {
public:
    HeapTupleTest() : arena_(env_->GetLowLevelAllocator()) {}
    
    Env *env_ = Env::Default();
    base::StandaloneArena arena_;
};
    
TEST_F(HeapTupleTest, Builder) {
    VirtualSchema::Builder builder("vs");
    
    auto vs = builder.BeginColumn("a", SQL_INT)
        .table_name("t1")
        .is_unsigned(true)
    .EndColumn()
    .BeginColumn("b", SQL_VARCHAR)
        .table_name("t1")
    .EndColumn()
    .Build();
    std::unique_ptr<VirtualSchema> holder(vs);
    
    ASSERT_NE(nullptr, vs);
    ASSERT_EQ(2, vs->columns_size());
    
    auto col = vs->column(0);
    EXPECT_EQ("a", col->name());
    EXPECT_EQ(SQL_INT, col->type());
    EXPECT_EQ("t1", col->table_name());
    
    col = vs->column(1);
    EXPECT_EQ("b", col->name());
    EXPECT_EQ(SQL_VARCHAR, col->type());
    EXPECT_EQ("t1", col->table_name());
}
    
TEST_F(HeapTupleTest, BuildHeapTuple) {
    VirtualSchema::Builder builder("vs");
    
    auto vs = builder
        .BeginColumn("a", SQL_INT)
            .table_name("t1")
            .is_unsigned(true)
        .EndColumn()
        .BeginColumn("b", SQL_VARCHAR)
            .table_name("t1")
        .EndColumn()
    .Build();
    std::unique_ptr<VirtualSchema> holder(vs);

    HeapTupleBuilder bd(vs, &arena_);
    bd.SetU64(0, 100);
    bd.SetNull(1);
    auto tuple = bd.Build();
    bd.Reset();
    
    auto a = vs->column(0);
    auto b = vs->column(1);
    
    ASSERT_NE(nullptr, tuple);
    ASSERT_EQ(1, tuple->null_bitmap_size());
    ASSERT_EQ(8, tuple->size());
    ASSERT_TRUE(tuple->IsNotNull(a));
    ASSERT_TRUE(tuple->IsNull(b));
    
    tuple = bd.Build();
    bd.Reset();
    ASSERT_NE(nullptr, tuple);
    ASSERT_EQ(1, tuple->null_bitmap_size());
    ASSERT_EQ(0, tuple->size());
    ASSERT_TRUE(tuple->IsNull(a));
    ASSERT_TRUE(tuple->IsNull(b));
    
    bd.SetString(1, "Hello, World");
    tuple = bd.Build();
    ASSERT_NE(nullptr, tuple);
    ASSERT_EQ(1, tuple->null_bitmap_size());
    ASSERT_EQ(8, tuple->size());
    ASSERT_TRUE(tuple->IsNull(a));
    ASSERT_TRUE(tuple->IsNotNull(b));
    ASSERT_EQ("Hello, World", tuple->GetString(b));
}

} // namespace sql
    
} // namespace mai
