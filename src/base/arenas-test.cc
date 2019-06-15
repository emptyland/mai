#include "base/arenas.h"
#include "base/zone.h"
#include "mai/env.h"
#include "gtest/gtest.h"
#include <thread>

namespace mai {
    
namespace base {
    
class ArenaTest : public ::testing::Test {
public:
    ArenaTest()
        : zone_(env_->GetLowLevelAllocator(), 1 * base::kMB) {}

    Env *env_ = Env::Default();
    Zone zone_;
}; // class ArenaTest

TEST_F(ArenaTest, Sanity) {
    auto allocator = env_->GetLowLevelAllocator();
    auto raw = allocator->Allocate(4096, 0);
    memset(raw, 0xcc, 4096);
    allocator->Free(raw, 4096);
    
    StandaloneArena arena(allocator);
    
    ASSERT_NE(nullptr, arena.Allocate(1024, 4));
    ASSERT_NE(nullptr, arena.Allocate(1024, 4));
}
    
TEST_F(ArenaTest, FuzzAllocation) {
    StandaloneArena arena(env_->GetLowLevelAllocator());
    for (int i = 0; i < 10240; ++i) {
        size_t size = abs(::rand()) % base::kKB;
        arena.Allocate(size, 4);
    }
}
    
TEST_F(ArenaTest, ConcurrentLargeAllocation) {
    StandaloneArena arena(env_->GetLowLevelAllocator());
    
    std::thread worker_thrds[4];
    
    for (int i = 0; i < arraysize(worker_thrds); ++i) {
        worker_thrds[i] = std::thread([&](){
            for (int j = 0; j < 1024; j++) {
                arena.NewLarge(1024, 4);
            }
        });
    }
    for (int i = 0; i < arraysize(worker_thrds); ++i) {
        worker_thrds[i].join();
    }
}
    
TEST_F(ArenaTest, ConcurrentNormalAllocation) {
    StandaloneArena arena(env_->GetLowLevelAllocator());
    
    std::thread worker_thrds[4];
    
    for (int i = 0; i < arraysize(worker_thrds); ++i) {
        worker_thrds[i] = std::thread([&](){
            for (int j = 0; j < 1024; j++) {
                arena.NewNormal(1024, 4);
            }
        });
    }
    for (int i = 0; i < arraysize(worker_thrds); ++i) {
        worker_thrds[i].join();
    }
}
    
TEST_F(ArenaTest, CocurrentFuzzAllocation) {
    StandaloneArena arena(env_->GetLowLevelAllocator());
    
    std::thread worker_thrds[8];
    
    for (int i = 0; i < arraysize(worker_thrds); ++i) {
        worker_thrds[i] = std::thread([&](){
            for (int j = 0; j < 10240; j++) {
                size_t size = abs(::rand()) % (3 * base::kKB);
                arena.Allocate(size, 4);
            }
        });
    }
    for (int i = 0; i < 1024; ++i) {
        auto memory_usage = arena.memory_usage();
        DCHECK_EQ(0, memory_usage % StandaloneArena::kPageSize);
    }
    for (int i = 0; i < arraysize(worker_thrds); ++i) {
        worker_thrds[i].join();
    }
    
//    std::vector<Arena::Statistics> r1, r2;
//    arena.GetUsageStatistics(&r1, &r2);
//
//    for (const auto &s : r1) {
//        printf("%lu %f\n", s.usage, s.used_rate);
//    }
//    printf("----------------large----------------\n");
//    for (const auto &s : r2) {
//        printf("%lu %f\n", s.usage, s.used_rate);
//    }
}
    
TEST_F(ArenaTest, ZoneSanity) {
    auto p = zone_.Allocate(4);
    *static_cast<int32_t *>(p) = 991;
}

    
TEST_F(ArenaTest, ZoneNewArena) {
    std::unique_ptr<Arena> arena(zone_.NewArena(0));
    ASSERT_NE(nullptr, arena.get());
    
    auto p = arena->Allocate(4, 8);
    *static_cast<int32_t *>(p) = 991;
    ASSERT_EQ(0, reinterpret_cast<uintptr_t>(p) % 8);
    
    auto x = static_cast<char *>(arena->Allocate(3, 8));
    x[0] = 1;
    x[1] = 2;
    x[2] = 3;
    ASSERT_EQ(x, static_cast<char *>(p) + 8);
    ASSERT_EQ(0, reinterpret_cast<uintptr_t>(x) % 8);
    
    p = x;
    x = static_cast<char *>(arena->Allocate(1, 8));
    x[0] = 4;
    ASSERT_EQ(x, static_cast<char *>(p) + 8);
    ASSERT_EQ(0, reinterpret_cast<uintptr_t>(x) % 8);
}
    
TEST_F(ArenaTest, ScopedSanity) {
    ScopedArena arena;
    
    uint8_t *p = static_cast<uint8_t *>(arena.Allocate(12));
    uint8_t *x = static_cast<uint8_t *>(arena.Allocate(4));
    ASSERT_EQ(12, x - p);
    
    p = static_cast<uint8_t *>(arena.Allocate(4));
    ASSERT_EQ(4, p - x);
}

TEST_F(ArenaTest, ScopedLargeAllocation) {
    ScopedArena arena;
    
    uint8_t *p = static_cast<uint8_t *>(arena.Allocate(12));
    ASSERT_NE(nullptr, p);
    ASSERT_TRUE(arena.chunks_.empty());
    
    p = static_cast<uint8_t *>(arena.Allocate(arraysize(arena.buf_) + 1));
    ASSERT_NE(nullptr, p);
    ASSERT_EQ(1, arena.chunks_.size());
    ASSERT_EQ(p, arena.chunks_[0]);
}

TEST_F(ArenaTest, ScopedTotalBufAllocation) {
    ScopedArena arena;
    
    uint8_t *p = static_cast<uint8_t *>(arena.Allocate(arraysize(arena.buf_)));
    ASSERT_NE(nullptr, p);
    ASSERT_TRUE(arena.chunks_.empty());
    ASSERT_EQ(p, arena.buf_);
    
    p = static_cast<uint8_t *>(arena.Allocate(4));
    ASSERT_NE(nullptr, p);
    ASSERT_EQ(1, arena.chunks_.size());
    ASSERT_EQ(p, arena.chunks_[0]);
}

TEST_F(ArenaTest, StaticSanity) {
    StaticArena<> arena;
    
    uint8_t *p = static_cast<uint8_t *>(arena.Allocate(12, 4));
    ASSERT_EQ(0, reinterpret_cast<uintptr_t>(p) % 4);
    uint8_t *x = static_cast<uint8_t *>(arena.Allocate(4, 4));
    ASSERT_EQ(0, reinterpret_cast<uintptr_t>(x) % 4);
    ASSERT_EQ(12, x - p);
    
    p = static_cast<uint8_t *>(arena.Allocate(4));
    ASSERT_EQ(4, p - x);
    
    x = static_cast<uint8_t *>(arena.Allocate(1, 8));
    ASSERT_EQ(0, reinterpret_cast<uintptr_t>(x) % 8);
}

TEST_F(ArenaTest, StaticOverflowAllocation) {
    StaticArena<> arena;
    
    uint8_t *p = static_cast<uint8_t *>(arena.Allocate(StaticArena<>::kLimitSize));
    ASSERT_NE(nullptr, p);
    
    uint8_t *x = static_cast<uint8_t *>(arena.Allocate(1));
    ASSERT_EQ(nullptr, x);
}
    
} // namespace base
    
} // namespace mai
