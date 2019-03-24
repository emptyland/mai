#include "nyaa/thread.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/object-factory.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/bytecode.h"
#include "test/nyaa-test.h"
#include "mai-lang/nyaa.h"

namespace mai {
    
namespace nyaa {
    
class NyaaThreadTest : public test::NyaaTest {
public:
    void SetUp() override {
        NyaaTest::SetUp();
        N_ = new Nyaa(Nyaa::Options{}, isolate_);
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
    //printf("calling test1...\n");
    return Return(String::New(N, "1st"),
                  String::New(N, "2nd"),
                  String::New(N, "3th"));
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

    Handle<NyScript> script(core_->factory()->NewScript(nullptr, nullptr, *bcbuf, *pool));
    auto thd = N_->core()->main_thd();
    auto rv = thd->Run(*script);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(3, thd->frame_size());
    
    Handle<NyString> r1(thd->Get(0));
    ASSERT_STREQ("1st", r1->bytes());
    r1 = thd->Get(1);
    ASSERT_STREQ("2nd", r1->bytes());
    r1 = thd->Get(2);
    ASSERT_STREQ("3th", r1->bytes());
}
    
static int CallingTest2(Local<Value> lhs, Local<Value> rhs, Nyaa *N) {
    int64_t result = lhs->AsInt() + rhs->AsInt();
    
    return Return(Integral::New(N, result));
}
    
TEST_F(NyaaThreadTest, Calling2) {
    HandleScope scope(isolate_);
    
    Handle<NyDelegated> fn(core_->factory()->NewDelegated(CallingTest2));
    Handle<NyString> name(core_->factory()->NewString("test2"));
    core_->SetGlobal(*name, *fn);
    
    Handle<NyArray> pool(core_->factory()->NewArray(4));
    pool->Add(*name, core_);
    
    Handle<NyByteArray> bcbuf(core_->factory()->NewByteArray(1024));
    
    // local a, b, c = test1()
    bcbuf->Add(Bytecode::kPushGlobal, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(Bytecode::kPushImm, core_);
    bcbuf->Add(100, core_);
    bcbuf->Add(Bytecode::kPushImm, core_);
    bcbuf->Add(22, core_);
    
    bcbuf->Add(Bytecode::kCall, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(2, core_);
    bcbuf->Add(1, core_);
    
    Handle<NyScript> script(core_->factory()->NewScript(nullptr, nullptr, *bcbuf, *pool));
    auto thd = N_->core()->main_thd();
    auto rv = thd->Run(*script);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(1, thd->frame_size());
    Handle<Object> r1(thd->Get(0));
    ASSERT_TRUE(r1->IsSmi());
    ASSERT_EQ(122, r1->ToSmi());
}
    
TEST_F(NyaaThreadTest, CallFunction) {
    HandleScope scope(isolate_);
    
    Handle<NyByteArray> bcbuf(core_->factory()->NewByteArray(1024));
    
    // local a, b, c = test1()
    bcbuf->Add(Bytecode::kAdd, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(1, core_);
    bcbuf->Add(Bytecode::kReturn, core_);
    bcbuf->Add(1, core_);
    
    Handle<NyScript> script(core_->factory()->NewScript(nullptr, nullptr, *bcbuf, nullptr));
    Handle<NyFunction> fn(core_->factory()->NewFunction(2, false, 1, *script));
    Handle<NyString> name(core_->factory()->NewString("test3"));
    core_->SetGlobal(*name, *fn);
    
    bcbuf = core_->factory()->NewByteArray(128);
    bcbuf->Add(Bytecode::kPushGlobal, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(Bytecode::kPushImm, core_);
    bcbuf->Add(99, core_);
    bcbuf->Add(Bytecode::kPushImm, core_);
    bcbuf->Add(1, core_);
    bcbuf->Add(Bytecode::kCall, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(2, core_);
    bcbuf->Add(1, core_);
    
    Handle<NyArray> pool(core_->factory()->NewArray(4));
    pool->Add(*name, core_);
    script = core_->factory()->NewScript(nullptr, nullptr, *bcbuf, *pool);
    auto thd = N_->core()->main_thd();
    auto rv = thd->Run(*script);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(1, thd->frame_size());
    Handle<Object> r1(thd->Get(0));
    ASSERT_TRUE(r1->IsSmi());
    ASSERT_EQ(100, r1->ToSmi());
}
    
TEST_F(NyaaThreadTest, CallVarArgsAndResults) {
    HandleScope scope(isolate_);
    
    Handle<NyArray> pool(core_->factory()->NewArray(4));
    Handle<NyString> name(core_->factory()->NewString("test3"));
    pool->Add(*name, core_);
    pool->Add(core_->factory()->NewString("print"), core_);
    
    // print(test3())
    Handle<NyByteArray> bcbuf(core_->factory()->NewByteArray(1024));
    
    bcbuf->Add(Bytecode::kPushImm, core_);
    bcbuf->Add(1, core_); // push 1
    bcbuf->Add(Bytecode::kPushImm, core_);
    bcbuf->Add(2, core_); // push 2
    bcbuf->Add(Bytecode::kPushImm, core_);
    bcbuf->Add(3, core_); // push 3
    bcbuf->Add(Bytecode::kReturn, core_);
    bcbuf->Add(3, core_); // ret 3
    
    Handle<NyScript> script(core_->factory()->NewScript(nullptr, nullptr, *bcbuf, nullptr));
    Handle<NyFunction> fn(core_->factory()->NewFunction(0, false, 3, *script));
    core_->SetGlobal(*name, *fn);

    bcbuf = core_->factory()->NewByteArray(1024);
    bcbuf->Add(Bytecode::kPushGlobal, core_);
    bcbuf->Add(1, core_); // push g.print
    bcbuf->Add(Bytecode::kPushGlobal, core_);
    bcbuf->Add(0, core_); // push g.test3
    bcbuf->Add(Bytecode::kCall, core_);
    bcbuf->Add(1, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(-1, core_); // call l[1], 0, -1
    bcbuf->Add(Bytecode::kCall, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(-1, core_);
    bcbuf->Add(0, core_); // call l[0], -1, 0
    
    script = core_->factory()->NewScript(nullptr, nullptr, *bcbuf, *pool);
    auto thd = N_->core()->main_thd();
    auto rv = thd->Run(*script);
    ASSERT_EQ(0, rv);
}
    
//#if 0
TEST_F(NyaaThreadTest, NewCoroutine) {
    HandleScope scope(isolate_);
    
    Handle<NyArray> pool(core_->factory()->NewArray(4));
    //Handle<NyString> clazz(core_->factory()->NewString("coroutine"));
    pool->Add(core_->factory()->NewString("coroutine"), core_);
    pool->Add(core_->factory()->NewString("print"), core_);
    
    Handle<NyByteArray> bcbuf(core_->factory()->NewByteArray(1024));
    
    bcbuf->Add(Bytecode::kPushGlobal, core_);
    bcbuf->Add(0, core_); // push g["coroutine"]
    bcbuf->Add(Bytecode::kPushNil, core_);
    bcbuf->Add(1, core_); // push nil
    bcbuf->Add(Bytecode::kPushGlobal, core_);
    bcbuf->Add(1, core_); // push g["print"]
    bcbuf->Add(Bytecode::kNew, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(2, core_); // new local[0], 1
    
    Handle<NyScript> script(core_->factory()->NewScript(nullptr, nullptr, *bcbuf, *pool));
    auto thd = N_->core()->main_thd();
    auto rv = thd->Run(*script);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(1, thd->frame_size());
//    Handle<Object> r1(thd->Get(0));
//    ASSERT_TRUE(r1->IsSmi());
//    ASSERT_EQ(200, r1->ToSmi());
}
//#endif

} // namespace nyaa
    
} // namespace mai
