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
    Handle<NyFunction> BuildRet3(const char *name) {
        Handle<NyByteArray> bcbuf(core_->factory()->NewByteArray(1024));
        
        bcbuf->Add(Bytecode::kLoadImm, core_);
        bcbuf->Add(0, core_);
        bcbuf->Add(1, core_); // load l[0], 1
        bcbuf->Add(Bytecode::kLoadImm, core_);
        bcbuf->Add(1, core_);
        bcbuf->Add(2, core_); // load l[1], 2
        bcbuf->Add(Bytecode::kLoadImm, core_);
        bcbuf->Add(2, core_);
        bcbuf->Add(3, core_); // load l[2], 3
        bcbuf->Add(Bytecode::kLoadImm, core_);
        bcbuf->Add(3, core_);
        bcbuf->Add(4, core_); // load l[3], 4
        bcbuf->Add(Bytecode::kRet, core_);
        bcbuf->Add(0, core_); // ret offset+0, 4
        bcbuf->Add(4, core_);
        
        Handle<NyFunction> fn =
        factory_->NewFunction(nullptr/*name*/, 0/*n_params*/, false/*vargs*/, 0/*n_upvals*/,
                              4/*max_stack*/, nullptr/*file_name*/, nullptr/*file_info*/, *bcbuf,
                              nullptr/*proto_pool*/, nullptr/*const_pool*/);
        core_->SetGlobal(factory_->NewString(name), factory_->NewClosure(*fn));
        return fn;
    }

    // def f2() { return name() }
    Handle<NyFunction> BuildRetFn(const char *name, const char *callee) {
        Handle<NyArray> pool(core_->factory()->NewArray(4));
        pool->Add(factory_->NewString(callee), core_);
        
        Handle<NyByteArray> bcbuf(core_->factory()->NewByteArray(1024));

        bcbuf->Add(Bytecode::kLoadGlobal, core_);
        bcbuf->Add(0, core_);
        bcbuf->Add(0, core_); // load l[0], g[k[0]]
        bcbuf->Add(Bytecode::kCall, core_);
        bcbuf->Add(0, core_);
        bcbuf->Add(0, core_);
        bcbuf->Add(-1, core_); // call l[0], 0, -1
        bcbuf->Add(Bytecode::kRet, core_);
        bcbuf->Add(0, core_); // ret offset+0, -1
        bcbuf->Add(-1, core_);
        
        Handle<NyFunction> fn =
        factory_->NewFunction(nullptr/*name*/, 0/*n_params*/, false/*vargs*/, 0/*n_upvals*/,
                              4/*max_stack*/, nullptr/*file_name*/, nullptr/*file_info*/, *bcbuf,
                              nullptr/*proto_pool*/, *pool/*const_pool*/);
        core_->SetGlobal(factory_->NewString(name), factory_->NewClosure(*fn));
        return fn;
    }

    Handle<NyDelegated> RegisterChecker(const char *name, int n_upvals,
                                        int (*fp)(Arguments *, Nyaa *)) {
        Handle<NyDelegated> fn(factory_->NewDelegated(fp, n_upvals));
        core_->SetGlobal(factory_->NewString(name), *fn);
        return fn;
    }
    
    Handle<NyClosure> NewClosure(Handle<NyByteArray> bcbuf, Handle<NyArray> kpool) {
        Handle<NyFunction> fn =
        factory_->NewFunction(nullptr/*name*/, 0/*n_params*/, false/*vargs*/, 0/*n_upvals*/,
                              4/*max_stack*/, nullptr/*file_name*/, nullptr/*file_info*/, *bcbuf,
                              nullptr/*proto_pool*/, *kpool/*const_pool*/);
        return factory_->NewClosure(*fn);
    }
    
    Nyaa *N_ = nullptr;
    NyaaCore *core_ = nullptr;
    ObjectFactory *factory_ = nullptr;
    
    //return Yield(1, 2, 3);
};
    
TEST_F(NyaaThreadTest, Sanity) {
    HandleScope scope(isolate_);
    
    Handle<NyByteArray> bcbuf(core_->factory()->NewByteArray(1024));
    bcbuf->Add(Bytecode::kLoadImm, core_);
    bcbuf->Add(0, core_); // load l[0], 119
    bcbuf->Add(119, core_);
    bcbuf->Add(Bytecode::kRet, core_);
    bcbuf->Add(0, core_); // ret offset+0
    bcbuf->Add(1, core_);
    
    Handle<NyClosure> script = NewClosure(bcbuf, Handle<NyArray>::Null());
    Arguments args(0);
    auto thd = N_->core()->curr_thd();
    auto rv = thd->Run(*script, &args);
    ASSERT_EQ(1, rv);
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
    bcbuf->Add(Bytecode::kLoadGlobal, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(Bytecode::kCall, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(3, core_);
    bcbuf->Add(Bytecode::kRet, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(-1, core_);

    Handle<NyClosure> script = NewClosure(bcbuf, pool);
    Arguments args(0);
    auto thd = N_->core()->curr_thd();
    auto rv = thd->Run(*script, &args, -1);
    ASSERT_EQ(3, rv);
    ASSERT_EQ(3, thd->frame_size());
    
    Handle<NyString> r1(thd->Get(0));
    ASSERT_STREQ("1st", r1->bytes());
    r1 = thd->Get(1);
    ASSERT_STREQ("2nd", r1->bytes());
    r1 = thd->Get(2);
    ASSERT_STREQ("3th", r1->bytes());
}
    
TEST_F(NyaaThreadTest, Return) {
    HandleScope scope(isolate_);
    Handle<NyByteArray> bcbuf(core_->factory()->NewByteArray(1024));
    
    // return 1, 2, 3
    bcbuf->Add(Bytecode::kLoadImm, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(1, core_);
    bcbuf->Add(Bytecode::kLoadImm, core_);
    bcbuf->Add(1, core_);
    bcbuf->Add(2, core_);
    bcbuf->Add(Bytecode::kLoadImm, core_);
    bcbuf->Add(2, core_);
    bcbuf->Add(3, core_);
    bcbuf->Add(Bytecode::kRet, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(3, core_);
    
    Handle<NyClosure> script = NewClosure(bcbuf, Handle<NyArray>::Null());
    Arguments args(0);
    auto thd = N_->core()->main_thd();
    auto rv = thd->Run(*script, &args, 3);
    ASSERT_EQ(3, rv);
    ASSERT_EQ(3, thd->frame_size());
    
    Handle<Object> r1(thd->Get(0));
    ASSERT_EQ(1, r1->ToSmi());
    r1 = thd->Get(1);
    ASSERT_EQ(2, r1->ToSmi());
    r1 = thd->Get(2);
    ASSERT_EQ(3, r1->ToSmi());
}
    
TEST_F(NyaaThreadTest, CallVarResults) {
    HandleScope scope(isolate_);
    
    BuildRet3("f1");
    BuildRetFn("f2", "f1");
    
    Handle<NyArray> pool(core_->factory()->NewArray(4));
    pool->Add(factory_->NewString("f2"), core_);
    Handle<NyByteArray> bcbuf(core_->factory()->NewByteArray(1024));
    
    bcbuf->Add(Bytecode::kLoadGlobal, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(Bytecode::kCall, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(-1, core_);
    bcbuf->Add(Bytecode::kRet, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(-1, core_);
    
    Handle<NyClosure> script = NewClosure(bcbuf, pool);
    Arguments args(0);
    auto thd = N_->core()->curr_thd();
    auto rv = thd->Run(*script, &args, 3);
    ASSERT_EQ(4, rv);
    ASSERT_EQ(3, thd->frame_size());
}
    
int ReturnVarArgs_Check(Arguments *args, Nyaa *N) {
    Local<Function> callee = Local<Function>::Cast(args->Callee());
    if (!callee) {
        return -1;
    }
    
    callee->Bind(0, args->Get(0));
    callee->Bind(1, args->Get(1));
    callee->Bind(2, args->Get(2));
    callee->Bind(3, args->Get(3));
    return 0;
}
  
TEST_F(NyaaThreadTest, CallVarArgs) {
    HandleScope scope(isolate_);
    
    BuildRet3("f1");
    Handle<NyDelegated> fn(RegisterChecker("check1", 4, ReturnVarArgs_Check));
    
    Handle<NyArray> pool(core_->factory()->NewArray(4));
    pool->Add(factory_->NewString("check1"), core_);
    pool->Add(factory_->NewString("f1"), core_);
    Handle<NyByteArray> bcbuf(core_->factory()->NewByteArray(1024));
    
    // check1(f1())
    bcbuf->Add(Bytecode::kLoadGlobal, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(Bytecode::kLoadGlobal, core_);
    bcbuf->Add(1, core_);
    bcbuf->Add(1, core_);
    bcbuf->Add(Bytecode::kCall, core_); // call f1
    bcbuf->Add(1, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(-1, core_);
    bcbuf->Add(Bytecode::kCall, core_); // call check1
    bcbuf->Add(0, core_);
    bcbuf->Add(-1, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(Bytecode::kRet, core_);
    bcbuf->Add(0, core_);
    bcbuf->Add(0, core_);
    
    Handle<NyClosure> script = NewClosure(bcbuf, pool);
    Arguments args(0);
    auto thd = N_->core()->main_thd();
    auto rv = thd->Run(*script, &args, 0);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, thd->frame_size());
    
    ASSERT_EQ(1, fn->upval(0)->ToSmi());
    ASSERT_EQ(2, fn->upval(1)->ToSmi());
    ASSERT_EQ(3, fn->upval(2)->ToSmi());
    ASSERT_EQ(4, fn->upval(3)->ToSmi());
}
    
TEST_F(NyaaThreadTest, Raise) {
    static const char s[] = {
        "print(\"----1-----\")\n"
        "print(\"----2-----\")\n"
        "raise(\"error!\")\n"
        "print(\"----3-----\")\n"
    };
    
    HandleScope scope(isolate_);
    
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Do(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.message()->bytes();
    Arguments args(0);
    auto rv = script->Call(&args, 0, core_);
    ASSERT_GT(0, rv);
    ASSERT_TRUE(try_catch.has_caught());
    ASSERT_STREQ("error!", try_catch.message()->bytes());
}
    
TEST_F(NyaaThreadTest, FunctionDefinition) {
    static const char s[] = {
        "def foo(a) { print(\"params:\", a) }\n"
        "foo(1)\n"
        "foo(2)\n"
    };
    
    HandleScope scope(isolate_);
    
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Do(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.message()->bytes();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    Arguments args(0);
    auto rv = script->Call(&args, 0, core_);
    ASSERT_EQ(0, rv) << try_catch.message()->bytes();
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.message()->bytes();
}
    
int FunctionUpvals_Check(Arguments *args, Nyaa *N) {
    Local<Function> callee = Local<Function>::Cast(args->Callee());
    if (!callee) {
        return -1;
    }
    
    callee->Bind(0, args->Get(0));
    callee->Bind(1, args->Get(1));
    callee->Bind(2, args->Get(2));
    return 0;
}

TEST_F(NyaaThreadTest, FunctionUpvals) {
    static const char s[] = {
        "var a, b, c = 1, 2, 3\n"
        "def foo(a) {\n"
        "   check(a, b, c)\n"
        "}\n"
        "foo(4)\n"
    };
    
    HandleScope scope(isolate_);
    Handle<NyDelegated> checker(RegisterChecker("check", 3, FunctionUpvals_Check));
    
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Do(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.message()->bytes();
    Arguments args(0);
    auto rv = script->Call(&args, 0, core_);
    ASSERT_EQ(0, rv) << try_catch.message()->bytes();
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.message()->bytes();
    
    ASSERT_EQ(4, checker->upval(0)->ToSmi());
    ASSERT_EQ(2, checker->upval(1)->ToSmi());
    ASSERT_EQ(3, checker->upval(2)->ToSmi());
}

TEST_F(NyaaThreadTest, FunctionUpvals2) {
    static const char s[] = {
        "var a, b, c = 1, 2, 3\n"
        "def foo(a) {\n"
        "   return lambda { return a, b, c }\n"
        "}\n"
        "var f = foo(4)\n"
        "check(f())\n"
    };

    HandleScope scope(isolate_);
    Handle<NyDelegated> checker(RegisterChecker("check", 3, FunctionUpvals_Check));
    
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Do(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.message()->bytes();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    Arguments args(0);
    auto rv = script->Call(&args, 0, core_);
    ASSERT_EQ(0, rv) << try_catch.message()->bytes();
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.message()->bytes();
    
    ASSERT_EQ(4, checker->upval(0)->ToSmi());
    ASSERT_EQ(2, checker->upval(1)->ToSmi());
    ASSERT_EQ(3, checker->upval(2)->ToSmi());
}

} // namespace nyaa
    
} // namespace mai
