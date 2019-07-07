#include "nyaa/thread.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/profiling.h"
#include "nyaa/object-factory.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/bytecode.h"
#include "nyaa/bytecode-builder.h"
#include "base/slice.h"
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
TEST_F(NyaaThreadJITTest, BranchProfilingBenchmark) {
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
    EXPECT_EQ(0, script->Call(nullptr, 0, 0, core_)) << try_catch.ToString();

    auto slot = core_->profiler()->FindOrNull(1);
    ASSERT_NE(nullptr, slot);
    ASSERT_EQ(ProfileEntry::kCalling, slot->kind);
    ASSERT_EQ(1, slot->hit);
    ASSERT_EQ(1, slot->nrets);
    ASSERT_EQ(kTypeSmi, slot->ret);

    slot = core_->profiler()->FindOrNull(3);
    ASSERT_NE(nullptr, slot);
    ASSERT_EQ(ProfileEntry::kCalling, slot->kind);
    ASSERT_EQ(1, slot->hit);
    ASSERT_EQ(2, slot->nrets);
    ASSERT_EQ(kTypeSmi, slot->rets[0]);
    ASSERT_EQ(kTypeSmi, slot->rets[1]);
}
    
TEST_F(NyaaThreadJITTest, BranchProfiling) {
    static const char s[] = {
        "var a = ...\n"
        "if (a == 0) {\n"
        "   log('1')\n"
        "}\n"
        "if (a == 1) {\n"
        "   log('2')\n"
        "} else {\n"
        "   log('3')\n"
        "}\n"
    };
    
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    Object *argv[] = { NySmi::New(0) };
    EXPECT_EQ(0, script->Call(argv, 1, 0, core_)) << try_catch.ToString();
    
    auto slot = core_->profiler()->FindOrNull(2);
    ASSERT_NE(nullptr, slot);
    ASSERT_EQ(ProfileEntry::kBranch, slot->kind);
    EXPECT_EQ(1, slot->hit);
    EXPECT_EQ(1, slot->true_guard);
    
    slot = core_->profiler()->FindOrNull(6);
    ASSERT_NE(nullptr, slot);
    ASSERT_EQ(ProfileEntry::kBranch, slot->kind);
    EXPECT_EQ(1, slot->hit);
    EXPECT_EQ(0, slot->true_guard);
    
    argv[0] = NySmi::New(1);
    EXPECT_EQ(0, script->Call(argv, 1, 0, core_)) << try_catch.ToString();
    
    slot = core_->profiler()->FindOrNull(2);
    ASSERT_NE(nullptr, slot);
    ASSERT_EQ(ProfileEntry::kBranch, slot->kind);
    EXPECT_EQ(2, slot->hit);
    EXPECT_EQ(1, slot->true_guard);
    
    slot = core_->profiler()->FindOrNull(6);
    ASSERT_NE(nullptr, slot);
    ASSERT_EQ(ProfileEntry::kBranch, slot->kind);
    EXPECT_EQ(2, slot->hit);
    EXPECT_EQ(1, slot->true_guard);    
//    Handle<NyString> ast = script->proto()->packed_ast();
//    std::string_view v(ast->bytes(), ast->size());
//    printf("%s\n", base::Slice::ToReadable(v).c_str());
}

} // namespace nyaa

} // namespace mai
