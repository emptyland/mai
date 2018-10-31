#include "core/key-boundle.h"
#include "core/hash-map.h"
#include "gtest/gtest.h"

namespace {
    
using ::mai::core::KeyBoundle;
    
struct TestComparator {
    
    bool Equals(const KeyBoundle *lhs, const KeyBoundle *rhs) const {
        if (lhs == rhs) {
            return true;
        }
        return ::memcmp(lhs->key().data(), rhs->key().data(), lhs->key().size()) == 0;
    }
    
    int Hash(const KeyBoundle *key) const {
        int hash = 1315423911;
        for (auto s = key->key().begin(); s < key->key().end(); s++) {
            hash ^= ((hash << 5) + (*s) + (hash >> 2));
        }
        return hash;
    }
};
    
} // namespace

namespace mai {
    
namespace core {

TEST(UnorderedMemoryTableTest, KeyBoundleMap) {
    HashMap<KeyBoundle *, TestComparator> map(8, TestComparator{});
    map.Put(KeyBoundle::New("k1", "v1", 1, Tag::kFlagValue));
    map.Put(KeyBoundle::New("k1", "v2", 2, Tag::kFlagValue));
    map.Put(KeyBoundle::New("k2", "v2", 3, Tag::kFlagValue));
    map.Put(KeyBoundle::New("k3", "v3", 4, Tag::kFlagValue));
    map.Put(KeyBoundle::New("k4", "v4", 5, Tag::kFlagValue));
    ASSERT_EQ(5, map.items_count());
}

} // namespace core
    
} // namespace mai
