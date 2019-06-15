#include "core/skip-list.h"
#include "base/arenas.h"
#include "mai/env.h"
#include "gtest/gtest.h"
#include <stdio.h>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace mai {

namespace core {

class SkipListTest : public ::testing::Test {
public:
    using IntSkipList = SkipList<int, std::function<int (int, int)>>;
    
    void SetUp() override {
        arena_.reset(new base::StandaloneArena(Env::Default()->GetLowLevelAllocator()));
    }

    void Fill(int k, IntSkipList *list) {
        while (k--) {
            list->Put(std::move(k));
        }
    }

    std::unique_ptr<base::Arena> arena_;
};

TEST_F(SkipListTest, Sanity) {

    IntSkipList list([](int a, int b) { return a - b; }, arena_.get());

    static const auto k = 100;
    Fill(k, &list);

    auto i = k;
    while (i--) {
        EXPECT_TRUE(list.Contains(i));
    }
}

TEST_F(SkipListTest, Sequence) {
    IntSkipList list([](int a, int b) { return a - b; }, arena_.get());

    static const auto k = 100;
    Fill(k, &list);

    IntSkipList::Iterator iter(&list);
    auto i = 0;
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        EXPECT_EQ(i++, iter.key());
    }
}

TEST_F(SkipListTest, Seek) {
    IntSkipList list([](int a, int b) { return a - b; }, arena_.get());

    static const auto k = 100;
    Fill(k, &list);

    IntSkipList::Iterator iter(&list);
    auto i = k;
    while (i--) {
        iter.Seek(i);
        EXPECT_TRUE(iter.Valid());
        EXPECT_EQ(i, iter.key());
    }
}

TEST_F(SkipListTest, ThreadingPut) {
    IntSkipList list([](int a, int b) { return a - b; }, arena_.get());
    std::mutex m;

    auto Putter = [&] (SkipListTest::IntSkipList *list, int start, int end) {

        for (auto i = start; i < end; ++i) {
            std::unique_lock<std::mutex> lock(m);
            list->Put(std::move(i));
        }
    };

    std::thread threads[] = {
        std::thread(Putter, &list, 0,     10000),
        std::thread(Putter, &list, 10000, 20000),
        std::thread(Putter, &list, 20000, 30000),
        std::thread(Putter, &list, 30000, 40000),
    };

    for (auto &thread : threads) {
        thread.join();
    }

    IntSkipList::Iterator iter(&list);
    auto i = 40000;
    while (i--) {
        iter.Seek(i);
        EXPECT_TRUE(iter.Valid());
        EXPECT_EQ(i, iter.key());
    }
}

//TEST_F(SkipListTest, ChunkPut) {
//
//    auto comparator = [] (const lsm::InternalKey &a, const lsm::InternalKey &b) {
//        return BytewiseCompartor()->Compare(a.user_key_slice(), b.user_key_slice());
//    };
//
//    typedef util::SkipList<lsm::InternalKey,
//                    std::function<int(const lsm::InternalKey &, const lsm::InternalKey &)>> Table;
//    Table list(comparator);
//
//    lsm::InternalKey key[] = {
//        lsm::InternalKey::CreateKey("a", "1", 100, lsm::kFlagValue),
//        lsm::InternalKey::CreateKey("b", "2", 200, lsm::kFlagValue),
//        lsm::InternalKey::CreateKey("c", "3", 300, lsm::kFlagValue),
//    };
//
//    for (lsm::InternalKey &k : key) {
//        list.Put(std::move(k));
//    }
//
//    Table::Iterator iter(&list);
//    iter.SeekToFirst();
//    EXPECT_TRUE(iter.Valid());
//    EXPECT_EQ("a", iter.key().user_key_slice().ToString());
//    EXPECT_EQ("1", iter.key().value_slice().ToString());
//    EXPECT_EQ(100ULL, iter.key().tag().version);
//
//    iter.Next();
//    EXPECT_TRUE(iter.Valid());
//    EXPECT_EQ("b", iter.key().user_key_slice().ToString());
//    EXPECT_EQ("2", iter.key().value_slice().ToString());
//    EXPECT_EQ(200ULL, iter.key().tag().version);
//
//    iter.Next();
//    EXPECT_TRUE(iter.Valid());
//    EXPECT_EQ("c", iter.key().user_key_slice().ToString());
//    EXPECT_EQ("3", iter.key().value_slice().ToString());
//    EXPECT_EQ(300ULL, iter.key().tag().version);
//}

} // namespace core

} // namespace mai
