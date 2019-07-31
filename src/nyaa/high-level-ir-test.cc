#include "nyaa/high-level-ir.h"
#include "nyaa/thread.h"
#include "nyaa/code-gen.h"
#include "base/arenas.h"
#include "test/nyaa-test.h"
#include "mai-lang/nyaa.h"
#include "mai/env.h"
#include "gtest/gtest.h"


namespace mai {

namespace nyaa {

namespace hir {

class NyaaHIRTest : public test::NyaaTest {
public:
    NyaaHIRTest()
        : arena_(Env::Default()->GetLowLevelAllocator()) {
    }

    virtual void SetUp() override {
        NyaaTest::SetUp();
        NyaaOptions opts;
        opts.exec = Nyaa::kAOT_And_JIT;
        N_ = new Nyaa(opts, isolate_);
        core_ = N_->core();
        fn_ = Function::New(&arena_);
    }
    
    virtual void TearDown() override {
        delete N_;
        NyaaTest::TearDown();
    }
    
    void GenerateHIRInMemory(const char *z, std::vector<BuiltinType> args, hir::Function **rv) {
        TryCatchCore try_catch(core_);
        auto script = NyClosure::Compile(z, core_);
        ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
        
        auto rs = CodeGen::GenerateHIR(script, &args[0], args.size(), rv, &arena_, core_);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
    }
    
    base::StandaloneArena arena_;
    Function *fn_;
    Nyaa *N_ = nullptr;
    NyaaCore *core_ = nullptr;
};
    
TEST_F(NyaaHIRTest, Sanity) {
    auto bb = fn_->NewBB(nullptr);
    ASSERT_EQ(0, bb->label());
    
    auto a1 = fn_->Parameter(Type::kInt, 1);
    EXPECT_TRUE(a1->IsIntTy());
    EXPECT_EQ(0, a1->index());
    EXPECT_EQ(1, a1->line());
    auto a2 = fn_->Parameter(Type::kInt, 2);
    EXPECT_TRUE(a2->IsIntTy());
    EXPECT_EQ(1, a2->index());
    EXPECT_EQ(2, a2->line());
    
    auto t1 = fn_->IAdd(bb, a1, a2, 3);
    EXPECT_TRUE(t1->IsIntTy());
    EXPECT_EQ(2, t1->index());
    EXPECT_EQ(3, t1->line());
    EXPECT_EQ(Value::kIAdd, t1->kind());
    
    auto t2 = fn_->IMinus(bb, t1, 4);
    EXPECT_TRUE(t2->IsIntTy());
    EXPECT_EQ(3, t2->index());
    EXPECT_EQ(4, t2->line());
    EXPECT_EQ(Value::kIMinus, t2->kind());
    
    auto ret = fn_->Ret(bb, 5);
    ret->AddRetVal(t2);
    EXPECT_TRUE(ret->IsVoidTy());
    EXPECT_EQ(1, ret->ret_vals().size());
    EXPECT_EQ(Value::kRet, ret->kind());

    //fn_->PrintTo(stdout);
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
    //fn_->PrintTo(stdout);
}
    
TEST_F(NyaaHIRTest, CastPriority) {
    auto prio = GetCastPriority(Type::kInt, Type::kLong);
    ASSERT_EQ(CastPriority::kLHS, prio.how);
    ASSERT_EQ(Type::kLong, prio.type);
    ASSERT_EQ(Value::kIToL, GetCastAction(Type::kLong, Type::kInt));
}
    
TEST_F(NyaaHIRTest, ReplacementUses) {
    auto entry = fn_->NewBB(nullptr);
    auto a1 = fn_->Parameter(Type::kInt, 1);
    auto a2 = fn_->Parameter(Type::kInt, 2);
    auto v1 = fn_->IAdd(entry, a1, a2, 3);
    ASSERT_NE(nullptr, v1);
    auto ret = fn_->Ret(entry, 4);
    ret->AddRetVal(a1);
    ret->AddRetVal(a1);

    auto k1 = fn_->Constant(Type::kInt, 4);
    k1->set_smi_val(100);
    
    fn_->ReplaceAllUses(a1, k1);
    fn_->PrintTo(stdout);
}
    
TEST_F(NyaaHIRTest, GenerateHIR) {
    static const char z[] = {
        "var a, b, c = t, 1, '2'\n"
        "return a, b, c\n"
    };
    HandleScope handle_scope(N_);
    GenerateHIRInMemory(z, {}, &fn_);
    
    fn_->PrintTo(stdout);
}
    
TEST_F(NyaaHIRTest, GenerateCalling) {
    static const char z[] = {
        "var a, b, c = t(foo())\n"
        "return a, b, c\n"
    };
    HandleScope handle_scope(N_);
    GenerateHIRInMemory(z, {}, &fn_);
    
    fn_->PrintTo(stdout);
}
    
TEST_F(NyaaHIRTest, GenerateIfBranchs) {
    static const char z[] = {
        "var a, b, c = t, 1, '2'\n"
        "if (a) { return -1 } else { return 11 }\n"
        "return a, b, c\n"
    };
    HandleScope handle_scope(N_);
    GenerateHIRInMemory(z, {}, &fn_);
    
    fn_->PrintTo(stdout);
}

TEST_F(NyaaHIRTest, GenerateMulitIfBranchs) {
    static const char z[] = {
        "var a, b, c = t, 1, '2'\n"
        "if (a) { return -1 } else if (b) { return 11 }\n"
        "return a, b, c\n"
    };
    HandleScope handle_scope(N_);
    GenerateHIRInMemory(z, {}, &fn_);
    ASSERT_NE(nullptr, fn_);
    
    fn_->PrintTo(stdout);
}
    
TEST_F(NyaaHIRTest, GenerateIfBranchsPhiNode) {
    static const char z[] = {
        "var a, b, c = 2, 1, '2'\n"
        "if (a) { a = 1 } else { a = 'a' b = 3 }\n"
        "return a\n"
    };
    HandleScope handle_scope(N_);
    GenerateHIRInMemory(z, {}, &fn_);
    ASSERT_NE(nullptr, fn_);
    
    fn_->PrintTo(stdout);
}
    
TEST_F(NyaaHIRTest, GenerateIfBranchsPhiNode2) {
    static const char z[] = {
        "var a, b, c = 2, 1, '2'\n"
        "if (a) { a = 1 } else if (b) { a = 2 } else { a = 3 }\n"
        "return a\n"
    };
    HandleScope handle_scope(N_);
    GenerateHIRInMemory(z, {}, &fn_);
    ASSERT_NE(nullptr, fn_);
    
    fn_->PrintTo(stdout);
}
    
TEST_F(NyaaHIRTest, GenerateWhileLoop) {
    static const char z[] = {
        "var a = 0\n"
        "while (a) {\n"
        "   a = a + 2\n"
        "   a = a + 1.0\n"
        "   if (b) { break }\n"
        "}"
        "return a\n"
    };
    HandleScope handle_scope(N_);
    GenerateHIRInMemory(z, {}, &fn_);
    ASSERT_NE(nullptr, fn_);
    
    fn_->PrintTo(stdout);
}

TEST_F(NyaaHIRTest, GenerateNestedWhileLoop) {
    static const char z[] = {
        "var a = 0\n"
        "while (a) {\n"
        "   while (a < 100) {\n"
        "       a = a + 2\n"
        "       a = a + 1.0\n"
        "       if (b) { break }\n"
        "   }\n"
        "   a = \"str\"\n"
        "}\n"
        "return a\n"
    };
    HandleScope handle_scope(N_);
    GenerateHIRInMemory(z, {}, &fn_);
    ASSERT_NE(nullptr, fn_);
    
    fn_->PrintTo(stdout);
}
    
TEST_F(NyaaHIRTest, ConstantFolding) {
    static const char s[] = {
        "var a, b = 0, 1.1\n"
        "a = a + b\n"
        "a = a + 2.2\n"
        "return a + b\n"
    };
    HandleScope handle_scope(N_);
    GenerateHIRInMemory(s, {}, &fn_);
    ASSERT_NE(nullptr, fn_);

    std::string buf;
    fn_->PrintTo(&buf, 1024);

    static const char z[] = {
        "l0:\n"
        "    ret(1) float 4.400000; line = 4\n"
    };
    ASSERT_EQ(z, buf) << buf;
}
    
TEST_F(NyaaHIRTest, ForStepLoop) {
    static const char s[] = {
        "var a = 1\n"
        "for (i in 0 to 100) {\n"
        "   a = a + i * 4.0\n"
        "}\n"
        "return a\n"
    };
    HandleScope handle_scope(N_);
    GenerateHIRInMemory(s, {}, &fn_);
    ASSERT_NE(nullptr, fn_);

    std::string buf;
    fn_->PrintTo(&buf, 1024);
    static const char z[] = {
        "l0:\n"
        "    br l1; line = 2\n"
        "l1:\n"
        "    float %v6 = phi [l2 float %v4] [l0 float 1.000000] ; line = 2\n"
        "    int %v0 = phi [l0 int 0] [l2 int %v5] ; line = 2\n"
        "    int %v1 = cmp gt int %v0 int 100; line = 2\n"
        "    br int %v1 then l3 else l2; line = 2\n"
        "    br l2; line = 2\n"
        "l2:\n"
        "    float %v2 = itof int %v0; line = 3\n"
        "    float %v3 = float %v2 * float 4.000000; line = 3\n"
        "    float %v4 = float 1.000000 + float %v3; line = 3\n"
        "    int %v5 = int %v0 + int 1; line = 2\n"
        "    br l1; line = 4\n"
        "l3:\n"
        "    ret(1) float %v6; line = 5\n"
    };
    ASSERT_EQ(z, buf) << buf;
}

} // namespace hir

} // namespace nyaa

} // namespace mai
