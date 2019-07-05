#include "nyaa/thread.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/object-factory.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/bytecode.h"
#include "nyaa/bytecode-builder.h"
#include "test/nyaa-test.h"
#include "mai-lang/nyaa.h"

namespace mai {
    
namespace nyaa {

class NyaaThreadJITTest : public test::NyaaTest {
public:
    void SetUp() override {
        NyaaTest::SetUp();
        NyaaOptions opts;
        opts.exec = Nyaa::kAOT_And_JIT;
        N_ = new Nyaa(opts, isolate_);
        core_ = N_->core();
        factory_ = core_->factory();
    }
    
    void TearDown() override {
        delete N_;
        NyaaTest::TearDown();
    }
    
    Nyaa *N_ = nullptr;
    NyaaCore *core_ = nullptr;
    ObjectFactory *factory_ = nullptr;
};
    
//     profiling: 208 162 162 162 166
// non-profiling: 145 145 145 145 146
TEST_F(NyaaThreadJITTest, BranchProfiling) {
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    ASSERT_EQ(0, NyClosure::DoFile("tests/nyaa/18-branch-guard-profiling.nyaa", 0, nullptr, core_))
        << try_catch.ToString();
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
}

}

}
