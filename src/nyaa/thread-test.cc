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
                                        void (*fp)(const FunctionCallbackInfo<Object> &)) {
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
    HandleScope scope(N_);
    
    Handle<NyByteArray> bcbuf(core_->factory()->NewByteArray(1024));
    bcbuf->Add(Bytecode::kLoadImm, core_);
    bcbuf->Add(0, core_); // load l[0], 119
    bcbuf->Add(119, core_);
    bcbuf->Add(Bytecode::kRet, core_);
    bcbuf->Add(0, core_); // ret offset+0
    bcbuf->Add(1, core_);
    
    Handle<NyClosure> script = NewClosure(bcbuf, Handle<NyArray>::Null());
    auto thd = N_->core()->curr_thd();
    auto rv = thd->Run(*script, nullptr, 0);
    ASSERT_EQ(1, rv);
}
    
static void CallingTest1(const FunctionCallbackInfo<Object> &info) {
    info.GetReturnValues()
        .Add("1st")
        .Add("2nd")
        .Add("3th");
}
  

    
TEST_F(NyaaThreadTest, Calling) {
    HandleScope scope(N_);
    
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
    auto thd = N_->core()->curr_thd();
    auto rv = thd->Run(*script, nullptr, 0, -1);
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
    HandleScope scope(N_);
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
    auto thd = N_->core()->main_thd();
    auto rv = thd->Run(*script, nullptr, 0, 3);
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
    HandleScope scope(N_);
    
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
    auto thd = N_->core()->curr_thd();
    auto rv = thd->Run(*script, nullptr, 0, 3);
    ASSERT_EQ(4, rv);
    ASSERT_EQ(3, thd->frame_size());
}
    
void ReturnVarArgs_Check(const FunctionCallbackInfo<Object> &info) {
    Handle<NyDelegated> callee = NyDelegated::Cast(*info.Callee());
    if (!callee) {
        return;
    }
    
    callee->Bind(0, *info[0], info.Core());
    callee->Bind(1, *info[1], info.Core());
    callee->Bind(2, *info[2], info.Core());
    callee->Bind(3, *info[3], info.Core());
}
  
TEST_F(NyaaThreadTest, CallVarArgs) {
    HandleScope scope(N_);
    
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
    auto thd = N_->core()->main_thd();
    auto rv = thd->Run(*script, nullptr, 0, 0);
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
    
    HandleScope scope(N_);
    
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.message()->bytes();
    auto rv = script->Call(nullptr, 0, 0, core_);
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
    
    HandleScope scope(N_);
    
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.message()->bytes();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    auto rv = script->Call(nullptr, 0, 0, core_);
    ASSERT_EQ(0, rv) << try_catch.message()->bytes();
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.message()->bytes();
}
    
void FunctionUpvals_Check(const FunctionCallbackInfo<Object> &info) {
    Handle<NyDelegated> callee = NyDelegated::Cast(*info.Callee());
    if (!callee) {
        return;
    }
    
    callee->Bind(0, *info[0], info.Core());
    callee->Bind(1, *info[1], info.Core());
    callee->Bind(2, *info[2], info.Core());
}

TEST_F(NyaaThreadTest, FunctionUpvals) {
    static const char s[] = {
        "var a, b, c = 1, 2, 3\n"
        "def foo(a) {\n"
        "   check(a, b, c)\n"
        "}\n"
        "foo(4)\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> checker(RegisterChecker("check", 3, FunctionUpvals_Check));
    
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.message()->bytes();
    auto rv = script->Call(nullptr, 0, 0, core_);
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
        "   return lambda -> a, b, c\n"
        "}\n"
        "var f = foo(4)\n"
        "check(f())\n"
    };

    HandleScope scope(N_);
    Handle<NyDelegated> checker(RegisterChecker("check", 3, FunctionUpvals_Check));
    
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.message()->bytes();
//    BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    auto rv = script->Call(nullptr, 0, 0, core_);
    ASSERT_EQ(0, rv) << try_catch.message()->bytes();
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.message()->bytes();
    
    ASSERT_EQ(4, checker->upval(0)->ToSmi());
    ASSERT_EQ(2, checker->upval(1)->ToSmi());
    ASSERT_EQ(3, checker->upval(2)->ToSmi());
}
    
TEST_F(NyaaThreadTest, MapInitializer) {
    static const char s[] = {
        "var a, b, c = 1, 2, 3\n"
        "return {\n"
        "   a, b, c, \n"
        "   a: 1,\n"
        "   b: \"ok\","
        "   [100] = 100\n"
        "}\n"
    };

    HandleScope scope(N_);
    
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.message()->bytes();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    auto rv = script->Call(nullptr, 0, 1, core_);
    ASSERT_EQ(1, rv) << try_catch.message()->bytes();
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.message()->bytes();
    ASSERT_EQ(1, core_->curr_thd()->frame_size());
    Handle<NyMap> map(core_->curr_thd()->Get(-1));
    ASSERT_TRUE(map->IsMap());
    
    Object *val = map->RawGet(NyInt32::New(0), core_);
    ASSERT_TRUE(val->IsSmi());
    ASSERT_EQ(1, val->ToSmi());
    
    val = map->RawGet(NyInt32::New(100), core_);
    ASSERT_TRUE(val->IsSmi());
    ASSERT_EQ(100, val->ToSmi());
}
    
TEST_F(NyaaThreadTest, MapLinearInitializer)  {
    static const char s[] = {
        "var a, b, c = 1, 2, 3\n"
        "return {\n"
        "   a, b, c, 4, 5, 6, 7, 8"
        "}\n"
    };
    
    HandleScope scope(N_);
    
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.message()->bytes();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    auto rv = script->Call(nullptr, 0, 1, core_);
    ASSERT_EQ(1, rv) << try_catch.message()->bytes();
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.message()->bytes();
    ASSERT_EQ(1, core_->curr_thd()->frame_size());
    Handle<NyMap> map(core_->curr_thd()->Get(-1));
    ASSERT_TRUE(map->IsMap());
    
    for (int i = 0; i < 8; ++i) {
        Handle<Object> val = map->RawGet(NyInt32::New(i), core_);
        ASSERT_TRUE(val->IsSmi());
        ASSERT_EQ(1 + i, val->ToSmi());
    }
}
    
void Values_Check(const FunctionCallbackInfo<Object> &info) {
    Handle<NyDelegated> callee = NyDelegated::Cast(*info.Callee());
    if (callee.is_not_valid()) {
        return;
    }
    for (int i = 0; i < info.Length(); ++i) {
        callee->Bind(i, *info[i], info.Core());
    }
}
    
TEST_F(NyaaThreadTest, OnlyIf)  {
    static const char s[] = {
        "var a, b, c = 1, 0, 1\n"
        "if (a) { check(3) }\n"
        "return\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 1, Values_Check);
    
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.message()->bytes();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 1, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.message()->bytes();
    
    Handle<Object> val = check->upval(0);
    ASSERT_TRUE(val->IsSmi());
    ASSERT_EQ(3, val->ToSmi());
}

TEST_F(NyaaThreadTest, IfElse)  {
    static const char s[] = {
        "var a, b, c = 0, 0, 1\n"
        "if (a) { check(1) }\n"
        "else { check(2) }\n"
        "return\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 1, Values_Check);
    
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.message()->bytes();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 1, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.message()->bytes();
    
    Handle<Object> val = check->upval(0);
    ASSERT_TRUE(val->IsSmi());
    ASSERT_EQ(2, val->ToSmi());
}
    
TEST_F(NyaaThreadTest, IfElseIf)  {
    static const char s[] = {
        "var a, b, c = nil, 0, 1\n"
        "if (c) { check(1) }\n"
        "else { check(2) }\n"
        "return\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 1, Values_Check);
    
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.message()->bytes();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 1, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.message()->bytes();

    Handle<Object> val = check->upval(0);
    ASSERT_TRUE(val->IsSmi());
    ASSERT_EQ(1, val->ToSmi());
}
    
TEST_F(NyaaThreadTest, EmptyObjectDefinition)  {
    static const char s[] = {
        "object [local] foo {}\n"
        "check(foo)"
        "return\n"
    };

    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 1, Values_Check);
    
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.message()->bytes();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 1, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.message()->bytes();

    Handle<Object> val = check->upval(0);
    ASSERT_TRUE(val->IsObject());
    Handle<NyUDO> udo = val->ToHeapObject()->ToUDO();
    ASSERT_TRUE(udo.is_valid());
}

TEST_F(NyaaThreadTest, ObjectDefinitionConstructor) {
    static const char s[] = {
        "object [local] foo {\n"
        "  def __init__(self) { check(self) }\n"
        "}\n"
        "return\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 1, Values_Check);
    
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.message()->bytes();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 1, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.message()->bytes();
    
    Handle<Object> val = check->upval(0);
    ASSERT_TRUE(val->IsObject());
    Handle<NyUDO> udo = val->ToHeapObject()->ToUDO();
    ASSERT_TRUE(udo.is_valid());
}
    
TEST_F(NyaaThreadTest, FunctionVargs) {
    static const char s[] = {
        "def foo(a, b, ...) {\n"
        "   check(a, b)\n"
        "}\n"
        "foo(1,2,3,4,5)\n"
        "return\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 2, Values_Check);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.message()->bytes();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 1, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.message()->bytes();
    
    Handle<Object> val = check->upval(0);
    ASSERT_TRUE(val->IsSmi());
    ASSERT_EQ(1, val->ToSmi());
    val = check->upval(1);
    ASSERT_TRUE(val->IsSmi());
    ASSERT_EQ(2, val->ToSmi());
}
    
TEST_F(NyaaThreadTest, FunctionVargsRef) {
    static const char s[] = {
        "def foo(a, b, ...) {\n"
        "   var c, d, e = ...\n"
        "   check(a, b, c, d, e)\n"
        "}\n"
        "foo(1,2,3,4,5)\n"
        "return\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 5, Values_Check);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.message()->bytes();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 1, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.message()->bytes();
    
    for (int i = 0; i < 5; ++i) {
        Handle<Object> val = check->upval(i);
        ASSERT_TRUE(val->IsSmi());
        ASSERT_EQ(i + 1, val->ToSmi());
    }
}
    
TEST_F(NyaaThreadTest, MapGetField) {
    static const char s[] = {
        "var t = {\n"
        "   name: \"doom\",\n"
        "   id: 100\n"
        "}\n"
        "check(t.name, t[\"id\"])\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 2, Values_Check);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    script->Call(nullptr, 0, 1, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
    
    Handle<Object> val = check->upval(0);
    ASSERT_TRUE(val->IsObject());
    Handle<NyString> v1 = Handle<NyString>::Cast(val);
    ASSERT_STREQ("doom", v1->bytes());
    val = check->upval(1);
    ASSERT_TRUE(val->IsSmi());
    ASSERT_EQ(100, val->ToSmi());
}
    
TEST_F(NyaaThreadTest, UdoSetGetField) {
    static const char s[] = {
        "object [local] foo {\n"
        "   def get() { return 199 }\n"
        "   property [ro] a, b, c\n"
        "}\n"
        "foo.a_ = 1\n"
        "check(foo.get(), foo.a, foo.b)\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 3, Values_Check);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 0, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
    
    Handle<Object> val = check->upval(0);
    ASSERT_TRUE(val->IsSmi());
    ASSERT_EQ(199, val->ToSmi());
    val = check->upval(1);
    ASSERT_TRUE(val->IsSmi());
    ASSERT_EQ(1, val->ToSmi());
    val = check->upval(2);
    ASSERT_TRUE(val->IsNil());
}
    
TEST_F(NyaaThreadTest, UdoSelfCall) {
    static const char s[] = {
        "object [local] foo {\n"
        "   def __init__(self) {\n"
        "       self.a_ = 1\n"
        "       self.b_ = 2\n"
        "       self.c_ = 3\n"
        "   }\n"
        "   def get(self) { return self.a, self.b, self.c }\n"
        "   property [ro] a, b, c\n"
        "}\n"
        "check(foo:get())\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 3, Values_Check);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 0, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
    
    for (int i = 0; i < 3; ++i) {
        Handle<Object> val = check->upval(i);
        ASSERT_TRUE(val->IsSmi());
        ASSERT_EQ(i + 1, val->ToSmi());
    }
}
    
TEST_F(NyaaThreadTest, NewUdoCallInit) {
    static const char s[] = {
        "class foo {\n"
        "   def __init__(self, a, b, c) {\n"
        "       self.a_ = a\n"
        "       self.b_ = b\n"
        "       self.c_ = c\n"
        "   }\n"
        "   def get(self) { return self.a, self.b, self.c }\n"
        "   property [ro] a, b, c\n"
        "}\n"
        "var o = new foo(1, 2, 3)\n"
        "check(o:get())\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 3, Values_Check);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 0, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
    
    for (int i = 0; i < 3; ++i) {
        Handle<Object> val = check->upval(i);
        ASSERT_TRUE(val->IsSmi());
        ASSERT_EQ(i + 1, val->ToSmi());
    }
}

TEST_F(NyaaThreadTest, NewUdoCallInitVargs) {
    static const char s[] = {
        "class foo {\n"
        "   def __init__(self, ...) {\n"
        "       self.a_, self.b_, self.c_ = ...\n"
        "   }\n"
        "   def get(self) { return self.a, self.b, self.c }\n"
        "   property [ro] a, b, c\n"
        "}\n"
        "var o = new foo(1, 2, 3)\n"
        "check(o:get())\n"
    };

    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 3, Values_Check);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 0, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
    
    for (int i = 0; i < 3; ++i) {
        Handle<Object> val = check->upval(i);
        ASSERT_TRUE(val->IsSmi());
        ASSERT_EQ(i + 1, val->ToSmi());
    }
}

TEST_F(NyaaThreadTest, PairIterate) {
    static const char s[] = {
        "var t = {\n"
        "   a: 1,\n"
        "   b: 2,\n"
        "   c: 3,\n"
        "   d: 4\n"
        "}\n"
        "var iter = pairs(t)\n"
        "print(iter())\n"
        "print(iter())\n"
        "print(iter())\n"
        "print(iter())\n"
        "print(iter())\n"
    };
    
    HandleScope scope(N_);
    //Handle<NyDelegated> check = RegisterChecker("check", 3, Values_Check);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 0, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
}
    
TEST_F(NyaaThreadTest, ForIterate) {
    static const char s[] = {
        "var t = {\n"
        "   a: 1,\n"
        "   b: 2,\n"
        "   c: 3,\n"
        "   d: 4\n"
        "}\n"
        "for k, v in pairs(t) {\n"
        "   print(k, v)\n"
        "}\n"
    };
    
    HandleScope scope(N_);
    //Handle<NyDelegated> check = RegisterChecker("check", 3, Values_Check);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 0, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
}
    
TEST_F(NyaaThreadTest, MapToStr) {
    static const char s[] = {
        "var t = {\n"
        "   a: 1,\n"
        "   b: 2,\n"
        "   c: 3,\n"
        "   d: 4\n"
        "}\n"
        "check(str(t))\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 1, Values_Check);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 0, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
    
    Handle<NyString> v1 = NyString::Cast(check->upval(0));
    ASSERT_TRUE(v1.is_valid());
    ASSERT_STREQ("{a:1, c:3, d:4, b:2}", v1->bytes());
}
    
TEST_F(NyaaThreadTest, MapMetaStr) {
    static const char s[] = {
        "var t = setmetatable({a:1, b:2}, {__str__:lambda(self) { return \"ok\" }})\n"
        "check(str(t))\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 1, Values_Check);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 0, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
    
    Handle<NyString> v1 = NyString::Cast(check->upval(0));
    ASSERT_TRUE(v1.is_valid());
    ASSERT_STREQ("ok", v1->bytes());
}
    
TEST_F(NyaaThreadTest, Concat) {
    static const char s[] = {
        "var t = setmetatable({a:1, b:2}, {__str__:lambda(self) { return \"ok\" }})\n"
        "var a, b, c = 1.1, 2, 3"
        "check(t..\",\"..a..\",\"..b..\",\"..c)\n"
    };

    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 1, Values_Check);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 0, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
    
    Handle<NyString> v1 = NyString::Cast(check->upval(0));
    ASSERT_TRUE(v1.is_valid());
    ASSERT_STREQ("ok,1.100000,2,3", v1->bytes());
}
    
TEST_F(NyaaThreadTest, UdoMetaStr) {
    static const char s[] = {
        "object [local] foo {\n"
        "   def __init__(self) { self.a_, self.b_ = 1, 2 }\n"
        "   def __str__(self) { return self.a..\",\"..self.b }\n"
        "   property [ro] a, b\n"
        "}\n"
        "check(str(foo))\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 1, Values_Check);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 0, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
    
    Handle<NyString> v1 = NyString::Cast(check->upval(0));
    ASSERT_TRUE(v1.is_valid());
    ASSERT_STREQ("1,2", v1->bytes());
}

TEST_F(NyaaThreadTest, ArithAdd) {
    static const char s[] = {
        "var a, b, c = 1, 2, 3"
        "check(a + 1, a + b, c + -4)\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 3, Values_Check);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 0, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
    
    ASSERT_EQ(2, check->upval(0)->ToSmi());
    ASSERT_EQ(3, check->upval(1)->ToSmi());
    ASSERT_EQ(-1, check->upval(2)->ToSmi());
}

TEST_F(NyaaThreadTest, ArithAddOverflow) {
    static const char s[] = {
        "var a, b = 1, 2305843009213693951"
        "check(a + b)\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 1, Values_Check);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 0, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
    
    Handle<NyInt> v1 = NyInt::Cast(check->upval(0));
    ASSERT_TRUE(v1.is_valid());
    ASSERT_STREQ("2305843009213693952", v1->ToString(core_)->bytes());
}

TEST_F(NyaaThreadTest, ArithSub) {
    static const char s[] = {
        "var a, b, c = 1, 2, 3"
        "check(a - 1, a - b, c - -4)\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 3, Values_Check);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 0, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
    
    ASSERT_EQ(0, check->upval(0)->ToSmi());
    ASSERT_EQ(-1, check->upval(1)->ToSmi());
    ASSERT_EQ(7, check->upval(2)->ToSmi());
}

TEST_F(NyaaThreadTest, AndOrSwitch) {
    static const char s[] = {
        "var a, b = 1, 0"
        "check(a and b, a or b)\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 2, Values_Check);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 0, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
    
    ASSERT_EQ(0, check->upval(0)->ToSmi());
    ASSERT_EQ(1, check->upval(1)->ToSmi());
}

TEST_F(NyaaThreadTest, CheckStack) {
    NyaaOptions opts;
    opts.init_thread_stack_size = 4;
    Nyaa N(opts, isolate_);
    
    static const char s[] = {
        "var a, b, c, d = 1, 2, 3, 4\n"
        "return a + b + c + d\n"
    };
    HandleScope scope(&N);
    TryCatchCore try_catch(N.core());
    auto script = NyClosure::Compile(s, N.core());
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    script->Call(nullptr, 0, 1, N.core());
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
    
    ASSERT_EQ(N.core()->curr_thd()->frame_size(), 1);
    ASSERT_EQ(10, N.core()->curr_thd()->Get(-1)->ToSmi());
}

TEST_F(NyaaThreadTest, Log) {
    static const char f[] = "tests/nyaa/00-do-file.nyaa";
    HandleScope scope(N_);
    TryCatchCore try_catch(core_);
    FILE *fp = fopen(f, "r");
    NyClosure::Do(f, fp, 0, nullptr, core_);
    fclose(fp);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
}
    
TEST_F(NyaaThreadTest, LoadFileError) {
    static const char s[] = {
        "check(loadfile(\"tests/nyaa/03-error-file.nyaa\"))\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 2, Values_Check);
    TryCatchCore try_catch(core_);
    NyClosure::Do(s, arraysize(s), 0/*wanted*/, nullptr, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
    
    ASSERT_EQ(Object::kNil, check->upval(0));
    NyString *val = NyString::Cast(check->upval(1));
    ASSERT_NE(nullptr, val);
}
    
TEST_F(NyaaThreadTest, LoadFile) {
    static const char s[] = {
        "var f = loadfile(\"tests/nyaa/01-foo-file.nyaa\")\n"
        "check(f())"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 4, Values_Check);
    TryCatchCore try_catch(core_);
    NyClosure::Do(s, arraysize(s), 0/*wanted*/, nullptr, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
    
    NyString *val = NyString::Cast(check->upval(0));
    ASSERT_NE(nullptr, val);
    ASSERT_STREQ("foo", val->bytes());
    ASSERT_EQ(1, check->upval(1)->ToSmi());
    ASSERT_EQ(2, check->upval(2)->ToSmi());
    ASSERT_EQ(3, check->upval(3)->ToSmi());
}

TEST_F(NyaaThreadTest, Require) {
    static const char s[] = {
        "var a = require \"tests/nyaa/01-foo-file.nyaa\"\n"
        "var b, c, d, e = require \"tests/nyaa/02-bar-file.nyaa\"\n"
        "check(a, b)\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 2, Values_Check);
    TryCatchCore try_catch(core_);
    NyClosure::Do(s, arraysize(s), 0/*wanted*/, nullptr, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
    
    NyString *val = NyString::Cast(check->upval(0));
    ASSERT_NE(nullptr, val);
    ASSERT_STREQ("foo", val->bytes());
    val = NyString::Cast(check->upval(1));
    ASSERT_NE(nullptr, val);
    ASSERT_STREQ("bar", val->bytes());
}

TEST_F(NyaaThreadTest, TopScriptArgs) {
    static const char s[] = {
        "var a, b, c = ..."
        "check(a, b, c)\n"
    };
    
    HandleScope scope(N_);
    Handle<NyDelegated> check = RegisterChecker("check", 3, Values_Check);
    TryCatchCore try_catch(core_);
    auto script = NyClosure::Compile(s, core_);
    ASSERT_TRUE(script.is_not_empty()) << try_catch.ToString();
    //BytecodeArrayDisassembler::Disassembly(core_, script->proto(), stdout);
    Object *argv[] = {NySmi::New(1), NySmi::New(2), NySmi::New(3)};
    script->Call(argv, 3, 0, core_);
    ASSERT_FALSE(try_catch.has_caught()) << try_catch.ToString();
    
    ASSERT_EQ(1, check->upval(0)->ToSmi());
    ASSERT_EQ(2, check->upval(1)->ToSmi());
    ASSERT_EQ(3, check->upval(2)->ToSmi());
}

} // namespace nyaa
    
} // namespace mai
