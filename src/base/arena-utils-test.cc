#include "base/arena-utils.h"
#include "base/arenas.h"
#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace base {
    
class ArenaUtilsTest : public ::testing::Test {
public:
    ArenaUtilsTest()
        : arena_(env_->GetLowLevelAllocator()) {}
    
    Env *env_ = Env::Default();
    base::StandaloneArena arena_;
};
    
TEST_F(ArenaUtilsTest, ArenaVector) {
    
    ArenaVector<ArenaString *> v(3, &arena_);
    
    v[0] = ArenaString::New(&arena_, "aaa");
    v[1] = ArenaString::New(&arena_, "bbb");
    v[2] = ArenaString::New(&arena_, "ccc");
    
    ASSERT_EQ(3, v.size());
    ASSERT_EQ("aaa", v[0]->ToString());
    ASSERT_EQ("bbb", v[1]->ToString());
    ASSERT_EQ("ccc", v[2]->ToString());
}
    
TEST_F(ArenaUtilsTest, ArenaMap) {
    ArenaMap<const ArenaString *, int> m(&arena_);
    
    auto k1 = ArenaString::New(&arena_, "aaa");
    auto k2 = ArenaString::New(&arena_, "bbb");
    auto k3 = ArenaString::New(&arena_, "ccc");
    
    m.insert({k1, 1});
    m.insert({k2, 2});
    m.insert({k3, 3});
    
    ASSERT_EQ(1, m[k1]);
    ASSERT_EQ(2, m[k2]);
    ASSERT_EQ(3, m[k3]);
}
    
TEST_F(ArenaUtilsTest, ArenaHashMap) {
    ArenaUnorderedMap<const ArenaString *, int> m(&arena_);
    
    auto k1 = ArenaString::New(&arena_, "aaa");
    auto k2 = ArenaString::New(&arena_, "bbb");
    auto k3 = ArenaString::New(&arena_, "ccc");
    
    m.insert({k1, 1});
    m.insert({k2, 2});
    m.insert({k3, 3});
    
    ASSERT_EQ(1, m[k1]);
    ASSERT_EQ(2, m[k2]);
    ASSERT_EQ(3, m[k3]);
}
    
} // namespace base
    
} // namespace mai
