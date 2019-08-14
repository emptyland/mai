#include "nyaa/low-level-ir.h"
#include "nyaa/thread.h"
#include "nyaa/code-gen.h"
#include "nyaa/runtime.h"
#include "base/arenas.h"
#include "test/nyaa-test.h"
#include "mai-lang/nyaa.h"
#include "mai/env.h"
#include "gtest/gtest.h"


namespace mai {
    
namespace nyaa {

namespace lir {
    
class NyaaLIRTest : public test::NyaaTest {
public:
    NyaaLIRTest()
        : arena_(Env::Default()->GetLowLevelAllocator()) {
    }

    virtual void SetUp() override {
        NyaaTest::SetUp();
        NyaaOptions opts;
        opts.exec = Nyaa::kAOT_And_JIT;
        N_ = new Nyaa(opts, isolate_);
        core_ = N_->core();
        ib_ = Function::New(&arena_);
    }
    
    virtual void TearDown() override {
        //delete ib_;
        delete N_;
        NyaaTest::TearDown();
    }
    
    base::StandaloneArena arena_;
    Function *ib_ = nullptr;
    Nyaa *N_ = nullptr;
    NyaaCore *core_ = nullptr;
};
    
TEST_F(NyaaLIRTest, Sanity) {
    Block *l0 = ib_->NewBlock(0);
    
    Instruction *instr = Instruction::New(kMove, Architecture::kAllRegisters[0],
                                          Architecture::kAllRegisters[1], 0, &arena_);
    l0->Add(instr);
    instr = Instruction::New(kJump, l0, 1, &arena_);
    l0->Add(instr);
    instr = Instruction::New(kAdd, Architecture::kAllRegisters[0],
                             Architecture::kAllRegisters[1],
                             Architecture::kAllRegisters[2], 2, &arena_);
    l0->Add(instr);
    Operand *out = new (&arena_) ImmediateOperand(Runtime::kExternalLinks[0]);
    instr = Instruction::New(kCallNative, 0/*subcode*/, out, 3/*n_inputs*/, 3/*line*/, &arena_);
    instr->set_input(0, Architecture::kAllRegisters[1]);
    instr->set_input(1, Architecture::kAllRegisters[2]);
    instr->set_input(2, Architecture::kAllRegisters[3]);
    l0->Add(instr);
    ib_->PrintAll(stdout);
};

} // namespace lir

} // namespace nyaa
    
} // namespace mai
