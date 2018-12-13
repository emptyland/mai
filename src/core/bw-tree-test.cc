#include "core/bw-tree.h"
#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace core {
    
class BwTreeTest : public ::testing::Test {
public:
    BwTreeTest() : arena_(env_->GetLowLevelAllocator()) {}

    struct IntComparator {
        int operator () (int a, int b) const { return a - b; }
    };
    
    using IntTree = BwTree<int, IntComparator>;

    Env *env_ = Env::Default();
    base::Arena arena_;
};
    
TEST_F(BwTreeTest, Sanity) {
    IntTree tree(IntComparator{}, 3, 6, &arena_);
    
    for (int i = 0; i < 8; ++i) {
        tree.Put(i);
    }
    auto root = tree.GetNode(tree.GetRootId());
    ASSERT_NE(nullptr, root);
    ASSERT_EQ(3, root->largest_key);
    ASSERT_EQ(3, root->smallest_key);
    ASSERT_EQ(1, root->size);
    
    IntTree::View view =tree.TEST_GetView(root, nullptr);
    ASSERT_EQ(root->size, view.size());
}
    
TEST_F(BwTreeTest, BaseLayout) {
    IntTree tree(IntComparator{}, 3, 6, &arena_);
    
    auto n = tree.NewBaseLine(0, 0, 0);
    ASSERT_EQ(n->pid, 0);
    ASSERT_EQ(n->size, 0);
    ASSERT_EQ(n->sibling, 0);
}
    
TEST_F(BwTreeTest, Iterator) {
    IntTree tree(IntComparator{}, 3, 6, &arena_);
    for (int i = 0; i < 8; ++i) {
        tree.Put(i);
    }
    
    IntTree::Iterator iter(&tree);
    iter.Seek(4);
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(4, iter.key());
}
    
} // namespace core
    
} // namespace mai
