#include "db/write-batch-with-index.h"
#include "core/key-boundle.h"
#include "mai/iterator.h"
#include "test/mock-column-family.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace db {
    
using namespace ::mai::core;
    
    
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
    
    std::string value;
    auto rs = batch.Get(mock_cf0_, "aaaa", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("bbbb", value);
}
    
TEST_F(WriteBatchWithIndexTest, DeletionEntries) {
    WriteBatchWithIndex batch(env_->GetLowLevelAllocator());
    
    batch.AddOrUpdate(mock_cf0_, Tag::kFlagValue, "aaaa", "bbbb");
    batch.AddOrUpdate(mock_cf0_, Tag::kFlagValue, "aaab", "bbbb");
    batch.AddOrUpdate(mock_cf0_, Tag::kFlagValue, "aaac", "bbbb");
    
    std::string value;
    auto rs = batch.Get(mock_cf0_, "aaaa", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("bbbb", value);
    
    batch.AddOrUpdate(mock_cf0_, Tag::kFlagDeletion, "aaaa", "");
    rs = batch.Get(mock_cf0_, "aaaa", &value);
    ASSERT_FALSE(rs.ok());
    ASSERT_TRUE(rs.IsNotFound());
    
    batch.AddOrUpdate(mock_cf0_, Tag::kFlagValue, "aaaa", "xxxx");
    rs = batch.Get(mock_cf0_, "aaaa", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("xxxx", value);
}
    
TEST_F(WriteBatchWithIndexTest, Iterate) {
    WriteBatchWithIndex batch(env_->GetLowLevelAllocator());
    
    batch.AddOrUpdate(mock_cf0_, Tag::kFlagValue, "aaaa", "bbbb");
    batch.AddOrUpdate(mock_cf0_, Tag::kFlagValue, "aaab", "bbbb");
    batch.AddOrUpdate(mock_cf0_, Tag::kFlagValue, "aaac", "bbbb");
    
    std::unique_ptr<Iterator> iter(batch.NewIterator(mock_cf0_));
    iter->SeekToFirst();
    ASSERT_TRUE(iter->Valid());
    ParsedTaggedKey ikey;
    KeyBoundle::ParseTaggedKey(iter->key(), &ikey);
    ASSERT_EQ("aaaa", ikey.user_key);
    ASSERT_EQ(Tag::kFlagValue, ikey.tag.flag());
    
    iter->Next();
    ASSERT_TRUE(iter->Valid());
    KeyBoundle::ParseTaggedKey(iter->key(), &ikey);
    ASSERT_EQ("aaab", ikey.user_key);
    ASSERT_EQ(Tag::kFlagValue, ikey.tag.flag());
    
    iter->Next();
    ASSERT_TRUE(iter->Valid());
    KeyBoundle::ParseTaggedKey(iter->key(), &ikey);
    ASSERT_EQ("aaac", ikey.user_key);
    ASSERT_EQ(Tag::kFlagValue, ikey.tag.flag());
}
    
} // namespace db
    
} // namespace mai
