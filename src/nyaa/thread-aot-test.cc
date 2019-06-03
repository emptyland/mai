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

class NyaaThreadAOTTest : public test::NyaaTest {
public:
    void SetUp() override {
        NyaaTest::SetUp();
        NyaaOptions opts;
        opts.exec = Nyaa::kAOT;
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

TEST_F(NyaaThreadAOTTest, Sanity) {
    static const char s[] = {
        "var t = 1\n"
        "return t\n"
    };
    
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    ASSERT_TRUE(script->proto()->IsNativeExec());
    EXPECT_EQ(1, script->Call(nullptr, 0, 0, core_));
}
    
TEST_F(NyaaThreadAOTTest, Raise) {
    static const char s[] = {
        "print(\"ok\")\n"
        "print(1, 2, 3)\n"
    };
    
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    ASSERT_TRUE(script->proto()->IsNativeExec());
    EXPECT_EQ(0, script->Call(nullptr, 0, 0, core_));
}

}

}
