#include "db/write-batch-with-index.h"
#include "core/key-boundle.h"
#include "test/mock-column-family.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace db {
    
class WriteBatchWithIndexTest : public ::testing::Test {
public:
    WriteBatchWithIndexTest() {
        mock_cf0_ = new test::MockColumnFamily("cf0", 1, cmp_);
        mock_cf1_ = new test::MockColumnFamily("cf1", 2, cmp_);
    }
    
    ~WriteBatchWithIndexTest() override {
        delete mock_cf0_;
        delete mock_cf1_;
    }
    
    Env *env_ = Env::Default();
    const Comparator *cmp_ = Comparator::Bytewise();
    ColumnFamily *mock_cf0_;
    ColumnFamily *mock_cf1_;
};

using ::mai::core::Tag;
    
TEST_F(WriteBatchWithIndexTest, AddOrUpdate) {
    WriteBatchWithIndex batch(env_->GetLowLevelAllocator());
    
    batch.AddOrUpdate(mock_cf0_, Tag::kFlagValue, "aaaa", "bbbb");
    batch.AddOrUpdate(mock_cf0_, Tag::kFlagValue, "aaab", "bbbb");
    batch.AddOrUpdate(mock_cf0_, Tag::kFlagValue, "aaac", "bbbb");
    
    ASSERT_GT(batch.redo().size(), WriteBatch::kHeaderSize);
}
    
} // namespace db
    
} // namespace mai
