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
        "assert(1 and 1)\n"
        "assert((0 and 1) == 0)\n"
        "assert((1 and 0) == 0)\n"
        "assert((0 and 0) == 0)\n"
        "\n"
        "assert(1 or 1)\n"
        "assert(0 or 1)\n"
        "assert(1 or 0)\n"
        "assert((0 or 0) == 0)\n"
    };
    
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //ASSERT_TRUE(script->proto()->IsNativeExec());
    EXPECT_EQ(0, script->Call(nullptr, 0, 0, core_)) << try_catch.ToString();
//    printf("%u %u\n", script->proto()->file_info()->size(),
//           script->proto()->code()->instructions_bytes_size());
}

TEST_F(NyaaThreadAOTTest, IfElseStatement) {
    static const char s[] = {
        "var a = 1\n"
        "if (a) { assert(true) }\n"
        "a = nil\n"
        "if (a) { assert(false) }\n"
        "else { assert(true) }\n"
    };
    
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //ASSERT_TRUE(script->proto()->IsNativeExec());
    EXPECT_EQ(0, script->Call(nullptr, 0, 0, core_)) << try_catch.ToString();
}
    
TEST_F(NyaaThreadAOTTest, NewMap) {
    static const char s[] = {
        "var a = {1, 2, 3, 4}\n"
        "assert(a)\n"
    };
    
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //ASSERT_TRUE(script->proto()->IsNativeExec());
    EXPECT_EQ(0, script->Call(nullptr, 0, 0, core_)) << try_catch.ToString();
}
    
TEST_F(NyaaThreadAOTTest, Add) {
    static const char s[] = {
        "var a, b, c = 1, 2, 3\n"
        "assert(a + b == c)\n"
        "a = setmetatable({f:1}, {\n"
        "   __add__: lambda(lhs, rhs) { return 999 + rhs }\n"
        "})\n"
        "assert(a + 1 == 1000)\n"
        "assert(a + 2 == 1001)\n"
        "assert(a + 3 == 1002)\n"
    };
    
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //ASSERT_TRUE(script->proto()->IsNativeExec());
    EXPECT_EQ(0, script->Call(nullptr, 0, 0, core_)) << try_catch.ToString();
}
    
TEST_F(NyaaThreadAOTTest, Compare) {
    static const char s[] = {
        "var a, b, c = 1, 2, 3\n"
        "assert(a == a)\n"
        "assert(b == b)\n"
        "assert(a != b)\n"
        "assert(a != c)\n"
        "assert(a < c)\n"
        "assert(b < c)\n"
        "assert(c > a)\n"
        "assert(b > a)\n"
    };

    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //ASSERT_TRUE(script->proto()->IsNativeExec());
    EXPECT_EQ(0, script->Call(nullptr, 0, 0, core_)) << try_catch.ToString();
//    printf("%u %u\n", script->proto()->file_info()->size(),
//           script->proto()->code()->instructions_bytes_size());
}
    
TEST_F(NyaaThreadAOTTest, SetField) {
    static const char s[] = {
        "var a = {a:1, b:2}\n"
        "a.a = 2\n"
        "a.c = 3\n"
        "assert(a.a == 2)\n"
        "assert(a.b == 2)\n"
        "assert(a.c == 3)\n"
    };

    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //ASSERT_TRUE(script->proto()->IsNativeExec());
    EXPECT_EQ(0, script->Call(nullptr, 0, 0, core_)) << try_catch.ToString();
//    printf("%u %u\n", script->proto()->file_info()->size(),
//           script->proto()->code()->instructions_bytes_size());
}
    
TEST_F(NyaaThreadAOTTest, SelfCall) {
    static const char s[] = {
        "var a = {a:1, b:2}\n"
        "a.f = lambda(self) { return self.a + self.b }\n"
        "assert(a.a == 1)\n"
        "assert(a.b == 2)\n"
        "assert(a:f() == 3)\n"
        "assert(a:f() == (a.a + a.b))\n"
    };
    
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //ASSERT_TRUE(script->proto()->IsNativeExec());
    EXPECT_EQ(0, script->Call(nullptr, 0, 0, core_)) << try_catch.ToString();
    //    printf("%u %u\n", script->proto()->file_info()->size(),
    //           script->proto()->code()->instructions_bytes_size());
}
    
TEST_F(NyaaThreadAOTTest, MapIndex) {
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    ASSERT_EQ(0, NyClosure::DoFile("tests/nyaa/20-map-index-meta.nyaa", 0, nullptr, core_))
        << try_catch.ToString();
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
}

TEST_F(NyaaThreadAOTTest, NewCoroutine) {
    static const char s[] = {
        "var co = assert(new coroutine(lambda(){}))\n"
        "assert(co)\n"
        "assert(co.status == \"suspended\")\n"
    };
    
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //ASSERT_TRUE(script->proto()->IsNativeExec());
    EXPECT_EQ(0, script->Call(nullptr, 0, 0, core_)) << try_catch.ToString();
//    printf("%u %u\n", script->proto()->file_info()->size(),
//           script->proto()->code()->instructions_bytes_size());
}
    
TEST_F(NyaaThreadAOTTest, NewYield) {
    static const char s[] = {
        "var co = assert(new coroutine(lambda(){ yield(99) }))\n"
        "assert(co:resume() == 99)\n"
    };

    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    EXPECT_EQ(0, script->Call(nullptr, 0, 0, core_)) << try_catch.ToString();
}
    
TEST_F(NyaaThreadAOTTest, YieldThenResume) {
    static const char s[] = {
        "var entry = lambda(a, b) {\n"
        "   assert(a == 1) assert(b == 2)\n"
        "   a, b = yield(99)\n"
        "   assert(a == 3) assert(b == 4)\n"
        "   a, b = yield(98)\n"
        "   assert(a == 5) assert(b == 6)\n"
        "}\n"
        "var co = assert(new coroutine(entry))\n"
        "assert(co:resume(1, 2) == 99)\n"
        "assert(co:resume(3, 4) == 98)\n"
        "co:resume(5, 6)\n"
    };
    
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    EXPECT_EQ(0, script->Call(nullptr, 0, 0, core_)) << try_catch.ToString();
}

TEST_F(NyaaThreadAOTTest, Coroutine) {
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    ASSERT_EQ(0, NyClosure::DoFile("tests/nyaa/10-coroutine-sanity.nyaa", 0, nullptr, core_))
    << try_catch.ToString();
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
}

// Benchmark:
// Interpreter: 467 ms
//         AOT: 103 ms
TEST_F(NyaaThreadAOTTest, WhileLoop) {
    static const char s[] = {
        "var i, n = 0, 0"
        "while (i < 1000000) {\n"
        "   n = i + 1\n"
        "   n = n * 9\n"
        "   n = n / (1 + i) + i\n"
        "   i = i + 1\n"
        "}\n"
        "assert(n == 1000008)\n"
    };

    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    EXPECT_EQ(0, script->Call(nullptr, 0, 0, core_)) << try_catch.ToString();
}

TEST_F(NyaaThreadAOTTest, ForIterateLoop) {
    static const char s[] = {
        "var n = 0\n"
        "for i in range(0, 100) {\n"
        "   assert(i > 0)\n"
        "   n = i\n"
        "}\n"
        "assert(n == 100)\n"
    };
    
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    EXPECT_EQ(0, script->Call(nullptr, 0, 0, core_)) << try_catch.ToString();
}

TEST_F(NyaaThreadAOTTest, CoroutineIterate) {
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    ASSERT_EQ(0, NyClosure::DoFile("tests/nyaa/11-coroutine-iterate.nyaa", 0, nullptr, core_))
        << try_catch.ToString();
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
}

TEST_F(NyaaThreadAOTTest, ObjectDefinition) {
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    ASSERT_EQ(0, NyClosure::DoFile("tests/nyaa/30-object-define.nyaa", 0, nullptr, core_))
        << try_catch.ToString();
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
}
    
TEST_F(NyaaThreadAOTTest, ClassDefinition) {
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    ASSERT_EQ(0, NyClosure::DoFile("tests/nyaa/31-class-define.nyaa", 0, nullptr, core_))
    << try_catch.ToString();
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
}

} // namespace nyaa

} // namespace mai
