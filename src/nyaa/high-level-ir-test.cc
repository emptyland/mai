#include "nyaa/high-level-ir.h"
#include "base/arenas.h"
#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {

namespace nyaa {

namespace hir {

class NyaaHIRTest : public testing::Test {
public:
    NyaaHIRTest()
        : arena_(Env::Default()->GetLowLevelAllocator()) {
    }

    virtual void SetUp() override {
        fn_ = Function::New(&arena_);
    }
    
    base::StandaloneArena arena_;
    Function *fn_;
};
    
TEST_F(NyaaHIRTest, Sanity) {
    auto bb = fn_->NewBB(nullptr);
    ASSERT_EQ(0, bb->label());
    
    auto a1 = fn_->Parameter(Type::kInt, 1);
    EXPECT_TRUE(a1->IsInt());
    EXPECT_EQ(0, a1->index());
    EXPECT_EQ(1, a1->line());
    auto a2 = fn_->Parameter(Type::kInt, 2);
    EXPECT_TRUE(a2->IsInt());
    EXPECT_EQ(1, a2->index());
    EXPECT_EQ(2, a2->line());
    
    auto t1 = fn_->IAdd(bb, a1, a2, 3);
    EXPECT_TRUE(t1->IsInt());
    EXPECT_EQ(2, t1->index());
    EXPECT_EQ(3, t1->line());
    EXPECT_EQ(Value::kIAdd, t1->kind());
    
    auto t2 = fn_->IMinus(bb, t1, 4);
    EXPECT_TRUE(t2->IsInt());
    EXPECT_EQ(3, t2->index());
    EXPECT_EQ(4, t2->line());
    EXPECT_EQ(Value::kIMinus, t2->kind());
    
    auto ret = fn_->Ret(bb, 5);
    ret->AddRetVal(t2);
    EXPECT_TRUE(ret->IsVoid());
    EXPECT_EQ(1, ret->ret_vals().size());
    EXPECT_EQ(Value::kRet, ret->kind());

    fn_->PrintTo(stdout);
}
    
TEST_F(NyaaHIRTest, BranchAndPhi) {
    auto entry = fn_->NewBB(nullptr);
    auto a1 = fn_->Parameter(Type::kInt, 1);
    auto a2 = fn_->Parameter(Type::kInt, 2);
    auto v1 = fn_->IAdd(entry, a1, a2, 3);
    auto br = fn_->Branch(entry, v1, 4);
    
    auto b1 = fn_->NewBB(entry);
    auto b2 = fn_->NewBB(entry);
    br->set_if_true(b1);
    br->set_if_false(b2);
    
    auto v2 = fn_->IMinus(b1, v1, 1);
    auto v3 = fn_->IMinus(b2, v1, 1);
    
    auto b3 = fn_->NewBB(b1);
    b3->AddInEdge(b2);
    auto phi = fn_->Phi(b3, Type::kInt, 1);
    phi->AddIncoming(b1, v2);
    phi->AddIncoming(b2, v3);
    auto ret = fn_->Ret(b3, 2);
    ret->AddRetVal(phi);
    
    fn_->PrintTo(stdout);
}

    
} // namespace dag

} // namespace nyaa

} // namespace mai
