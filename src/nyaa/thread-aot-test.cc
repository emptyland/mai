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
    //ASSERT_TRUE(script->proto()->IsNativeExec());
    EXPECT_EQ(1, script->Call(nullptr, 0, -1, core_));
}
    
TEST_F(NyaaThreadAOTTest, Raise) {
    static const char s[] = {
        "print(\"ok\")\n"
        "raise(\"error\")\n"
    };

    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //ASSERT_TRUE(script->proto()->IsNativeExec());
    EXPECT_EQ(-1, script->Call(nullptr, 0, 0, core_));
    //FAIL() << try_catch.ToString();
}
    
TEST_F(NyaaThreadAOTTest, Call) {
    static const char s[] = {
        "def baz() { bar() }\n"
        "def bar() { foo() }\n"
        "def foo() { print(\"foo\") }\n"
        "baz()\n"
        "bar()\n"
        "foo()\n"
    };
    
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //ASSERT_TRUE(script->proto()->IsNativeExec());
    EXPECT_EQ(0, script->Call(nullptr, 0, 0, core_)) << try_catch.ToString();
    //FAIL() << try_catch.ToString();
}
    
TEST_F(NyaaThreadAOTTest, AndOrSwitch) {
    static const char s[] = {
        "print(1 and 1)\n"
        "print(0 and 1)\n"
        "print(1 and 0)\n"
        "print(0 and 0)\n"
        "print(\"-------\")\n"
        "print(1 or 1)\n"
        "print(0 or 1)\n"
        "print(1 or 0)\n"
        "print(0 or 0)\n"
    };
    
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //ASSERT_TRUE(script->proto()->IsNativeExec());
    EXPECT_EQ(0, script->Call(nullptr, 0, 0, core_)) << try_catch.ToString();
    //FAIL() << try_catch.ToString();
    printf("%u %u\n", script->proto()->file_info()->size(),
           script->proto()->code()->instructions_bytes_size());
}

TEST_F(NyaaThreadAOTTest, IfElseStatement) {
    static const char s[] = {
        "var a = 1\n"
        "if (a) { print(\"true\") }\n"
        "a = nil\n"
        "if (a) { print(\"true\") }\n"
        "else { print(\"false\") }\n"
    };
    
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //ASSERT_TRUE(script->proto()->IsNativeExec());
    EXPECT_EQ(0, script->Call(nullptr, 0, 0, core_)) << try_catch.ToString();
}

}

}
