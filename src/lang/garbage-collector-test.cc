#include "lang/garbage-collector.h"
#include "lang/heap.h"
#include "test/isolate-initializer.h"
#include "base/slice.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class GarbageCollectorTest : public test::IsolateInitializer {
public:
    GarbageCollectorTest() {}
    
    void SetUp() override {
        IsolateInitializer::SetUp();
    }
    void TearDown() override {
        IsolateInitializer::TearDown();
    }
}; // class GarbageCollectorTest


TEST_F(GarbageCollectorTest, RememberSetSanity) {
    RememberSet rset(isolate_->env()->GetLowLevelAllocator(), 16);
    ASSERT_EQ(16, rset.buckets_size());
    ASSERT_EQ(0, rset.size());
    
    Any *host = bit_cast<Any *>(1);
    rset.Put(host, bit_cast<Any **>(1UL * 4));
    rset.Put(host, bit_cast<Any **>(2UL * 4));
    rset.Put(host, bit_cast<Any **>(3UL * 4));
    rset.Put(host, bit_cast<Any **>(4UL * 4));
    
    ASSERT_EQ(4, rset.size());
    
    RememberSet::Iterator iter(&rset);
    iter.SeekToFirst();
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(host, iter->host);
    ASSERT_EQ(bit_cast<Any **>(1UL * 4), iter->address);
    ASSERT_EQ(0, iter->seuqnce_number);
    iter.Next();
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(host, iter->host);
    ASSERT_EQ(bit_cast<Any **>(2UL * 4), iter->address);
    ASSERT_EQ(1, iter->seuqnce_number);
    iter.Next();
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(host, iter->host);
    ASSERT_EQ(bit_cast<Any **>(3UL * 4), iter->address);
    ASSERT_EQ(2, iter->seuqnce_number);
    iter.Next();
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(host, iter->host);
    ASSERT_EQ(bit_cast<Any **>(4UL * 4), iter->address);
    ASSERT_EQ(3, iter->seuqnce_number);
    iter.Next();
    ASSERT_FALSE(iter.Valid());
}

TEST_F(GarbageCollectorTest, RememberSetMultiVersions) {
    RememberSet rset(isolate_->env()->GetLowLevelAllocator(), 16);
    ASSERT_EQ(16, rset.buckets_size());
    ASSERT_EQ(0, rset.size());
    
    Any *host = bit_cast<Any *>(1);
    rset.Put(host, bit_cast<Any **>(1UL * 4));
    rset.Put(host, bit_cast<Any **>(1UL * 4));
    rset.Delete(bit_cast<Any **>(1UL * 4));
    
    ASSERT_EQ(3, rset.size());
    
    RememberSet::Iterator iter(&rset);
    iter.SeekToFirst();
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(bit_cast<Any **>(1UL * 4), iter->address);
    ASSERT_EQ(2, iter->seuqnce_number);
    ASSERT_TRUE(iter.is_deletion());
    iter.Next();
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(host, iter->host);
    ASSERT_EQ(bit_cast<Any **>(1UL * 4), iter->address);
    ASSERT_EQ(1, iter->seuqnce_number);
    ASSERT_TRUE(iter.is_record());
    iter.Next();
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(host, iter->host);
    ASSERT_EQ(bit_cast<Any **>(1UL * 4), iter->address);
    ASSERT_EQ(0, iter->seuqnce_number);
    ASSERT_TRUE(iter.is_record());
}

} // namespace lang

} // namespace mai
