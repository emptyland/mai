#include "nyaa/spaces.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/nyaa-values.h"
#include "test/nyaa-test.h"
#include "mai-lang/nyaa.h"

namespace mai {
    
namespace nyaa {

class NyaaApiTest : public test::NyaaTest {
public:
    NyaaApiTest()
        : scope_(Nyaa::Options{}, isolate_)
        , N_(&scope_)
        , core_(scope_.core()) {
    }
    
    void SetUp() override {
        NyaaTest::SetUp();
    }
    
    void TearDown() override {
        NyaaTest::TearDown();
    }
    
    Nyaa scope_;
    Nyaa *N_ = nullptr;
    NyaaCore *core_ = nullptr;
};
    
TEST_F(NyaaApiTest, Sanity) {
    HandleScope handle_scope(N_);
    
    TryCatch try_catch(N_);
    Handle<String> source = String::New("return 1, 2, 3");
    Handle<Script> script = Script::Compile(source);
    ASSERT_FALSE(try_catch.HasCaught()) << try_catch.Message()->ToStl();
    
    Handle<Value> rets[3];
    ASSERT_EQ(3, script->Run(nullptr, 0, rets, 3));
    ASSERT_FALSE(try_catch.HasCaught()) << try_catch.Message()->ToStl();
    
    for (int i = 0; i < arraysize(rets); ++i) {
        ASSERT_TRUE(rets[i]->IsNumber());
        auto val = Handle<Number>::Cast(rets[i]);
        ASSERT_EQ(i + 1, val->I64Value());
    }
}
    
static void TestCallback1(const FunctionCallbackInfo<Value> &info) {
    info.GetReturnValues().Add("foo").Add("bar").Add("baz");
}

TEST_F(NyaaApiTest, CallCxxFunction) {
    HandleScope handle_scope(N_);
    
    TryCatch try_catch(N_);
    Handle<Function> func = Function::New(TestCallback1, 0);
    N_->SetGlobal(String::New("fn"), func);
    Handle<String> source = String::New("return fn()");
    Handle<Script> script = Script::Compile(source);
    ASSERT_FALSE(try_catch.HasCaught()) << try_catch.Message()->ToStl();
    
    Handle<Value> rets[3];
    ASSERT_EQ(3, script->Run(nullptr, 0, rets, 3));
    ASSERT_FALSE(try_catch.HasCaught()) << try_catch.Message()->ToStl();
    
    static const char *z[] = {"foo", "bar", "baz"};
    for (int i = 0; i < arraysize(rets); ++i) {
        ASSERT_TRUE(rets[i]->IsString());
        Handle<String> val = Handle<String>::Cast(rets[i]);
        ASSERT_STREQ(z[i], val->Bytes());
    }
}

TEST_F(NyaaApiTest, CallNyaaFunction) {
    HandleScope handle_scope(N_);

    TryCatch try_catch(N_);
    Handle<String> source = String::New("def foo(a, b, c) { return a + b + c }\n"
                                        "return foo\n");
    Handle<Script> script = Script::Compile(source);
    ASSERT_FALSE(try_catch.HasCaught()) << try_catch.Message()->ToStl();

    Handle<Value> ret;
    ASSERT_EQ(1, script->Run(nullptr, 0, &ret, -1));
    ASSERT_FALSE(try_catch.HasCaught()) << try_catch.Message()->ToStl();

    ASSERT_TRUE(ret->IsFunction());
    Handle<Function> nyaafn = Handle<Function>::Cast(ret);

    Handle<Value> argv[3] = {
        Number::NewI64(1),
        Number::NewI64(2),
        Number::NewI64(3),
    };
    ASSERT_EQ(1, nyaafn->Call(argv, 3, &ret, -1));
    ASSERT_FALSE(try_catch.HasCaught()) << try_catch.Message()->ToStl();

    ASSERT_TRUE(ret->IsNumber());
    ASSERT_EQ(6, Handle<Number>::Cast(ret)->I64Value());
}
    
TEST_F(NyaaApiTest, ScriptVarg) {
    HandleScope handle_scope(N_);
    
    TryCatch try_catch(N_);
    Handle<String> source = String::New("var a, b, c = ...\n"
                                        "return a + b * c\n");
    Handle<Script> script = Script::Compile(source);
    ASSERT_FALSE(try_catch.HasCaught()) << try_catch.Message()->ToStl();
    
    Handle<Value> ret;
    Handle<Value> argv[3] = {
        Number::NewI64(1),
        Number::NewI64(2),
        Number::NewI64(3),
    };
    ASSERT_EQ(1, script->Run(argv, 3, &ret, 1));
    ASSERT_FALSE(try_catch.HasCaught()) << try_catch.Message()->ToStl();
    
    ASSERT_TRUE(ret->IsNumber());
    ASSERT_EQ(7, Handle<Number>::Cast(ret)->I64Value());
}
    
} // namespace nyaa

} // namespace mai
