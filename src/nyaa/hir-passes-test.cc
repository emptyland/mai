#include "nyaa/high-level-ir.h"
#include "nyaa/hir-passes.h"
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
    
class NyaaHIRPassesTest : public test::NyaaTest {
public:
    NyaaHIRPassesTest()
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
    
TEST_F(NyaaHIRPassesTest, PassManagement) {
    auto a1 = fn_->Parameter(Type::kInt, 1);
    auto a2 = fn_->Parameter(Type::kInt, 1);
    fn_->AddParameter(a1);
    fn_->AddParameter(a2);

    auto bb = fn_->NewBB(nullptr);
    fn_->set_entry(bb);
    auto br = fn_->Branch(bb, fn_->IntVal(1, 2), 2);
    
    auto br1 = fn_->NewBB(bb);
    br->set_if_true(br1);
    
    auto br2 = fn_->NewBB(bb);
    br->set_if_false(br2);
    
    auto out = fn_->NewBB(br1);
    out->AddInEdge(br2);
    fn_->NoCondBranch(br1, out, 4);
    fn_->NoCondBranch(br2, out, 4);
    auto phi = fn_->Phi(out, Type::kInt, 5);
    phi->AddIncoming(br1, a1);
    phi->AddIncoming(br2, a2);
    auto ret = fn_->Ret(out, 6);
    ret->AddRetVal(phi);

    PassManagement passes;
    auto pass = passes.AddPass<PhiEliminationPass>();
    pass->RunOnFunction(fn_);
    fn_->PrintTo(stdout);
}

} // namespace hir

} // namespace nyaa
    
} // namespace mai
