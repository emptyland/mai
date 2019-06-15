#include "core/bw-tree-memory-table.h"
#include "core/internal-key-comparator.h"
#include "base/hash.h"
#include "base/slice.h"
#include "mai/env.h"
#include "mai/iterator.h"
#include "gtest/gtest.h"

namespace mai {

namespace core {
    
class BwTreeMemoryTableTest : public ::testing::Test {
public:
    BwTreeMemoryTableTest()
        : ikcmp_(Comparator::Bytewise()) {}
    
    InternalKeyComparator ikcmp_;
    
    Env *env_ = Env::Default();
    Allocator *ll_allocator_ = Env::Default()->GetLowLevelAllocator();
};
    
TEST_F(BwTreeMemoryTableTest, Sanity) {
    base::intrusive_ptr<MemoryTable> table(new BwTreeMemoryTable(&ikcmp_, env_,
                                                                 ll_allocator_));
    table->Put("aaa", "v1", 1, Tag::kFlagValue);
    table->Put("aaa", "v2", 2, Tag::kFlagValue);
    
    ASSERT_EQ(2, table->NumEntries());
    
    std::string value;
    Error rs = table->Get("aaa", 3, nullptr, &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    EXPECT_EQ("v2", value);

    rs = table->Get("aaa", 1, nullptr, &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    EXPECT_EQ("v1", value);
    
    rs = table->Get("aaa", 0, nullptr, &value);
    ASSERT_FALSE(rs.ok());
}
    
TEST_F(BwTreeMemoryTableTest, Put) {
    static const char *kv[] = {
        "aaa", "v1",
        "aac", "v2",
        "aaa", "v3",
        "aab", "v4",
        "bbb", "v5",
    };
    base::intrusive_ptr<MemoryTable> table(new BwTreeMemoryTable(&ikcmp_, env_,
                                                                 ll_allocator_));
    
    SequenceNumber sn = 1;
    for (int i = 0; i < arraysize(kv); i += 2) {
        table->Put(kv[i], kv[i + 1], sn++, Tag::kFlagValue);
    }
    ASSERT_EQ(5, table->NumEntries());
    
    std::unique_ptr<Iterator> iter(table->NewIterator());
    iter->SeekToFirst();
    ASSERT_TRUE(iter->Valid());
    ASSERT_EQ("aaa", KeyBoundle::ExtractUserKey(iter->key()));
    ASSERT_EQ("v3", iter->value());
    
    iter->Next();
    ASSERT_TRUE(iter->Valid());
    ASSERT_EQ("aaa", KeyBoundle::ExtractUserKey(iter->key()));
    ASSERT_EQ("v1", iter->value());
    
    iter->Next();
    ASSERT_TRUE(iter->Valid());
    ASSERT_EQ("aab", KeyBoundle::ExtractUserKey(iter->key()));
    ASSERT_EQ("v4", iter->value());
    
    iter->Next();
    ASSERT_TRUE(iter->Valid());
    ASSERT_EQ("aac", KeyBoundle::ExtractUserKey(iter->key()));
    ASSERT_EQ("v2", iter->value());
    
    iter->Next();
    ASSERT_TRUE(iter->Valid());
    ASSERT_EQ("bbb", KeyBoundle::ExtractUserKey(iter->key()));
    ASSERT_EQ("v5", iter->value());
    
    iter->Next();
    ASSERT_FALSE(iter->Valid());
}
    
TEST_F(BwTreeMemoryTableTest, BatchPut) {
    static const int kN = 10000;
    
    base::intrusive_ptr<MemoryTable> table(new BwTreeMemoryTable(&ikcmp_, env_,
                                                                 ll_allocator_));
    
    SequenceNumber sn = 1;
    auto jiffy = env_->CurrentTimeMicros();
    for (int i = 0; i < kN; ++i) {
        std::string k = base::Sprintf("k.%05d", i);
        std::string v = base::Sprintf("v.%05d", i);
        table->Put(k, v, sn++, Tag::kFlagValue);
    }
    auto cost = (env_->CurrentTimeMicros() - jiffy) / 1000.0;
    
    printf("put cost: %f\n", cost);
    
    std::string value;
    
    jiffy = env_->CurrentTimeMicros();
    for (int i = 0; i < kN; ++i) {
        std::string k = base::Sprintf("k.%05d", i);
        table->Get(k, sn, nullptr, &value);
    }
    cost = (env_->CurrentTimeMicros() - jiffy) / 1000.0;
    
    printf("get cost: %f\n", cost);
    
}
    
} // namespace core
    
} // namespace mai
