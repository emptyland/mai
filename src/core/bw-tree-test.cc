#include "core/bw-tree.h"
#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace core {
    
class BwTreeTest : public ::testing::Test {
public:
    BwTreeTest() {}

    struct IntComparator {
        int operator () (int a, int b) const { return a - b; }
    };
    
    using IntTree = BwTree<int, IntComparator>;

    Env *env_ = Env::Default();
};
    
TEST_F(BwTreeTest, Sanity) {
    IntTree tree(IntComparator{}, 3, 6, env_);
    
    for (int i = 0; i < 8; ++i) {
        tree.Put(i);
    }
    auto root = tree.GetNode(tree.GetRootId());
    ASSERT_NE(nullptr, root);
    EXPECT_EQ(1, root->size);
    
    IntTree::View view =tree.TEST_MakeView(root, nullptr);
    ASSERT_EQ(root->size, view.size());
}
    
TEST_F(BwTreeTest, BaseLayout) {
    IntTree tree(IntComparator{}, 3, 6, env_);
    
    auto n = tree.NewBaseLine(0, 0, 0);
    ASSERT_EQ(n->pid, 0);
    ASSERT_EQ(n->size, 0);
    ASSERT_EQ(n->sibling, 0);
}
    
TEST_F(BwTreeTest, MakeView) {
    IntTree tree(IntComparator{}, 3, 6, env_);
    
    bw::Pid pid = tree.GeneratePid();
    bw::DeltaNode<int> *n = tree.NewBaseLine(pid, 0, 0);
    ASSERT_EQ(n->size, 0);
    ASSERT_EQ(n->sibling, 0);
    
    n = tree.NewDeltaKey(pid, 100, n);
    ASSERT_EQ(n->smallest_key, 100);
    ASSERT_EQ(n->largest_key, 100);
    
    n = tree.NewDeltaKey(pid, 99, n);
    ASSERT_EQ(n->smallest_key, 99);
    ASSERT_EQ(n->largest_key, 100);
    
    bw::Pid overflow;
    auto view = tree.TEST_MakeView(n, &overflow);
    ASSERT_EQ(0, overflow);
    ASSERT_EQ(2, view.size());
    ASSERT_TRUE(view.find(99) != view.end());
    ASSERT_TRUE(view.find(100) != view.end());
}
    
TEST_F(BwTreeTest, MakeView2) {
    IntTree tree(IntComparator{}, 3, 6, env_);
    
    bw::Pid pid = tree.GeneratePid();
    bw::DeltaNode<int> *n = tree.NewBaseLine(pid, 3, 0);
    ASSERT_EQ(n->size, 3);
    ASSERT_EQ(n->sibling, 0);
    
    auto bl = bw::BaseLine<int>::Cast(n);
    bl->set_entry(0, {100, 1});
    bl->set_entry(1, {200, 2});
    bl->set_entry(2, {300, 3});
    bl->overflow = 4;
    bl->UpdateBound();
    
    n = tree.NewDeltaIndex(pid, 400, 5, 6, n);
    ASSERT_EQ(n->size, 4);
    ASSERT_EQ(n->smallest_key, 100);
    ASSERT_EQ(n->largest_key, 400);
    
    bw::Pid overflow;
    auto view = tree.TEST_MakeView(n, &overflow);
    ASSERT_EQ(6, overflow);
    ASSERT_EQ(4, view.size());
    ASSERT_EQ(1, view[100]);
    ASSERT_EQ(2, view[200]);
    ASSERT_EQ(3, view[300]);
    ASSERT_EQ(5, view[400]);
}
    
TEST_F(BwTreeTest, MakeView3) {
    IntTree tree(IntComparator{}, 3, 6, env_);
    
    bw::Pid pid = tree.GeneratePid();
    bw::DeltaNode<int> *n = tree.NewBaseLine(pid, 3, 0);
    EXPECT_EQ(n->size, 3);
    EXPECT_EQ(n->sibling, 0);
    
    auto bl = bw::BaseLine<int>::Cast(n);
    bl->set_entry(0, {200, 2});
    bl->set_entry(1, {300, 3});
    bl->set_entry(2, {400, 4});
    bl->overflow = 5;
    bl->UpdateBound();
    
    n = tree.NewDeltaIndex(pid, 100, 1, 2, n);
    EXPECT_EQ(n->size, 4);
    EXPECT_EQ(n->smallest_key, 100);
    EXPECT_EQ(n->largest_key, 400);
    
    bw::Pid overflow;
    auto view = tree.TEST_MakeView(n, &overflow);
    EXPECT_EQ(5, overflow);
    ASSERT_EQ(4, view.size());
    EXPECT_EQ(1, view[100]);
    EXPECT_EQ(2, view[200]);
    EXPECT_EQ(3, view[300]);
    EXPECT_EQ(4, view[400]);
}
    
TEST_F(BwTreeTest, MakeView4) {
    IntTree tree(IntComparator{}, 3, 6, env_);
    
    bw::Pid pid = tree.GeneratePid();
    bw::DeltaNode<int> *n = tree.NewBaseLine(pid, 1, 0);
    EXPECT_EQ(n->size, 1);
    
    auto bl = bw::BaseLine<int>::Cast(n);
    bl->set_entry(0, {300, 3});
    bl->sibling = 99;
    bl->UpdateBound();
    
    n = tree.NewDeltaIndex(pid, 100, 1, 2, n);
    n = tree.NewDeltaIndex(pid, 400, 4, 5, n);
    n = tree.NewDeltaIndex(pid, 200, 2, 3, n);
    
    ASSERT_EQ(4, n->size);
    EXPECT_EQ(100, n->smallest_key);
    EXPECT_EQ(400, n->largest_key);
    
    bw::Pid overflow;
    auto view = tree.TEST_MakeView(n, &overflow);
    EXPECT_EQ(5, overflow);
    ASSERT_EQ(4, view.size());
    EXPECT_EQ(1, view[100]);
    EXPECT_EQ(2, view[200]);
    EXPECT_EQ(3, view[300]);
    EXPECT_EQ(4, view[400]);
    
    n = tree.NewSplitNode(pid, 200, 0, n);
    view = tree.TEST_MakeView(n, &overflow);
    EXPECT_EQ(3, overflow);
    ASSERT_EQ(2, view.size());
    EXPECT_EQ(1, view[100]);
    EXPECT_EQ(2, view[200]);
}
    
TEST_F(BwTreeTest, SplitLeafNode) {
    IntTree tree(IntComparator{}, 3, 6, env_);
    
    auto pid = tree.GetRootId();
    auto n = tree.GetNode(pid);
    
    n = tree.NewDeltaKey(pid, 100, n);
    n = tree.NewDeltaKey(pid, 200, n);
    n = tree.NewDeltaKey(pid, 300, n);
    n = tree.NewDeltaKey(pid, 400, n);
    
    auto s = tree.TEST_SplitLeaf(n, 0);
    ASSERT_NE(nullptr, s);
    EXPECT_EQ(200, s->separator);
    
    bw::Pid overflow;
    auto view = tree.TEST_MakeView(s, &overflow);
    ASSERT_EQ(2, view.size());
    EXPECT_TRUE(view.find(100) != view.end());
    EXPECT_TRUE(view.find(200) != view.end());
    
    ASSERT_NE(pid, tree.GetRootId());
    
    // New root node
    pid = tree.GetRootId();
    n = tree.GetNode(pid);
    view = tree.TEST_MakeView(n, &overflow);
    ASSERT_EQ(1, view.size());
    EXPECT_EQ(2, overflow);
    EXPECT_EQ(1, view[200]);
    
    // Left child node:
    view = tree.TEST_MakeView(tree.GetNode(1), &overflow);
    ASSERT_EQ(2, view.size());
    EXPECT_TRUE(view.find(100) != view.end());
    EXPECT_TRUE(view.find(200) != view.end());
    
    // Right child node:
    view = tree.TEST_MakeView(tree.GetNode(2), &overflow);
    ASSERT_EQ(2, view.size());
    EXPECT_TRUE(view.find(300) != view.end());
    EXPECT_TRUE(view.find(400) != view.end());
}
    
TEST_F(BwTreeTest, SplitLeafNode2) {
    IntTree tree(IntComparator{}, 3, 6, env_);
    
    auto pid = tree.GetRootId();
    auto n = tree.GetNode(pid);
    
    n = tree.NewDeltaKey(pid, 100, n);
    n = tree.NewDeltaKey(pid, 200, n);
    n = tree.NewDeltaKey(pid, 300, n);
    n = tree.NewDeltaKey(pid, 400, n);
    n = tree.NewDeltaKey(pid, 500, n);
    
    auto s = tree.TEST_SplitLeaf(n, 0);
    ASSERT_NE(nullptr, s);
    EXPECT_EQ(300, s->separator);
    
    bw::Pid overflow;
    auto view = tree.TEST_MakeView(s, &overflow);
    ASSERT_EQ(3, view.size());
    EXPECT_TRUE(view.find(100) != view.end());
    EXPECT_TRUE(view.find(200) != view.end());
    EXPECT_TRUE(view.find(300) != view.end());
    
    ASSERT_NE(pid, tree.GetRootId());
    // New root node
    pid = tree.GetRootId();
    n = tree.GetNode(pid);
    view = tree.TEST_MakeView(n, &overflow);
    ASSERT_EQ(1, view.size());
    EXPECT_EQ(2, overflow);
    EXPECT_EQ(1, view[300]);
    
    // Left child node:
    view = tree.TEST_MakeView(tree.GetNode(1), &overflow);
    ASSERT_EQ(3, view.size());
    EXPECT_TRUE(view.find(100) != view.end());
    EXPECT_TRUE(view.find(200) != view.end());
    EXPECT_TRUE(view.find(300) != view.end());
    
    // Right child node:
    view = tree.TEST_MakeView(tree.GetNode(2), &overflow);
    ASSERT_EQ(2, view.size());
    EXPECT_TRUE(view.find(400) != view.end());
    EXPECT_TRUE(view.find(500) != view.end());
}

TEST_F(BwTreeTest, MakeView5) {
    IntTree tree(IntComparator{}, 3, 6, env_);
    
    bw::Pid pid = tree.GeneratePid();
    bw::DeltaNode<int> *n = tree.NewBaseLine(pid, 4, 0);
    EXPECT_EQ(n->size, 4);
    
    auto bl = bw::BaseLine<int>::Cast(n);
    bl->set_entry(0, {100, 1});
    bl->set_entry(1, {200, 2});
    bl->set_entry(2, {300, 3});
    bl->set_entry(3, {400, 4});
    bl->sibling = 5;
    bl->UpdateBound();
    
    bw::Pid overflow;
    n = tree.NewSplitNode(pid, 300, 0, n);
    auto view = tree.TEST_MakeView(n, &overflow);
    EXPECT_EQ(4, overflow);
    ASSERT_EQ(3, view.size());
    EXPECT_EQ(1, view[100]);
    EXPECT_EQ(2, view[200]);
    EXPECT_EQ(3, view[300]);
}
    
TEST_F(BwTreeTest, LowerBoundBaseLine) {
    IntTree tree(IntComparator{}, 3, 6, env_);
    
    auto node = tree.NewBaseLine(0, 8, 0);
    for (int i = 0; i < node->size; ++i) {
        node->entries[i].key = i;
    }
    
    ASSERT_EQ(0, tree.TEST_FindGreaterOrEqual(node, 0));
    ASSERT_EQ(0, tree.TEST_FindGreaterOrEqual(node, -1));
    ASSERT_EQ(1, tree.TEST_FindGreaterOrEqual(node, 1));
    ASSERT_EQ(node->size, tree.TEST_FindGreaterOrEqual(node, 8));
    ASSERT_EQ(node->size, tree.TEST_FindGreaterOrEqual(node, 999));
    
    ::free(node);
}
    
TEST_F(BwTreeTest, LowerBoundBaseLine2) {
    IntTree tree(IntComparator{}, 3, 6, env_);
    
    auto node = tree.NewBaseLine(0, 8, 0);
    for (int i = 0; i < node->size; ++i) {
        node->entries[i].key = i * 3;
    }
    // 0 3 6 9 12 15 18 21
    
    ASSERT_EQ(0, tree.TEST_FindGreaterOrEqual(node, 0));
    ASSERT_EQ(0, tree.TEST_FindGreaterOrEqual(node, -1));
    ASSERT_EQ(1, tree.TEST_FindGreaterOrEqual(node, 1));
    ASSERT_EQ(3, tree.TEST_FindGreaterOrEqual(node, 8));
    ASSERT_EQ(7, tree.TEST_FindGreaterOrEqual(node, 19));
    ASSERT_EQ(node->size, tree.TEST_FindGreaterOrEqual(node, 22));
    ASSERT_EQ(node->size, tree.TEST_FindGreaterOrEqual(node, 999));
    
    ::free(node);
}
    
TEST_F(BwTreeTest, Iterator) {
    IntTree tree(IntComparator{}, 3, 6, env_);
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
