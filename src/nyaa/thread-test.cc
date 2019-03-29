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
        factory_ = core_->factory();
    }
    
    void TearDown() override {
        delete N_;
        NyaaTest::TearDown();
    }
    
    // def f1() { return 1, 2, 3 }
    Handle<NyFunction> BuildF1() {
        Handle<NyByteArray> bcbuf(core_->factory()->NewByteArray(1024));
        
        bcbuf->Add(Bytecode::kPushImm, core_);
        bcbuf->Add(1, core_); // push 1
        bcbuf->Add(Bytecode::kPushImm, core_);
        bcbuf->Add(2, core_); // push 2
        bcbuf->Add(Bytecode::kPushImm, core_);
        bcbuf->Add(3, core_); // push 3
        bcbuf->Add(Bytecode::kPushImm, core_);
        bcbuf->Add(4, core_); // push 4
        bcbuf->Add(Bytecode::kRet, core_);
        bcbuf->Add(0, core_); // ret offset+0
        
        Handle<NyScript> script(factory_->NewScript(nullptr, nullptr, *bcbuf, nullptr));
        Handle<NyFunction> fn(factory_->NewFunction(0, 0, 4, *script));
        
        core_->SetGlobal(factory_->NewString("f1"), *fn);
        return fn;
    }
    
    // def f2() { return name() }
    Handle<NyFunction> BuildF2(const char *name) {
        Handle<NyArray> pool(core_->factory()->NewArray(4));
        pool->Add(factory_->NewString(name), core_);
        
        Handle<NyByteArray> bcbuf(core_->factory()->NewByteArray(1024));

        bcbuf->Add(Bytecode::kPushGlobal, core_);
        bcbuf->Add(0, core_); // push g[name]
        bcbuf->Add(Bytecode::kCall, core_);
        bcbuf->Add(0, core_);
        bcbuf->Add(0, core_);
        bcbuf->Add(-1, core_); // call l[0], 0, -1
        bcbuf->Add(Bytecode::kRet, core_);
        bcbuf->Add(0, core_); // ret offset+0
        
        Handle<NyScript> script(factory_->NewScript(nullptr, nullptr, *bcbuf, *pool));
        Handle<NyFunction> fn(factory_->NewFunction(0, 0, 4, *script));
        
        core_->SetGlobal(factory_->NewString("f2"), *fn);
        return fn;
    }
    
    Handle<NyDelegated> RegisterChecker(const char *name, int n_upvals,
                                        int (*fp)(Arguments *, Nyaa *)) {
        Handle<NyDelegated> fn(factory_->NewDelegated(fp, n_upvals));
        core_->SetGlobal(factory_->NewString(name), *fn);
        return fn;
    }
    
    Nyaa *N_ = nullptr;
    NyaaCore *core_ = nullptr;
    ObjectFactory *factory_ = nullptr;
    
    //return Yield(1, 2, 3);
};
    
TEST_F(NyaaThreadTest, Sanity) {
    HandleScope scope(isolate_);
    
    Handle<NyByteArray> bcbuf(core_->factory()->NewByteArray(1024));
    bcbuf->Add(Bytecode::kPushImm, core_);
    bcbuf->Add(119, core_); // push 119
    bcbuf->Add(Bytecode::kRet, core_);
    bcbuf->Add(0, core_); // ret offset+0
    
    Handle<NyScript> script(core_->factory()->NewScript(nullptr, nullptr, *bcbuf, nullptr));
    
    auto thd = N_->core()->main_thd();
    auto rv = thd->Run(*script);
    ASSERT_EQ(0, rv);
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
    
    return Return(Integer::New(N, result));
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
    
    bcbuf->Add(Bytecode::kAdd, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(1, core_); // add l[0], l[1]
    bcbuf->Add(Bytecode::kRet, core_);
    bcbuf->Add(0, core_); // ret offset+0
    
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
    bcbuf->Add(Bytecode::kRet, core_);
    bcbuf->Add(0, core_); // ret offset+0
    
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
}
//#endif
TEST_F(NyaaThreadTest, IndexCommand) {
    HandleScope scope(isolate_);
    
    Handle<NyArray> pool(core_->factory()->NewArray(4));
    //Handle<NyString> clazz(core_->factory()->NewString("coroutine"));
    pool->Add(core_->factory()->NewString("coroutine"), core_);
    pool->Add(core_->factory()->NewString("print"), core_);
    pool->Add(core_->factory()->NewString("status"), core_);
    
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
    bcbuf->Add(Bytecode::kIndexConst, core_);
    bcbuf->Add(2, core_);
    
    Handle<NyScript> script(core_->factory()->NewScript(nullptr, nullptr, *bcbuf, *pool));
    auto thd = N_->core()->main_thd();
    auto rv = thd->Run(*script);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(1, thd->frame_size());
}
    
int ReturnVarResult_Check(Arguments *args, Nyaa *N) {
    Local<NyDelegated> callee = ApiWarp<NyDelegated>(args->Callee(), N->core());
    if (!callee) {
        return -1;
    }
    
    callee->Bind(0, *ApiWarpNoCheck<Object>(args->Get(0), N->core()), N->core());
    callee->Bind(1, *ApiWarpNoCheck<Object>(args->Get(1), N->core()), N->core());
    callee->Bind(2, *ApiWarpNoCheck<Object>(args->Get(2), N->core()), N->core());
    callee->Bind(3, *ApiWarpNoCheck<Object>(args->Get(3), N->core()), N->core());
    return 0;
}
    
TEST_F(NyaaThreadTest, ReturnVarResult) {
    HandleScope scope(isolate_);
    
    BuildF1();
    BuildF2("f1");
    Handle<NyDelegated> check(RegisterChecker("check1", 4, ReturnVarResult_Check));
    
    Handle<NyArray> pool(core_->factory()->NewArray(4));
    //Handle<NyString> clazz(core_->factory()->NewString("coroutine"));
    pool->Add(core_->factory()->NewString("f2"), core_);
    pool->Add(core_->factory()->NewString("check1"), core_);
    
    Handle<NyByteArray> bcbuf(core_->factory()->NewByteArray(1024));
    
    // print(f2())
    bcbuf->Add(Bytecode::kPushGlobal, core_);
    bcbuf->Add(1, core_); // push g["print"]
    bcbuf->Add(Bytecode::kPushGlobal, core_);
    bcbuf->Add(0, core_); // push g["f2"]
    bcbuf->Add(Bytecode::kCall, core_);
    bcbuf->Add(1, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(-1, core_); // call l[1], 0, -1
    bcbuf->Add(Bytecode::kCall, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(-1, core_);
    bcbuf->Add(0, core_); // call l[0], -1, 0
    
    Handle<NyScript> script(core_->factory()->NewScript(nullptr, nullptr, *bcbuf, *pool));
    auto thd = N_->core()->main_thd();
    auto rv = thd->Run(*script);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, thd->frame_size());
    
    ASSERT_EQ(1, check->upval(0)->ToSmi());
    ASSERT_EQ(2, check->upval(1)->ToSmi());
    ASSERT_EQ(3, check->upval(2)->ToSmi());
    ASSERT_EQ(4, check->upval(3)->ToSmi());
}
    
} // namespace nyaa
    
} // namespace mai
