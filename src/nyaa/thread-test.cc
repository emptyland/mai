#include "nyaa/thread.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/object-factory.h"
#include "nyaa/bytecode.h"
#include "test/nyaa-test.h"
#include "mai-lang/nyaa.h"

namespace mai {
    
namespace nyaa {
    
class NyaaThreadTest : public test::NyaaTest {
public:
    void SetUp() override {
        NyaaTest::SetUp();
        N_ = new Nyaa(isolate_);
        core_ = N_->core();
    }
    
    void TearDown() override {
        delete N_;
        NyaaTest::TearDown();
    }
    
    Nyaa *N_ = nullptr;
    NyaaCore *core_ = nullptr;
};
    
TEST_F(NyaaThreadTest, Sanity) {
    HandleScope scope(isolate_);
    
    Handle<NyByteArray> bcbuf(core_->factory()->NewByteArray(1024));
    bcbuf->Add(Bytecode::kPushImm, core_);
    bcbuf->Add(119, core_);
    bcbuf->Add(Bytecode::kReturn, core_);
    bcbuf->Add(0, core_);
    
    Handle<NyScript> script(core_->factory()->NewScript(nullptr, nullptr, *bcbuf, nullptr));
    
    auto thd = N_->core()->main_thd();
    auto rv = thd->Run(*script);
    ASSERT_EQ(0, rv);
//    ASSERT_TRUE(thd->ra()->is_smi());
//    ASSERT_EQ(119, thd->ra()->smi());
}
    
static int CallingTest1(Nyaa *N) {
    printf("calling test1...\n");
    return Return(String::New(N, "1st"),
                  String::New(N, "2nd"),
                  String::New(N, "3rd"));
}
    
TEST_F(NyaaThreadTest, Calling) {
    HandleScope scope(isolate_);
    
    Handle<NyDelegated> fn(core_->factory()->NewDelegated(CallingTest1));
    Handle<NyString> name(core_->factory()->NewString("test1"));
    core_->SetGlobal(*name, *fn);
    
    Handle<NyArray> pool(core_->factory()->NewArray(4));
    pool->Add(*name, core_);
    
    Handle<NyByteArray> bcbuf(core_->factory()->NewByteArray(1024));

    // local a, b, c = test1()
    bcbuf->Add(Bytecode::kPushGlobal, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(Bytecode::kCall, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(3, core_);
    
    // var a, b, c = foo(), 1
    // LoadGlobal 'foo'
    // Call 0, 1
    // PushImm 1
    // PushNil
    //
    // var a, b, c = foo()
    // LoadGlobal 'foo'
    // Call 0, 3
    //
    // var a, b, c = 1, 2, 3
    // PushImm 1
    // PushImm 2
    // PushImm 3
    Handle<NyScript> script(core_->factory()->NewScript(nullptr, nullptr, *bcbuf, *pool));
    auto thd = N_->core()->main_thd();
    auto rv = thd->Run(*script);
    ASSERT_EQ(0, rv);
}
    
} // namespace nyaa
    
} // namespace mai
