#include "lang/garbage-collector.h"
#include "lang/heap.h"
#include "test/isolate-initializer.h"
#include "base/slice.h"
#include "gtest/gtest.h"
#include <thread>

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
    RememberSet rset(16);
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
    RememberSet rset(16);
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

TEST_F(GarbageCollectorTest, RememberSetLargeInsertion) {
    RememberSet rset(16);
    ASSERT_EQ(16, rset.buckets_size());
    ASSERT_EQ(0, rset.size());
    
    std::set<Any **> unique_keys;
    
    Any *host = bit_cast<Any *>(1);
    for (int i = 0; i < 10000; i++) {
        auto key = bit_cast<Any **>(i * 4UL);
        rset.Put(host, key);
        unique_keys.insert(key);
    }
    ASSERT_EQ(10000, rset.size());
    
    RememberSet::Iterator iter(&rset);
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        unique_keys.erase(iter->address);
    }
    ASSERT_TRUE(unique_keys.empty());
}

TEST_F(GarbageCollectorTest, RememberSetMultiThreading) {
    RememberSet rset(16);
    Any *host = bit_cast<Any *>(1);
    
    std::thread th1([&rset, host] () {
        for (int i = 0; i < 10000; i++) {
            rset.Put(host, bit_cast<Any **>(i * 4UL));
        }
    });
    std::thread th2([&rset, host] () {
        for (int i = 10000; i < 20000; i++) {
            rset.Put(host, bit_cast<Any **>(i * 4UL));
        }
    });
    std::thread th3([&rset, host] () {
        for (int i = 20000; i < 30000; i++) {
            rset.Put(host, bit_cast<Any **>(i * 4UL));
        }
    });
    std::thread th4([&rset, host] () {
        for (int i = 30000; i < 40000; i++) {
            rset.Put(host, bit_cast<Any **>(i * 4UL));
        }
    });
    th1.join();
    th2.join();
    th3.join();
    th4.join();
    ASSERT_EQ(40000, rset.size());
    
    std::set<Any **> unique_keys;
    for (int i = 0; i < 40000; i++) {
        unique_keys.insert(bit_cast<Any **>(i * 4UL));
    }
    RememberSet::Iterator iter(&rset);
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        unique_keys.erase(iter->address);
    }
    ASSERT_TRUE(unique_keys.empty());
}

} // namespace lang

} // namespace mai
