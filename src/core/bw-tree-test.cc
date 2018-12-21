#include "core/bw-tree.h"
#include "base/slice.h"
#include "mai/env.h"
#include "gtest/gtest.h"
#include <random>
#include <algorithm>
#include <thread>

namespace mai {
    
namespace core {
    
class BwTreeTest : public ::testing::Test {
public:
    BwTreeTest() {}

    struct IntComparator {
        int operator () (int a, int b) const { return a - b; }
    };
    
    struct FloatComparator {
        int operator () (float a, float b) const {
            if (a == b) {
                return 0;
            }
            return a < b ? -1 : 1;
        }
    };
    
    using IntTree   = BwTree<int, IntComparator>;
    using FloatTree = BwTree<float, FloatComparator>;

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
    
    n = tree.NewSplitNode(pid, 1, 200, 0, n);
    view = tree.TEST_MakeView(n, &overflow);
    EXPECT_EQ(2, overflow);
    ASSERT_EQ(1, view.size());
    EXPECT_EQ(1, view[100]);
    //EXPECT_EQ(2, view[200]);
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
    n = tree.NewSplitNode(pid, 2, 300, 0, n);
    auto view = tree.TEST_MakeView(n, &overflow);
    EXPECT_EQ(3, overflow);
    ASSERT_EQ(2, view.size());
    EXPECT_EQ(1, view[100]);
    EXPECT_EQ(2, view[200]);
}
    
TEST_F(BwTreeTest, SplitNode) {
    IntTree tree(IntComparator{}, 3, 6, env_);
    
    auto pid = tree.GetRootId();
    auto n = tree.GetNode(pid);
    
    n = tree.NewDeltaIndex(pid, 400, 40, 50, n);
    n = tree.NewDeltaIndex(pid, 300, 30, 40, n);
    n = tree.NewDeltaIndex(pid, 200, 20, 30, n);
    n = tree.NewDeltaIndex(pid, 100, 10, 20, n);
    
    auto s = tree.TEST_SpliInnter(n, 0);
    ASSERT_NE(nullptr, s);
    EXPECT_EQ(200, s->separator);
    EXPECT_EQ(1, s->size);
 
    // Left child node:
    bw::Pid overflow;
    auto view = tree.TEST_MakeView(s, &overflow);
    ASSERT_EQ(1, view.size());
    EXPECT_EQ(10, view[100]);
    EXPECT_EQ(20, overflow);
    auto lid = s->pid;
    
    // Right child node:
    n = tree.GetNode(s->splited);
    view = tree.TEST_MakeView(n, &overflow);
    ASSERT_EQ(2, view.size());
    EXPECT_EQ(30, view[300]);
    EXPECT_EQ(40, view[400]);
    EXPECT_EQ(50, overflow);
    auto rid = n->pid;
    
    // Root Node:
    n = tree.GetNode(tree.GetRootId());
    view = tree.TEST_MakeView(n, &overflow);
    ASSERT_EQ(1, view.size());
    EXPECT_EQ(lid, view[200]);
    EXPECT_EQ(rid, overflow);
}
    
TEST_F(BwTreeTest, SplitNode2) {
    IntTree tree(IntComparator{}, 3, 6, env_);
    
    auto pid = tree.GetRootId();
    auto n = tree.GetNode(pid);
    
    n = tree.NewDeltaIndex(pid, 400, 40, 50, n);
    n = tree.NewDeltaIndex(pid, 300, 30, 40, n);
    n = tree.NewDeltaIndex(pid, 200, 20, 30, n);
    n = tree.NewDeltaIndex(pid, 100, 10, 20, n);
    n = tree.NewDeltaIndex(pid, 500, 50, 60, n);
    
    auto s = tree.TEST_SpliInnter(n, 0);
    ASSERT_NE(nullptr, s);
    EXPECT_EQ(2, s->size);
    EXPECT_EQ(300, s->separator);
    
    
    // Left child node:
    bw::Pid overflow;
    auto view = tree.TEST_MakeView(s, &overflow);
    ASSERT_EQ(2, view.size());
    EXPECT_EQ(10, view[100]);
    EXPECT_EQ(20, view[200]);
    EXPECT_EQ(30, overflow);
    auto lid = s->pid;
    
    // Right child node:
    n = tree.GetNode(s->splited);
    view = tree.TEST_MakeView(n, &overflow);
    ASSERT_EQ(2, view.size());
    EXPECT_EQ(40, view[400]);
    EXPECT_EQ(50, view[500]);
    EXPECT_EQ(60, overflow);
    auto rid = n->pid;
    
    // Root Node:
    n = tree.GetNode(tree.GetRootId());
    view = tree.TEST_MakeView(n, &overflow);
    ASSERT_EQ(1, view.size());
    EXPECT_EQ(lid, view[300]);
    EXPECT_EQ(rid, overflow);
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

TEST_F(BwTreeTest, InnerLowerBound_V2) {
    IntTree tree(IntComparator{}, 3, 6, env_);

    auto node = tree.NewBaseLine(0, 3, 0);
    node->set_entry(0, {1, 10});
    node->set_entry(1, {3, 30});
    node->set_entry(2, {5, 50});
    node->overflow = 60;
    node->UpdateBound();

    auto n = tree.NewDeltaIndex(0, 2, 20, 31, node);
    n = tree.NewDeltaIndex(0, 4, 40, 51, n);
    n = tree.NewDeltaIndex(0, 6, 60, 71, n);

    bool found = false;
    bw::Pid value = tree.TEST_InnerFindGreaterOrEqual(n, 0, &found);
    EXPECT_TRUE(found);
    EXPECT_EQ(10, value);

    value = tree.TEST_InnerFindGreaterOrEqual(n, 1, &found);
    EXPECT_TRUE(found);
    EXPECT_EQ(10, value);

    value = tree.TEST_InnerFindGreaterOrEqual(n, 2, &found);
    EXPECT_TRUE(found);
    EXPECT_EQ(20, value);

    value = tree.TEST_InnerFindGreaterOrEqual(n, 5, &found);
    EXPECT_TRUE(found);
    EXPECT_EQ(51, value);

    value = tree.TEST_InnerFindGreaterOrEqual(n, 6, &found);
    EXPECT_TRUE(found);
    EXPECT_EQ(60, value);

    value = tree.TEST_InnerFindGreaterOrEqual(n, 7, &found);
    EXPECT_FALSE(found);
    EXPECT_EQ(71, value);
}
    
TEST_F(BwTreeTest, InnerLowerBound_V3) {
    IntTree tree(IntComparator{}, 3, 6, env_);
    
    auto node = tree.NewBaseLine(0, 3, 0);
    node->set_entry(0, {3, 30});
    node->set_entry(1, {4, 40});
    node->set_entry(2, {5, 50});
    node->overflow = 61;
    node->UpdateBound();
    
    auto n = tree.NewDeltaIndex(0, 7, 70, 81, node);
    n = tree.NewDeltaIndex(0, 6, 60, 71, n);
    n = tree.NewDeltaIndex(0, 2, 20, 31, n);
    n = tree.NewDeltaIndex(0, 1, 10, 21, n);
    
    bw::Pid overflow;
    auto view = tree.TEST_MakeView(n, &overflow);
    
    bool found = false;
    bw::Pid value = tree.TEST_InnerFindGreaterOrEqual(n, 0, &found);
    EXPECT_TRUE(found);
    EXPECT_EQ(view.lower_bound(0)->second, value);
    
    value = tree.TEST_InnerFindGreaterOrEqual(n, 3, &found);
    EXPECT_TRUE(found);
    EXPECT_EQ(31, value);
    
    for (int i = 1; i < 8; ++i) {
        value = tree.TEST_InnerFindGreaterOrEqual(n, i, &found);
        EXPECT_TRUE(found) << i;
        EXPECT_EQ(view[i], value) << i;
    }
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
    
TEST_F(BwTreeTest, PutMakeSplitLeaf) {
    IntTree tree(IntComparator{}, 3, 5, env_);
    
    for (int i = 0; i < 10; ++i) {
        tree.Put(i);
    }
    ASSERT_EQ(1, tree.GetLevel());
    
    IntTree::Iterator iter(&tree);
    for (int i = 0; i < 10; ++i) {
        iter.Seek(i);
        ASSERT_TRUE(iter.Valid());
        ASSERT_EQ(i, iter.key());
    }
    
    // Root node:
    auto root = tree.GetNode(tree.GetRootId());
    bw::Pid overflow;
    auto view = tree.TEST_MakeView(root, &overflow);
    ASSERT_EQ(2, view.size());
    EXPECT_EQ(1, view[2]);
    EXPECT_EQ(2, view[5]);
    EXPECT_EQ(4, overflow);
    
    // Left node:
    auto n = tree.GetNode(1);
    view = tree.TEST_MakeView(n, &overflow);
    ASSERT_EQ(2, view.size());
    EXPECT_TRUE(view.find(0) != view.end());
    EXPECT_TRUE(view.find(1) != view.end());
    
    // Middle node:
    n = tree.GetNode(2);
    view = tree.TEST_MakeView(n, &overflow);
    ASSERT_EQ(2, view.size());
    EXPECT_TRUE(view.find(3) != view.end());
    EXPECT_TRUE(view.find(4) != view.end());
    
    // Right node:
    n = tree.GetNode(4);
    view = tree.TEST_MakeView(n, &overflow);
    ASSERT_EQ(4, view.size());
    EXPECT_TRUE(view.find(6) != view.end());
    EXPECT_TRUE(view.find(7) != view.end());
    EXPECT_TRUE(view.find(8) != view.end());
    EXPECT_TRUE(view.find(9) != view.end());
}
    
TEST_F(BwTreeTest, PutMakeMoreSplit) {
    static const auto kN = 100;
    
    IntTree tree(IntComparator{}, 3, 5, env_);
    
    for (int i = 0; i < kN; ++i) {
        tree.Put(i);
    }
    ASSERT_EQ(3, tree.GetLevel());
    
    IntTree::Iterator iter(&tree);
    iter.Seek(8);
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(8, iter.key());
    
    iter.Seek(6);
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(6, iter.key());
    
    for (int i = 0; i < kN; ++i) {
        iter.Seek(i);
        ASSERT_TRUE(iter.Valid());
        ASSERT_EQ(i, iter.key());
    }
}

TEST_F(BwTreeTest, FuzzPut) {
    static const auto kN = 20000;
    
    std::vector<int> nums;
    for (int i = 0; i < kN; ++i) {
        nums.push_back(i);
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(nums.begin(), nums.end(), g);

    IntTree tree(IntComparator{}, 5, 64, env_);
    for (auto val : nums) {
        tree.Put(val);
    }
    
    //ASSERT_EQ(2, tree.GetLevel());
    
    IntTree::Iterator iter(&tree);
    iter.Seek(38);
    ASSERT_TRUE(iter.Valid());
    EXPECT_EQ(38, iter.key());
    
    iter.Seek(56);
    ASSERT_TRUE(iter.Valid());
    EXPECT_EQ(56, iter.key());
    for (int i = 0; i < kN; ++i) {
        iter.Seek(i);
        ASSERT_TRUE(iter.Valid());
        EXPECT_EQ(i, iter.key()) << "key: " << i;
    }
}
    
TEST_F(BwTreeTest, UpperBound) {
    static const auto kN = 1000;
    
    std::vector<float> nums;
    for (int i = 0; i < kN; ++i) {
        nums.push_back(i);
    }
    
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(nums.begin(), nums.end(), g);
    
    FloatTree tree(FloatComparator{}, 5, 64, env_);
    for (auto val : nums) {
        tree.Put(val);
    }
    
    FloatTree::Iterator iter(&tree);
    iter.Seek(0.0);
    ASSERT_TRUE(iter.Valid());
    EXPECT_EQ(0.0, iter.key());
    
    iter.Seek(0.5);
    ASSERT_TRUE(iter.Valid());
    EXPECT_EQ(1.0, iter.key());
    
    iter.Seek(99.9);
    ASSERT_TRUE(iter.Valid());
    EXPECT_EQ(100.0, iter.key());
    
    iter.Seek(100);
    ASSERT_TRUE(iter.Valid());
    EXPECT_EQ(100.0, iter.key());
    
    iter.Seek(999.9);
    ASSERT_FALSE(iter.Valid());
    
    iter.Seek(998.9);
    ASSERT_TRUE(iter.Valid());
    EXPECT_EQ(999.0, iter.key());
}
    
TEST_F(BwTreeTest, IteratorNext) {
    static const auto kN = 100;
    
    IntTree tree(IntComparator{}, 3, 5, env_);
    
    for (int i = 0; i < kN; ++i) {
        tree.Put(i);
    }
    ASSERT_EQ(3, tree.GetLevel());
    
    IntTree::Iterator iter(&tree);
    iter.Seek(8);
    iter.Next();
    ASSERT_TRUE(iter.Valid());
    EXPECT_EQ(9, iter.key());
    
    int n = 0;
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        EXPECT_EQ(n++, iter.key());
    }
}
    
TEST_F(BwTreeTest, IteratorPrev) {
    static const auto kN = 100;
    
    IntTree tree(IntComparator{}, 3, 5, env_);
    
    for (int i = 0; i < kN; ++i) {
        tree.Put(i);
    }
    ASSERT_EQ(3, tree.GetLevel());

    IntTree::Iterator iter(&tree);
    iter.Seek(96);
    iter.Prev();
    ASSERT_TRUE(iter.Valid());
    EXPECT_EQ(95, iter.key());
    
    iter.Seek(95);
    iter.Prev();
    ASSERT_TRUE(iter.Valid());
    EXPECT_EQ(94, iter.key());
    
    iter.Seek(89);
    iter.Prev();
    ASSERT_TRUE(iter.Valid());
    EXPECT_EQ(88, iter.key());
    
    iter.Seek(0);
    iter.Prev();
    ASSERT_FALSE(iter.Valid());
    
    int n = kN;
    for (iter.SeekToLast(); iter.Valid(); iter.Prev()) {
        EXPECT_EQ(--n, iter.key());
    }
}
    
TEST_F(BwTreeTest, FuzzIterator) {
    static const auto kN = 2000;
    
    std::vector<int> nums;
    for (int i = 0; i < kN; ++i) {
        nums.push_back(i);
    }
    
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(nums.begin(), nums.end(), g);
    
    IntTree tree(IntComparator{}, 3, 64, env_);
    for (auto val : nums) {
        tree.Put(val);
    }
    
    int n = kN;
    IntTree::Iterator iter(&tree);
    for (iter.SeekToLast(); iter.Valid(); iter.Prev()) {
        ASSERT_EQ(--n, iter.key());
    }
    
    n = 0;
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        EXPECT_EQ(n++, iter.key());
    }
}
    
TEST_F(BwTreeTest, ConcurrentPutGet) {
    static const auto kN = 1000;
    
    std::atomic<int> current(0);
    
    IntTree tree(IntComparator{}, 3, 128, env_);
    //tree.ReaderRegister();
    
    std::thread wr_thrd([&current, &tree]() {
        for (int i = 0; i < kN; ++i) {
            tree.Put(i);
            current.store(i, std::memory_order_release);
        }
    });
    
    IntTree::Iterator iter(&tree);
    
    while (true) {
        auto n = current.load(std::memory_order_acquire);
        if (n > 0) {

            iter.Seek(n);
            EXPECT_TRUE(iter.Valid()) << n;
            if (iter.Valid()) {
                EXPECT_EQ(n, iter.key());
            }

            for (int i = 0; i < 10; ++i) {
                auto k = rand() % n;
                iter.Seek(k);
                EXPECT_TRUE(iter.Valid()) << n << "," << k;
            }
        }
        
        if (n == kN - 1) {
            break;
        }
    }

    wr_thrd.join();
    
    for (int i = 0; i < kN; ++i) {
        iter.Seek(i);
        ASSERT_TRUE(iter.Valid());
        EXPECT_EQ(i, iter.key());
    }
}
    
} // namespace core
    
} // namespace mai
