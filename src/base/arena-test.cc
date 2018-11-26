#include "base/arena.h"
#include "mai/env.h"
#include "gtest/gtest.h"
#include <thread>

namespace mai {
    
namespace base {
    
class ArenaTest : public ::testing::Test {
public:
    

    Env *env_ = Env::Default();
}; // class ArenaTest

TEST_F(ArenaTest, Sanity) {
    auto allocator = env_->GetLowLevelAllocator();
    auto raw = allocator->Allocate(4096, 0);
    memset(raw, 0xcc, 4096);
    allocator->Free(raw, 4096);
    
    Arena arena(allocator);
    
    ASSERT_NE(nullptr, arena.Allocate(1024, 4));
    ASSERT_NE(nullptr, arena.Allocate(1024, 4));
}
    
TEST_F(ArenaTest, CocurrentLargeAllocation) {
    Arena arena(env_->GetLowLevelAllocator());
    
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
    
TEST_F(ArenaTest, CocurrentNormalAllocation) {
    Arena arena(env_->GetLowLevelAllocator());
    
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
    
TEST_F(ArenaTest, FuzzAllocation) {
    Arena arena(env_->GetLowLevelAllocator());
    
    std::thread worker_thrds[4];
    
    for (int i = 0; i < arraysize(worker_thrds); ++i) {
        worker_thrds[i] = std::thread([&](){
            for (int j = 0; j < 1024; j++) {
                size_t size = abs(::rand()) % base::kMB;
                arena.Allocate(size, 4);
            }
        });
    }
    for (int i = 0; i < arraysize(worker_thrds); ++i) {
        worker_thrds[i].join();
    }
}
    
} // namespace base
    
} // namespace mai
