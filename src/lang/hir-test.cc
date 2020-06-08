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
    const HOperator *phi = factory_.Phi(2);
    ASSERT_EQ(HPhi, phi->value());
    ASSERT_EQ(2, phi->value_in());
    ASSERT_STREQ("Phi", phi->name());
    
    HNode *node = HNode::New(&arena_, 0, 0, HTypes::Word8, phi, nullptr, 0);
    ASSERT_EQ(HPhi, node->opcode());
}


} // namespace lang


} // namespace mai
