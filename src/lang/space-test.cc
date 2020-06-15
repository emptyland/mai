#include "lang/space.h"
#include "mai/env.h"
#include "gtest/gtest.h"
#include <thread>
#include <random>
#include <algorithm>

namespace mai {

namespace lang {

class SpaceTest : public ::testing::Test {
public:
    // Dummy

    Env *env_ = Env::Default();
    Allocator *lla_ = env_->GetLowLevelAllocator();
};


TEST_F(SpaceTest, NewSpaceAllocation) {
    std::unique_ptr<NewSpace> space(new NewSpace(lla_));
    auto err = space->Initialize(10 * base::kMB);
    ASSERT_TRUE(err.ok()) << err.ToString();
    
    auto rv = space->Allocate(1);
    ASSERT_TRUE(rv.ok());
    ASSERT_EQ(0, reinterpret_cast<uintptr_t>(rv.address()) % kAligmentSize);
    ASSERT_EQ(kMinAllocationSize, space->GetAllocatedSize(rv.address()));
    
    auto addr = rv.address();
    rv = space->Allocate(1);
    ASSERT_EQ(kMinAllocationSize, rv.address() - addr);
}

TEST_F(SpaceTest, NewSpaceIterator0) {
    std::unique_ptr<NewSpace> space(new NewSpace(lla_));
    auto err = space->Initialize(10 * base::kMB);
    ASSERT_TRUE(err.ok()) << err.ToString();
    
    auto rv = space->Allocate(1);
    ASSERT_TRUE(rv.ok());
    
    SemiSpace::Iterator iter(space->original_area());
    iter.SeekToFirst();
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(rv.address(), iter.address());
    ASSERT_EQ(kMinAllocationSize, iter.object_size());
    
    iter.Next();
    ASSERT_FALSE(iter.Valid());
}

TEST_F(SpaceTest, NewSpaceIterator) {
    std::unique_ptr<NewSpace> space(new NewSpace(lla_));
    auto err = space->Initialize(10 * base::kMB);
    ASSERT_TRUE(err.ok()) << err.ToString();
    
    int n = 0;
    for (;;) {
        auto rv = space->Allocate(16 * base::kKB);
        if (!rv.ok()) {
            break;
        }
        ::memcpy(rv.address(), &n, sizeof(n));
        n++;
    }
    ASSERT_EQ(630, n);
    
    n = 0;
    SemiSpace::Iterator iter(space->original_area());
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        ASSERT_EQ(16 * base::kKB, iter.object_size()) << "err on:" << n;
        int i;
        ::memcpy(&i, iter.address(), sizeof(i));
        ASSERT_EQ(n++, i);
    }
    ASSERT_EQ(630, n);
}

TEST_F(SpaceTest, NewSpaceConcurrentAllocation) {
    std::unique_ptr<NewSpace> space(new NewSpace(lla_));
    auto err = space->Initialize(100 * base::kMB);
    ASSERT_TRUE(err.ok()) << err.ToString();

    std::thread t1([this, &space]() {
        for (int i = 0; i < 10000; i++) {
            auto rv = space->Allocate(16);
            ASSERT_TRUE(rv.ok());
            *reinterpret_cast<int *>(rv.address()) = 1;
            *reinterpret_cast<int *>(rv.address() + 4) = i;
        }
    });
    
    std::thread t2([this, &space]() {
        for (int i = 0; i < 10000; i++) {
            auto rv = space->Allocate(24);
            ASSERT_TRUE(rv.ok());
            *reinterpret_cast<int *>(rv.address()) = 2;
            *reinterpret_cast<int *>(rv.address() + 4) = i;
        }
    });

    std::thread t3([this, &space]() {
        for (int i = 0; i < 10000; i++) {
            auto rv = space->Allocate(32);
            ASSERT_TRUE(rv.ok());
            *reinterpret_cast<int *>(rv.address()) = 3;
            *reinterpret_cast<int *>(rv.address() + 4) = i;
        }
    });
    
    for (int i = 0; i < 10000; i++) {
        auto rv = space->Allocate(40);
        ASSERT_TRUE(rv.ok());
        *reinterpret_cast<int *>(rv.address()) = 4;
        *reinterpret_cast<int *>(rv.address() + 4) = i;
    }
    
    t1.join();
    t2.join();
    t3.join();
    
    int records[4] = {0};
    SemiSpace::Iterator iter(space->original_area());
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        switch (iter.object_size()) {
            case 16:
                ASSERT_EQ(1, *reinterpret_cast<int *>(iter.address()));
                records[0]++;
                break;
            case 24:
                ASSERT_EQ(2, *reinterpret_cast<int *>(iter.address()));
                records[1]++;
                break;
            case 32:
                ASSERT_EQ(3, *reinterpret_cast<int *>(iter.address()));
                records[2]++;
                break;
            case 40:
                ASSERT_EQ(4, *reinterpret_cast<int *>(iter.address()));
                records[3]++;
                break;
            default:
                break;
        }
    }
    ASSERT_EQ(records[0], 10000);
    ASSERT_EQ(records[1], 10000);
    ASSERT_EQ(records[2], 10000);
    ASSERT_EQ(records[3], 10000);
}


TEST_F(SpaceTest, OldSpaceAllocation) {
    std::unique_ptr<OldSpace> space(new OldSpace(lla_));
    ASSERT_EQ(0, space->allocated_pages());
    ASSERT_EQ(0, space->used_size());
    
    auto rv = space->Allocate(1);
    ASSERT_TRUE(rv.ok());
    ASSERT_EQ(0, reinterpret_cast<uintptr_t>(rv.address()) % kAligmentSize);
    ASSERT_EQ(1, space->allocated_pages());
    ASSERT_EQ(kMinAllocationSize, space->used_size());
}

TEST_F(SpaceTest, OldSpaceFree) {
    std::unique_ptr<OldSpace> space(new OldSpace(lla_));
    
    auto rv = space->Allocate(1);
    ASSERT_TRUE(rv.ok());
    ASSERT_EQ(1, space->allocated_pages());
    ASSERT_EQ(kMinAllocationSize, space->used_size());

    space->Free(rv.address(), true);
    ASSERT_EQ(1, space->allocated_pages());
    ASSERT_EQ(0, space->used_size());
    
    rv = space->Allocate(1);
    ASSERT_TRUE(rv.ok());
    ASSERT_EQ(1, space->allocated_pages());
    ASSERT_EQ(kMinAllocationSize, space->used_size());
}

TEST_F(SpaceTest, OldSpaceIterate) {
    std::unique_ptr<OldSpace> space(new OldSpace(lla_));
    
    auto rv = space->Allocate(1);
    auto v1 = rv.address();
    rv = space->Allocate(32);
    auto v2 = rv.address();
    rv = space->Allocate(64);
    auto v3 = rv.address();
    
    OldSpace::Iterator iter(space.get());
    iter.SeekToFirst();
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(v1, iter.address());
    ASSERT_EQ(16, iter.object_size());
    
    iter.Next();
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(v2, iter.address());
    ASSERT_EQ(32, iter.object_size());
    
    iter.Next();
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(v3, iter.address());
    ASSERT_EQ(64, iter.object_size());
    
    iter.Next();
    ASSERT_FALSE(iter.Valid());
    
    space->Free(v2, true);
    OldSpace::Iterator iter2(space.get());
    iter2.SeekToFirst();
    ASSERT_TRUE(iter2.Valid());
    ASSERT_EQ(v1, iter2.address());
    
    iter2.Next();
    ASSERT_TRUE(iter2.Valid());
    ASSERT_EQ(v3, iter2.address());
    
    iter2.Next();
    ASSERT_FALSE(iter2.Valid());
}

TEST_F(SpaceTest, OldSpaceEmptyIterate) {
    std::unique_ptr<OldSpace> space(new OldSpace(lla_));
    OldSpace::Iterator iter1(space.get());
    iter1.SeekToFirst();
    ASSERT_FALSE(iter1.Valid());
    
    auto rv = space->Allocate(32);
    OldSpace::Iterator iter2(space.get());
    iter2.SeekToFirst();
    ASSERT_TRUE(iter2.Valid());
    ASSERT_EQ(rv.address(), iter2.address());
    iter2.Next();
    ASSERT_FALSE(iter2.Valid());
    
    space->Free(rv.address(), false/*merge*/);
    OldSpace::Iterator iter3(space.get());
    iter3.SeekToFirst();
    ASSERT_FALSE(iter3.Valid());
}

TEST_F(SpaceTest, OldSpaceFuzzyAllocation) {
    static constexpr int N = 10000;
    std::unique_ptr<OldSpace> space(new OldSpace(lla_));
    std::vector<Address> chunks;
    for (int i = 0; i < N; i++) {
        auto rv = space->Allocate(256 + rand() % 256);
        ASSERT_TRUE(rv.ok());
        chunks.push_back(rv.address());
    }
    ASSERT_LT(512, space->used_size());
    ASSERT_LT(1, space->allocated_pages());
    ASSERT_EQ(0, space->freed_pages());
    
    OldSpace::Iterator iter(space.get());
    int count = 0;
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        ASSERT_LE(256, iter.object_size());
        count++;
    }
    ASSERT_EQ(N, count);
    
    for (auto chunk : chunks) {
        space->Free(chunk, true/*merge*/);
    }
    space->PurgeIfNeeded();
    ASSERT_LT(1, space->allocated_pages());
    ASSERT_EQ(space->allocated_pages(), space->freed_pages());
}

TEST_F(SpaceTest, OldSpaceFuzzyFree) {
    static constexpr int N = 100000;
    std::unique_ptr<OldSpace> space(new OldSpace(lla_));
    std::vector<Address> chunks;
    for (int i = 0; i < N; i++) {
        auto rv = space->Allocate(16 + rand() % 4096);
        ASSERT_TRUE(rv.ok());
        chunks.push_back(rv.address());
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(chunks.begin(), chunks.end(), g);
    
    for (int i = 0; i < N/2; i++) {
        space->Free(chunks[i], true/*merge*/);
    }
    OldSpace::Iterator iter(space.get());
    int count = 0;
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        ASSERT_LE(16, iter.object_size());
        count++;
    }
    ASSERT_EQ(N/2, count);
}

} // namespace lang

} // namespace mai
