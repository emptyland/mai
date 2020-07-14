#include "lang/hir-schedule.h"
#include "lang/hir.h"
#include "base/arenas.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class HIRScheduleTest : public ::testing::Test {
public:
    HIRScheduleTest() : factory_(&arena_) {}
    
    void SetUp() final { graph_ = new (&arena_) HGraph(&arena_); }

    base::StandaloneArena arena_;
    HOperatorFactory factory_;
    HGraph *graph_ = nullptr;
}; // class HIRScheduleTest


TEST_F(HIRScheduleTest, Sanity) {
    HBasicBlock *block = new (&arena_) HBasicBlock(&arena_, HBasicBlock::Id::FromSize(1));
    EXPECT_EQ(1, block->id().index());
    EXPECT_EQ(0, block->predecessors_size());
    EXPECT_EQ(0, block->successors_size());
    EXPECT_EQ(0, block->nodes_size());
    EXPECT_FALSE(block->deferred());
}

TEST_F(HIRScheduleTest, AddNodeToBlock) {
    HBasicBlock *block = new (&arena_) HBasicBlock(&arena_, HBasicBlock::Id::FromSize(1));
    HNode *node = graph_->NewNode(factory_.Begin());
    block->AddNode(node);
    node = graph_->NewNode(factory_.End(1, 0), HTypes::Void, node);
    block->AddNode(node);
    EXPECT_EQ(2, block->nodes_size());
    EXPECT_EQ(node->input(0), block->node(0));
    EXPECT_EQ(node, block->node(1));
}

TEST_F(HIRScheduleTest, RemoveNodeFromBlock) {
    HBasicBlock *block = new (&arena_) HBasicBlock(&arena_, HBasicBlock::Id::FromSize(1));
    HNode *node = graph_->NewNode(factory_.Begin());
    block->AddNode(node);
    node = graph_->NewNode(factory_.Word32Constant(1));
    block->AddNode(node);
    node = graph_->NewNode(factory_.Word32Constant(2));
    block->AddNode(node);
    EXPECT_EQ(3, block->nodes_size());
    block->RemoveNode(1);
    EXPECT_EQ(2, block->nodes_size());
    EXPECT_EQ(HBegin, block->node(0)->opcode());
    EXPECT_EQ(HWord32Constant, block->node(1)->opcode());
    EXPECT_EQ(2, HOperatorWith<uint32_t>::Data(block->node(1)));
}

TEST_F(HIRScheduleTest, AddBlockToBlock) {
    HBasicBlock *block = new (&arena_) HBasicBlock(&arena_, HBasicBlock::Id::FromSize(1));
    HBasicBlock *succ = new (&arena_) HBasicBlock(&arena_, HBasicBlock::Id::FromSize(2));
    block->AddSuccessor(succ);
    HBasicBlock *pred = new (&arena_) HBasicBlock(&arena_, HBasicBlock::Id::FromSize(3));
    block->AddPredecessor(pred);
    EXPECT_EQ(1, block->successors_size());
    EXPECT_EQ(succ, block->successor(0));
    EXPECT_EQ(1, block->predecessors_size());
    EXPECT_EQ(pred, block->predecessor(0));
}

} // namespace lang

} // namespace mai
