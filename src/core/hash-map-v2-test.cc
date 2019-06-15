#include "core/hash-map-v2.h"
#include "base/arenas.h"
#include "mai/env.h"
#include "gtest/gtest.h"
#include <thread>

namespace mai {
    
namespace core {

class HashMapV2Test : public ::testing::Test {
public:
    HashMapV2Test() : arena_(Env::Default()->GetLowLevelAllocator()) {
        
    }
    
    struct IntCmp {
        int operator () (int a, int b) const {
            return a - b;
        }
        uint32_t operator () (int a) const {
            return a;
        }
    };
    
    base::StandaloneArena arena_;
};
    
TEST_F(HashMapV2Test, Sanity) {
    HashMap<int, IntCmp> m(17, IntCmp{}, &arena_);
    m.Put(1);
    m.Put(2);
    m.Put(3);
    
    HashMap<int, IntCmp>::Iterator iter(&m);
    iter.Seek(1);
    ASSERT_EQ(1, iter.key());
    
    iter.Seek(2);
    ASSERT_EQ(2, iter.key());
    
    iter.Seek(3);
    ASSERT_EQ(3, iter.key());
}
    
TEST_F(HashMapV2Test, ConcurrentGet) {
    static const auto kN = 100000;
    std::atomic<int> current(0);
    
    HashMap<int, IntCmp> m(kN, IntCmp{}, &arena_);
    
    std::thread reader([&]() {
        HashMap<int, IntCmp>::Iterator iter(&m);
        
        while (true) {
            auto n = current.load(std::memory_order_acquire);
            if (n == kN) {
                break;
            }
            if (n > 0) {
                for (int i = 0; i < 10; ++i) {
                    auto val = rand() % n;
                    iter.Seek(val);
                    ASSERT_TRUE(iter.Valid());
                    ASSERT_EQ(val, iter.key());
                }
            }
        }
    });
    for (int i = 0; i < kN; ++i) {
        m.Put(i);
        current.store(i, std::memory_order_release);
    }
    current.store(kN, std::memory_order_release);
    
    reader.join();
}
    
} // namespace core
    
} // namespace mai
