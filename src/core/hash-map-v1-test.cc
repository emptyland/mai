#include "core/hash-map-v1.h"
#include "gtest/gtest.h"

namespace {

struct TestComparator {
    int operator ()(int key) const { return key; }
    int operator ()(int lhs, int rhs) const {
        if (lhs < rhs) {
            return -1;
        } else if (lhs > rhs) {
            return 1;
        }
        return 0;
    }
};
    
} // namespace

namespace mai {
    
namespace core {
    
TEST(HashMapTest, Sanity) {
    HashMap<int, TestComparator> map(1024, TestComparator());
    
    map.Put(0);
    map.Put(1);
    map.Put(2);
    map.Put(1024);
    
    HashMap<int, TestComparator>::Iterator iter(&map);
    iter.Seek(0);
    ASSERT_EQ(0, iter.key());
    
    iter.Seek(1);
    ASSERT_EQ(1, iter.key());
}
    
TEST(HashMapTest, Iterator) {
    HashMap<int, TestComparator> map(1024, TestComparator());
    
    map.Put(0);
    map.Put(1);
    map.Put(2);
    map.Put(1024);
    
    HashMap<int, TestComparator>::Iterator iter(&map);
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        //printf("%d\n", iter.key());
    }
}
    
} // namespace core
    
} // namespace mai
