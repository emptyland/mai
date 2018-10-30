#include "core/hash-map.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace core {
    
struct TestComparator {
    bool Equals(int lhs, int rhs) const {
        return lhs == rhs;
    }
    
    int Hash(int key) const {
        return key;
    }
};
    
TEST(HashMapTest, Sanity) {
    HashMap<int, TestComparator> map(1024, TestComparator());
    
    map.Put(0);
    map.Put(1);
    map.Put(2);
    map.Put(1024);
    
    int index;
    HashMap<int, TestComparator>::Node *node;
    std::tie(index, node) = map.Seek(0);
    
    ASSERT_EQ(0, index);
    ASSERT_EQ(0, node->key);
    
    std::tie(index, node) = map.Seek(1);
    ASSERT_EQ(1, index);
    ASSERT_EQ(1, node->key);
}
    
} // namespace core
    
} // namespace mai
