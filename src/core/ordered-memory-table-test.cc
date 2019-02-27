#include "core/ordered-memory-table.h"
#include "core/internal-key-comparator.h"
#include "base/hash.h"
#include "base/slice.h"
#include "mai/env.h"
#include "mai/iterator.h"
#include "gtest/gtest.h"
#include <thread>

namespace mai {
    
namespace core {
    
using ::mai::core::KeyBoundle;
using ::mai::core::Tag;
    
class OrderedMemoryTableTest : public ::testing::Test {
public:
    OrderedMemoryTableTest()
        : ikcmp_(Comparator::Bytewise()) {}

    InternalKeyComparator ikcmp_;
    Env *env_ = Env::Default();
    Allocator *ll_allocator_ = Env::Default()->GetLowLevelAllocator();
};

TEST_F(OrderedMemoryTableTest, Sanity) {
    auto h1 = base::MakeRef(new OrderedMemoryTable(&ikcmp_, ll_allocator_));
    h1->Put("aaaa", "v1", 1, Tag::kFlagValue);
    h1->Put("aaab", "v2", 2, Tag::kFlagValue);
    h1->Put("aaac", "v3", 3, Tag::kFlagValue);
    h1->Put("aaad", "v4", 4, Tag::kFlagValue);
    
    ASSERT_EQ(16 * base::kKB, h1->ApproximateMemoryUsage());
    
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
    auto h1 = base::MakeRef(new OrderedMemoryTable(&ikcmp_, ll_allocator_));
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
    auto h1 = base::MakeRef(new OrderedMemoryTable(&ikcmp_, ll_allocator_));
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
    
TEST_F(OrderedMemoryTableTest, ConcurrentPutting) {
    static const int kN = 10240;
    
    std::thread worker_thrds[8];
    std::atomic<core::SequenceNumber> sn(1);
    std::mutex mutex;
    
    auto table = base::MakeRef(new OrderedMemoryTable(&ikcmp_, ll_allocator_));
    for (int i = 0; i < arraysize(worker_thrds); ++i) {
        worker_thrds[i] = std::thread([&](auto slot) {
            for (int j = 0; j < kN; ++j) {
                std::string key = base::Sprintf("k.%d.%d", slot, j);
                std::string value = base::Sprintf("v.%d", j);
                mutex.lock();
                table->Put(key, value, sn.fetch_add(1), Tag::kFlagValue);
                mutex.unlock();
            }
        }, i);
    }
    
    for (auto &thrd : worker_thrds) {
        thrd.join();
    }
    
    ASSERT_EQ(arraysize(worker_thrds) * kN + 1, sn.load());
    for (int i = 0; i < arraysize(worker_thrds); ++i) {
        for (int j = 0; j < kN; ++j) {
            std::string key = base::Sprintf("k.%d.%d", i, j);
            std::string value = base::Sprintf("v.%d", j);
            std::string result;
            
            Error rs = table->Get(key, Tag::kMaxSequenceNumber, nullptr, &result);
            if (rs.fail()) {
                printf("Error: (%s) %s\n", key.c_str(), rs.ToString().c_str());
                continue;
            }
            EXPECT_EQ(value, result) << key;
        }
    }
}
    
TEST_F(OrderedMemoryTableTest, BatchPut) {
    static const int kN = 10000;
    
    base::intrusive_ptr<MemoryTable>
        table(new OrderedMemoryTable(&ikcmp_, ll_allocator_));
    
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
