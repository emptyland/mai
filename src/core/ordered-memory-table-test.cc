#include "core/ordered-memory-table.h"
#include "core/internal-key-comparator.h"
#include "base/hash.h"
#include "mai/env.h"
#include "mai/iterator.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace core {
    
using ::mai::core::KeyBoundle;
using ::mai::core::Tag;
    
class OrderedMemoryTableTest : public ::testing::Test {
public:
    OrderedMemoryTableTest()
        : ikcmp_(new core::InternalKeyComparator(Comparator::Bytewise())) {}
    void SetUp() override {}
    void TearDown() override {}

    std::unique_ptr<InternalKeyComparator> ikcmp_;
};

TEST_F(OrderedMemoryTableTest, Sanity) {
    auto h1 = base::MakeRef(new OrderedMemoryTable(ikcmp_.get()));
    h1->Put("aaaa", "v1", 1, Tag::kFlagValue);
    h1->Put("aaab", "v2", 2, Tag::kFlagValue);
    h1->Put("aaac", "v3", 3, Tag::kFlagValue);
    h1->Put("aaad", "v4", 4, Tag::kFlagValue);
    
    ASSERT_EQ(168, h1->ApproximateMemoryUsage());
    
    std::string value;
    auto rs = h1->Get("aaaa", 5, nullptr, &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("v1", value);
    
    rs = h1->Get("aaab", 5, nullptr, &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("v2", value);
    
    rs = h1->Get("bbbb", 5, nullptr, &value);
    ASSERT_TRUE(rs.IsNotFound());
    
    ASSERT_NEAR(0, h1->ApproximateConflictFactor(), 0.00001);
}
    
TEST_F(OrderedMemoryTableTest, IterateFarward) {
    const char *kv[] = {
        "aaaa", "v1",
        "aaab", "v2",
        "aaac", "v3",
        "aaad", "v4",
        "aaae", "v5",
        "aaaf", "v6",
    };
    auto h1 = base::MakeRef(new OrderedMemoryTable(ikcmp_.get()));
    for (size_t i = 0; i < arraysize(kv) / 2; i += 2) {
        h1->Put(kv[i], kv[i + 1], i / 2 + 1, Tag::kFlagValue);
    }
    
    std::unique_ptr<Iterator> iter(h1->NewIterator());
    ASSERT_TRUE(iter->error().ok()) << iter->error().ToString();
    
    iter->SeekToFirst();
    for (size_t i = 0; i < arraysize(kv) / 2; i += 2) {
        ASSERT_TRUE(iter->Valid());
        ASSERT_EQ(kv[i], KeyBoundle::ExtractUserKey(iter->key()));
        ASSERT_EQ(kv[i + 1], iter->value());
        iter->Next();
    }
    ASSERT_FALSE(iter->Valid());
}
    

TEST_F(OrderedMemoryTableTest, IterateReserve) {
    const char *kv[] = {
        "aaaa", "v1",
        "aaab", "v2",
        "aaac", "v3",
        "aaad", "v4",
        "aaae", "v5",
        "aaaf", "v6",
    };
    auto h1 = base::MakeRef(new OrderedMemoryTable(ikcmp_.get()));
    for (size_t i = 0; i < arraysize(kv) / 2; i += 2) {
        h1->Put(kv[i], kv[i + 1], i / 2 + 1, Tag::kFlagValue);
    }
    
    std::unique_ptr<Iterator> iter(h1->NewIterator());
    ASSERT_TRUE(iter->error().ok()) << iter->error().ToString();
    
    iter->SeekToLast();
    for (int64_t i = arraysize(kv) / 2 - 2; i >= 0; i -= 2) {
        ASSERT_TRUE(iter->Valid());
        ASSERT_EQ(kv[i], KeyBoundle::ExtractUserKey(iter->key()));
        ASSERT_EQ(kv[i + 1], iter->value());
        iter->Prev();
    }
    ASSERT_FALSE(iter->Valid());
}
    
} // namespace core
    
} // namespace mai
