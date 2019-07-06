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
    
//     profiling: 625 585 595
// non-profiling: 621 598 583
TEST_F(NyaaThreadJITTest, BranchProfiling) {
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    ASSERT_EQ(0, NyClosure::DoFile("tests/nyaa/18-branch-guard-profiling.nyaa", 0, nullptr, core_))
        << try_catch.ToString();
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
}
    
TEST_F(NyaaThreadJITTest, CallingProfiling) {
    static const char s[] = {
        "def foo() { return 1 }\n"
        "foo()\n"
        "def bar() { return 1, 2 }\n"
        "return bar()\n"
    };
    
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    EXPECT_EQ(2, script->Call(nullptr, 0, 0, core_)) << try_catch.ToString();
    
    auto profiler = core_->profiler();
    ASSERT_TRUE(profiler.find(1) != profiler.end());
    auto slot = profiler[1];
    ASSERT_EQ(ProfileSlot::kCalling, slot.kind);
    ASSERT_EQ(1, slot.hit);
    ASSERT_EQ(1, slot.nrets);
    ASSERT_EQ(kTypeSmi, slot.ret);
    
    ASSERT_TRUE(profiler.find(3) != profiler.end());
    slot = profiler[3];
    ASSERT_EQ(ProfileSlot::kCalling, slot.kind);
    ASSERT_EQ(1, slot.hit);
    ASSERT_EQ(2, slot.nrets);
    ASSERT_EQ(kTypeSmi, slot.rets[0]);
    ASSERT_EQ(kTypeSmi, slot.rets[1]);
}

}

}
