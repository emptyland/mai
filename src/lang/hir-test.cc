#include "lang/hir.h"
#include "base/arenas.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class HIRTest : public ::testing::Test {
public:
    HIRTest() : factory_(&arena_) {}
    
    base::StandaloneArena arena_;
    HOperatorFactory factory_;
};

TEST_F(HIRTest, Operators) {
    const HOperator *phi = factory_.Phi(2, 2);
    ASSERT_EQ(HPhi, phi->value());
    ASSERT_EQ(2, phi->control_in());
    ASSERT_EQ(2, phi->value_in());
    ASSERT_STREQ("Phi", phi->name());
    
    HNode *node = HNode::New(&arena_, 0, HTypes::Word8, phi, 0, 0, nullptr);
    ASSERT_EQ(HPhi, node->opcode());
}

TEST_F(HIRTest, UsedChain) {
    const HOperator *op = factory_.ConstantWord32(1);
    HNode *k1 = HNode::New(&arena_, 0, HTypes::Word32, op, 0, 0, nullptr);
    ASSERT_EQ(HConstant, k1->opcode());
    op = factory_.ConstantWord32(2);
    HNode *k2 = HNode::New(&arena_, 0, HTypes::Word32, op, 0, 0, nullptr);
    ASSERT_EQ(HConstant, k2->opcode());
    
    ASSERT_EQ(0, k1->UsedCount());
    ASSERT_EQ(0, k2->UsedCount());
    
    op = factory_.Phi(0/*control_in*/, 2/*value_in*/);
    HNode *inputs[] = {k1, k2};
    HNode *phi = HNode::New(&arena_, 0, HTypes::Word32, op, 8, 2, inputs);
    ASSERT_EQ(HPhi, phi->opcode());
    ASSERT_EQ(HConstant, phi->input(0)->opcode());
    EXPECT_EQ(1, HOperatorWith<uint32_t>::Data(phi->input(0)));
    ASSERT_EQ(HConstant, phi->input(1)->opcode());
    EXPECT_EQ(2, HOperatorWith<uint32_t>::Data(phi->input(1)));
    
    {
        HNode::UsedIterator iter(k1);
        iter.SeekToFirst();
        ASSERT_EQ(phi, *iter);
    } {
        HNode::UsedIterator iter(k2);
        iter.SeekToFirst();
        ASSERT_EQ(phi, *iter);
    }
}

TEST_F(HIRTest, AppendInput) {
    const HOperator *op = factory_.Phi(0/*control_in*/, 6/*value_in*/);
    HNode *phi = HNode::New(&arena_, 0, HTypes::Word32, op, 4, 0, nullptr);
    for (int i = 0; i < phi->inputs_capacity(); i++) {
        op = factory_.ConstantWord32(i);
        HNode *input = HNode::New(&arena_, 0, HTypes::Word32, op, 0, 0, nullptr);
        phi->AppendInput(&arena_, input);
    }
    
    ASSERT_EQ(4, phi->inputs_capacity());
    ASSERT_EQ(4, phi->inputs_size());
    
    op = factory_.ConstantWord32(999);
    HNode *input = HNode::New(&arena_, 0, HTypes::Word32, op, 0, 0, nullptr);
    phi->AppendInput(&arena_, input);
    ASSERT_EQ(8, phi->inputs_capacity());
    ASSERT_EQ(5, phi->inputs_size());
    
    HNode::UsedIterator iter(phi->input(0));
    iter.SeekToFirst();
    ASSERT_TRUE(iter.Valid());
    EXPECT_EQ(phi, *iter);
}

TEST_F(HIRTest, HGraphSanity) {
    HGraph *graph = new (&arena_) HGraph(&arena_);
    HNode *begin = graph->NewNode(factory_.Begin(), HTypes::Void);
    HNode *value = graph->NewNode(factory_.ConstantWord32(1), HTypes::Word32);
    HNode *end = graph->NewNode(factory_.End(1, 1), HTypes::Void, begin, value);
    //HOperator::UpdateValueIn(end->op(), 1);

    graph->set_start(begin);
    graph->set_end(end);
}

TEST_F(HIRTest, HGraphSimpleAdd) {
    HGraph *graph = new (&arena_) HGraph(&arena_);
    HNode *begin = graph->NewNode(factory_.Begin(), HTypes::Void);
    HNode *lhs = graph->NewNode(factory_.Parameter(0), HTypes::Word32);
    HNode *rhs = graph->NewNode(factory_.Parameter(1), HTypes::Word32);
    HNode *add = graph->NewNode(factory_.Add32(), HTypes::Word32, lhs, rhs);
    HNode *end = graph->NewNode(factory_.End(1, 1), HTypes::Void, begin, add);
    
    graph->set_start(begin);
    graph->set_end(end);

    ASSERT_EQ(begin, end->GetControlInput(0));
    ASSERT_EQ(add, end->GetValueInput(0));
    ASSERT_EQ(2, add->op()->value_in());
    ASSERT_EQ(0, add->op()->control_in());
}

TEST_F(HIRTest, HGraphSimpleBranch) {
    HGraph *graph = new (&arena_) HGraph(&arena_);
    
    HNode *begin = graph->NewNode(factory_.Begin());
    HNode *x = graph->NewNode(factory_.Parameter(0), HTypes::Word8);
    HNode *branch = graph->NewNode(factory_.Branch(1, 1), HTypes::Void, begin, x);
    HNode *if_true = graph->NewNode(factory_.IfTrue(0), HTypes::Void, branch);
    HNode *if_false = graph->NewNode(factory_.IfFalse(0), HTypes::Void, branch);
    HNode *merge = graph->NewNode(factory_.Merge(2), HTypes::Void, if_true, if_false);
    HNode *k1 = graph->NewNode(factory_.ConstantWord32(1), HTypes::Word32);
    HNode *k2 = graph->NewNode(factory_.ConstantWord32(2), HTypes::Word32);
    HNode *phi = graph->NewNode(factory_.Phi(1, 2), HTypes::Word32, branch, k1, k2);
    HNode *end = graph->NewNode(factory_.End(1, 1), HTypes::Void, merge, phi);
 
    graph->set_start(begin);
    graph->set_end(end);
    
    EXPECT_EQ(0, x->op()->control_in());
    EXPECT_EQ(1, branch->op()->control_in());
    EXPECT_EQ(begin, branch->input(0));
    EXPECT_EQ(1, branch->op()->value_in());
    EXPECT_EQ(x, branch->input(1));

    EXPECT_EQ(1, if_true->op()->control_in());
    EXPECT_EQ(1, if_false->op()->control_in());
    
}

TEST_F(HIRTest, HGraphSimpleForLoop) {
    // for (int i = 0; i < n; i++) { ... }
    HGraph *graph = new (&arena_) HGraph(&arena_);
    HNode *begin = graph->NewNode(factory_.Begin());
    HNode *loop = graph->NewNode(factory_.Loop(), HTypes::Void, begin);
    HNode *n = graph->NewNode(factory_.Parameter(0), HTypes::Int32);
    HNode *zero = graph->NewNode(factory_.ConstantWord32(0), HTypes::Int32);
    HNode *one = graph->NewNode(factory_.ConstantWord32(1), HTypes::Int32);
    HNode *phi = graph->NewNodeReserved(factory_.Phi(1, 2), HTypes::Int32, 4);
    HNode *add = graph->NewNode(factory_.Add32(), HTypes::Int32, phi, one);
    phi->AppendInput(&arena_, loop);
    phi->AppendInput(&arena_, zero);
    phi->AppendInput(&arena_, add);
    HNode *less_than = graph->NewNode(factory_.LessThan32(), HTypes::Word8, phi, n);
    HNode *branch = graph->NewNode(factory_.Branch(1, 1), HTypes::Void, loop, less_than);
    HNode *if_true = graph->NewNode(factory_.IfTrue(0), HTypes::Void, branch);
    HNode *if_false = graph->NewNode(factory_.IfFalse(0), HTypes::Void, branch);
    HNode *loop_exit = graph->NewNode(factory_.LoopExit(2), HTypes::Void, if_true, loop);
    HNode *end = graph->NewNode(factory_.End(2, 0), HTypes::Void, if_false, loop_exit);

    graph->set_start(begin);
    graph->set_end(end);
    
    EXPECT_EQ(2, end->op()->control_in());
    EXPECT_EQ(if_false, end->input(0));
    EXPECT_EQ(loop_exit, end->input(1));
    
    EXPECT_EQ(2, loop_exit->op()->control_in());
    EXPECT_EQ(if_true, loop_exit->input(0));
    EXPECT_EQ(loop, loop_exit->input(1));

    EXPECT_EQ(1, if_false->op()->control_in());
    EXPECT_EQ(branch, if_false->input(0));
    EXPECT_EQ(1, if_true->op()->control_in());
    EXPECT_EQ(branch, if_true->input(0));

    EXPECT_EQ(1, branch->op()->control_in());
    EXPECT_EQ(1, branch->op()->value_in());
    EXPECT_EQ(loop, branch->input(0));
    EXPECT_EQ(less_than, branch->input(1));
    
    EXPECT_EQ(2, less_than->op()->value_in());
    EXPECT_EQ(phi, less_than->input(0));
    EXPECT_EQ(n, less_than->input(1));
    
    EXPECT_EQ(1, phi->op()->control_in());
    EXPECT_EQ(2, phi->op()->value_in());
    EXPECT_EQ(loop, phi->input(0));
    EXPECT_EQ(zero, phi->input(1));
    EXPECT_EQ(add, phi->input(2));
    
    EXPECT_EQ(2, add->op()->value_in());
    EXPECT_EQ(phi, add->input(0));
    EXPECT_EQ(one, add->input(1));

    EXPECT_EQ(1, loop->op()->control_in());
    EXPECT_EQ(begin, loop->input(0));
}

} // namespace lang


} // namespace mai
