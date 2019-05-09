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
    Handle<String> source = String::New(N_, "return 1, 2, 3");
    Handle<Script> script = Script::Compile(N_, source);
    ASSERT_FALSE(try_catch.HasCaught()) << try_catch.Message()->ToStl();
    
    Handle<Value> rets[3];
    ASSERT_EQ(3, script->Run(N_, nullptr, 0, rets, 3));
    ASSERT_FALSE(try_catch.HasCaught()) << try_catch.Message()->ToStl();
    
    for (int i = 0; i < arraysize(rets); ++i) {
        ASSERT_TRUE(rets[i]->IsNumber());
        auto val = Handle<Number>::Cast(rets[i]);
        ASSERT_EQ(i + 1, val->I64Value());
    }
}

} // namespace nyaa
    
} // namespace mai
