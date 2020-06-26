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
    ASSERT_EQ(0, phi->value_in());
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
    HNode *end = graph->NewNode(factory_.End(1), HTypes::Void, begin, value);
    HOperator::UpdateValueIn(end->op(), 1);

    graph->set_start(begin);
    graph->set_end(end);
}

TEST_F(HIRTest, HGraphSimpleOperator) {
    HGraph *graph = new (&arena_) HGraph(&arena_);
    HNode *begin = graph->NewNode(factory_.Begin(), HTypes::Void);
    HNode *lhs = graph->NewNode(factory_.Parameter(0), HTypes::Word32);
    HNode *rhs = graph->NewNode(factory_.Parameter(1), HTypes::Word32);
    HNode *add = graph->NewNode(factory_.Add32(), HTypes::Word32, lhs, rhs);
    HNode *end = graph->NewNode(factory_.End(1), HTypes::Void, begin, add);
    HOperator::UpdateValueIn(end->op(), 1);
    
    graph->set_start(begin);
    graph->set_end(end);

    ASSERT_EQ(begin, end->GetControlInput(0));
    ASSERT_EQ(add, end->GetValueInput(0));
    ASSERT_EQ(2, add->op()->value_in());
    ASSERT_EQ(0, add->op()->control_in());
}

} // namespace lang


} // namespace mai
