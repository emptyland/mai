#include "lang/coroutine.h"
#include "lang/scheduler.h"
#include "lang/heap.h"
#include "lang/metadata-space.h"
#include "lang/value-inl.h"
#include "lang/runtime.h"
#include "lang/channel.h"
#include "lang/bytecode-array-builder.h"
#include "lang/stack-frame.h"
#include "lang/stable-space-builder.h"
#include "asm/utils.h"
#include "test/isolate-initializer.h"
#include "base/arenas.h"
#include "base/slice.h"
#include "gtest/gtest.h"
#include <thread>
//#include <sys/time.h>

namespace mai {

namespace lang {

struct DummyResult {
    int8_t  i8_1,  i8_2,  i8_3;
    int16_t i16_1, i16_2, i16_3;
    int32_t i32_1, i32_2, i32_3;
    int64_t i64_1, i64_2, i64_3;
    float   f32_1, f32_2, f32_3;
    double  f64_1, f64_2, f64_3;
    Any    *any_1,*any_2,*any_3;
};

static DummyResult dummy_result;

class DummyClass : public Any {
public:
    DummyClass(const Class *clazz, uint32_t tags): Any(clazz, tags) {}
    
    int32_t padding_;
    int8_t i8_1_;
    int8_t i8_2_;
    int16_t i16_1_;
    int32_t i32_1_;
    int64_t i64_1_;
    float f32_1_;
    double f64_1_;
    Any *any_1_;
};

class CoroutineTest : public test::IsolateInitializer {
public:
    // Dummy
    void SetUp() override {
        test::IsolateInitializer::SetUp();
        scheduler_ = isolate_->scheduler();
        metadata_ = isolate_->metadata_space();
        arena_ = new base::StandaloneArena(isolate_->env()->GetLowLevelAllocator());
        ::memset(&dummy_result, 0, sizeof(dummy_result));
    }
    
    void TearDown() override {
        delete arena_;
        scheduler_ = nullptr;
        metadata_ = nullptr;
        test::IsolateInitializer::TearDown();
    }
    
    template<class T>
    inline Local<CapturedValue> NewCapturedValue(T value) {
        CapturedValue *val = Machine::This()->NewCapturedValue(STATE->builtin_type(TypeTraits<T>::kType),
                                                               &value, sizeof(value), 0);
        return Local<CapturedValue>(val);
    }
    
    static constexpr int32_t kStackSize = RoundUp(BytecodeStackFrame::kOffsetHeaderSize + 32,
                                                  kStackAligmentSize);

    Local<Closure> BuildDummyClosure(const std::vector<BytecodeInstruction> &instrs,
                                      const std::vector<Span32> &const_pool = {},
                                      const std::vector<uint32_t> &const_pool_bitmap = {}) {
        Function *func = BuildDummyFunction(instrs, const_pool, const_pool_bitmap);
        Closure *closure = Machine::This()->NewClosure(func, 5, Closure::kMaiFunction);
        return Local<Closure>(closure);
    }

    Function *BuildDummyFunction(const std::vector<BytecodeInstruction> &instrs,
                                 const std::vector<Span32> &const_pool,
                                 const std::vector<uint32_t> &const_pool_bitmap) {
        SourceLineInfo *info = nullptr;
        if (!instrs.empty()) {
            std::vector<int> lines;
            for (size_t i = 0; i < instrs.size(); i++) {
                lines.push_back(static_cast<int>(i + 1));
            }
            info = metadata_->NewSourceLineInfo("dummy.mai", lines);
        }
        
        BytecodeArray *bc_array = metadata_->NewBytecodeArray(instrs);
        return FunctionBuilder("test.dummy")
            .prototype({}, false, kType_void)
            .stack_size(kStackSize)
            .stack_bitmap({0})
            .const_pool(const_pool)
            .const_pool_bitmap(const_pool_bitmap)
            .bytecode(bc_array)
            .source_line_info(info)
        .Build(metadata_);
    }
    
    const Class *RegisterDummyClass() {
        constexpr arch::ObjectTemplate<DummyClass, uint32_t> templ;
        
        return ClassBuilder("dummy.DummyClass")
            .base(STATE->builtin_type(kType_any))
            .tags(Type::kReferenceTag)
            .reference_size(kPointerSize)
            .instrance_size(sizeof(DummyClass))
            /*[0]*/ .field("i8_1_")
                .type(STATE->builtin_type(kType_i8))
                .flags(Field::kRdWr|Field::kPublic)
                .tag(1)
                .offset(templ.Offset(&DummyClass::i8_1_))
            .End()
            /*[1]*/ .field("i8_2_")
                .type(STATE->builtin_type(kType_i8))
                .flags(Field::kRdWr|Field::kPublic)
                .tag(2)
                .offset(templ.Offset(&DummyClass::i8_2_))
            .End()
            /*[2]*/ .field("i16_1_")
                .type(STATE->builtin_type(kType_i16))
                .flags(Field::kRdWr|Field::kPublic)
                .tag(3)
                .offset(templ.Offset(&DummyClass::i16_1_))
            .End()
            /*[3]*/ .field("i32_1_")
                .type(STATE->builtin_type(kType_i32))
                .flags(Field::kRdWr|Field::kPublic)
                .tag(4)
                .offset(templ.Offset(&DummyClass::i32_1_))
            .End()
            /*[4]*/ .field("i64_1_")
                .type(STATE->builtin_type(kType_i64))
                .flags(Field::kRdWr|Field::kPublic)
                .tag(5)
                .offset(templ.Offset(&DummyClass::i64_1_))
            .End()
            /*[5]*/ .field("f32_1_")
                .type(STATE->builtin_type(kType_f32))
                .flags(Field::kRdWr|Field::kPublic)
                .tag(6)
                .offset(templ.Offset(&DummyClass::f32_1_))
            .End()
            /*[6]*/ .field("f64_1_")
                .type(STATE->builtin_type(kType_f64))
                .flags(Field::kRdWr|Field::kPublic)
                .tag(7)
                .offset(templ.Offset(&DummyClass::f64_1_))
            .End()
            /*[7]*/ .field("any_1_")
                .type(STATE->builtin_type(kType_any))
                .flags(Field::kRdWr|Field::kPublic)
                .tag(8)
                .offset(templ.Offset(&DummyClass::any_1_))
            .End()
        .Build(STATE->metadata_space());
    }
    
    // 257 : -1
    // 255 : 1
    int stack_offset(int off) {
        return (kParameterSpaceOffset + off) / kStackOffsetGranularity;
    }
    
    int parameter_offset(int off) {
        return (kParameterSpaceOffset - off - kPointerSize * 2) / kStackOffsetGranularity;
    }
    
    int global_offset(int off) {
        return off / kGlobalSpaceOffsetGranularity;
    }
    
    int const_offset(int off) {
        return off / kConstPoolOffsetGranularity;
    }

    Scheduler *scheduler_ = nullptr;
    MetadataSpace *metadata_ = nullptr;
    base::Arena *arena_ = nullptr;
};

static void Dummy1(int a, int b, int c) {
    dummy_result.i32_1 = a;
    dummy_result.i32_2 = b;
    dummy_result.i32_3 = c;
}

static void Dummy2(int64_t a, float b) {
    dummy_result.i64_1 = a;
    dummy_result.f32_1 = b;
}

TEST_F(CoroutineTest, Sanity) {
    printf("Max bytecodes: %d\n", kMax_Bytecodes);
    
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    Local<Closure> entry(FunctionTemplate::New(Dummy1));
    
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    ASSERT_NE(nullptr, co);
    ASSERT_EQ(*entry, co->entry());
    
    uint8_t mock_data[32] = {0};
    ::memset(mock_data, 0xcc, sizeof(mock_data));
    
    CallStub<intptr_t(Coroutine *)> call_stub(metadata_->function_template_dummy_code());
    {
        int a = 1;
        int b = 2;
        int c = 3;
        ::memcpy(&mock_data[4], &c, sizeof(c));
        ::memcpy(&mock_data[8], &b, sizeof(b));
        ::memcpy(&mock_data[12], &a, sizeof(a));
        co->CopyArgv(mock_data, 16);
        call_stub.entry()(co);
        ASSERT_EQ(a, dummy_result.i32_1);
        ASSERT_EQ(b, dummy_result.i32_2);
        ASSERT_EQ(c, dummy_result.i32_3);
    }
    ASSERT_EQ(1, co->reentrant());
    scheduler_->PurgreCoroutine(co);
    
    ::memset(mock_data, 0, sizeof(mock_data));
    {
        entry = FunctionTemplate::New(Dummy2);
        co = scheduler_->NewCoroutine(*entry, true/*co0*/);
        int64_t a = 0x0123456789;
        float b = .112;
        ::memcpy(&mock_data[4], &b, sizeof(b));
        ::memcpy(&mock_data[8], &a, sizeof(a));
        co->CopyArgv(mock_data, 16);
        call_stub.entry()(co);
        ASSERT_EQ(a, dummy_result.i64_1);
        ASSERT_EQ(b, dummy_result.f32_1);
    }
}

static void Dummy3(int64_t a, float b, String *c) {
    dummy_result.i64_1 = a;
    dummy_result.f32_1 = b;
    dummy_result.any_1 = c;
    std::this_thread::sleep_for(std::chrono::milliseconds(0));
}

TEST_F(CoroutineTest, CallNativeFunctionDummy3) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    Local<Closure> entry(FunctionTemplate::New(Dummy3));
    
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    ASSERT_NE(nullptr, co);
    ASSERT_EQ(*entry, co->entry());
    
    uint8_t mock_data[32] = {0};
    ::memset(mock_data, 0xcc, sizeof(mock_data));
    
    CallStub<intptr_t(Coroutine *)> call_stub(metadata_->function_template_dummy_code());
    Local<String> handle(String::NewUtf8("Hello"));
    
    *reinterpret_cast<String **>(&mock_data[12]) = *handle;
    *reinterpret_cast<float *>(&mock_data[20]) = 1.12;
    *reinterpret_cast<int64_t *>(&mock_data[24]) = 110;
    co->CopyArgv(mock_data, 32);
    call_stub.entry()(co);
    ASSERT_EQ(110, dummy_result.i64_1);
    ASSERT_NEAR(1.12, dummy_result.f32_1, 0.001);
    ASSERT_EQ(*handle, dummy_result.any_1);
}

static void Dummy4(int16_t a, int8_t b) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    dummy_result.i16_1 = a;
    dummy_result.i8_1 = b;
    
    Local<String> handle(String::NewUtf8("Hello"));
    dummy_result.any_1 = *handle;
}

TEST_F(CoroutineTest, CallNativeFunctionDummy4) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    Local<Closure> entry(FunctionTemplate::New(Dummy4));
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);

    uint8_t mock_data[16] = {0};
    ::memset(mock_data, 0xcc, sizeof(mock_data));
    
    CallStub<intptr_t(Coroutine *)> call_stub(metadata_->function_template_dummy_code());
    *reinterpret_cast<int8_t *>(&mock_data[8]) = 127;
    *reinterpret_cast<int16_t *>(&mock_data[12]) = 270;
    co->CopyArgv(mock_data, sizeof(mock_data));
    call_stub.entry()(co);
    
    ASSERT_EQ(270, dummy_result.i16_1);
    ASSERT_EQ(127, dummy_result.i8_1);
    Local<String> handle(static_cast<String *>(dummy_result.any_1));
    ASSERT_STREQ("Hello", handle->data());
}

TEST_F(CoroutineTest, RunBytecodeFunctionSanity) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    
    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();
    builder.Add<kReturn>();

    Local<Closure> entry(BuildDummyClosure(builder.Build()));
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    CallStub<intptr_t(Coroutine *)> trampoline(metadata_->trampoline_code());

    co->set_state(Coroutine::kRunnable);;
    trampoline.entry()(co);
    
    ASSERT_EQ(Coroutine::kDead, co->state());
    ASSERT_EQ(0, co->yield());
    ASSERT_EQ(1, co->reentrant());
}

static void Dummy5() {
    printf("Hello, World\n");
    dummy_result.i32_1 = 1;
    dummy_result.i32_2 = 2;
    dummy_result.i32_3 = 3;
}

TEST_F(CoroutineTest, BytecodeCallNativeFunction) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    
    //int32_t local_var_base = RoundUp(BytecodeStackFrame::kOffsetHeaderSize, kStackAligmentSize);
    
    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();
    builder.Add<kLdaConstPtr>(0);
    builder.Add<kCallNativeFunction>(0);
    builder.Add<kReturn>();

    Local<Closure> dummy(FunctionTemplate::New(Dummy5));
    Span32 span;
    span.ptr[0].any = *dummy;

    Local<Closure> entry(BuildDummyClosure(builder.Build(), {span}, {0x1}));
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    CallStub<intptr_t(Coroutine *)> trampoline(metadata_->trampoline_code());
    
    co->set_state(Coroutine::kRunnable);
    trampoline.entry()(co);
    
    ASSERT_EQ(1, dummy_result.i32_1);
    ASSERT_EQ(2, dummy_result.i32_2);
    ASSERT_EQ(3, dummy_result.i32_3);
}

static void Dummy6(int a, int64_t b, float c) {
    dummy_result.i32_1 = a;
    dummy_result.i64_1 = b;
    dummy_result.f32_1 = c;
}

TEST_F(CoroutineTest, BytecodeCallNativeFunctionWithArguments) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    
    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();

    builder.Add<kLdaConstf32>(9);
    builder.Add<kStaf32>(stack_offset(kStackSize + 16));
    
    builder.Add<kLdaConst64>(10);
    builder.Add<kStar64>(stack_offset(kStackSize + 12));
    
    builder.Add<kLdaConst32>(8);
    builder.Add<kStar32>(stack_offset(kStackSize + 4));
    
    builder.Add<kLdaConstPtr>(0);
    builder.Add<kCallNativeFunction>(16);
    builder.Add<kReturn>();

    Local<Closure> dummy(FunctionTemplate::New(Dummy6));
    Span32 span[2];
    span[0].ptr[0].any = *dummy;
    span[1].v32[0].i32 = 101;
    span[1].v32[1].f32 = 101.1112;
    span[1].v64[1].i64 = -1;

    Local<Closure> entry(BuildDummyClosure(builder.Build(), {span[0], span[1]}, {0x1}));
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    CallStub<intptr_t(Coroutine *)> trampoline(metadata_->trampoline_code());
    
    co->set_state(Coroutine::kRunnable);
    trampoline.entry()(co);
    
    ASSERT_EQ(101, dummy_result.i32_1);
    ASSERT_EQ(-1, dummy_result.i64_1);
    ASSERT_NEAR(101.1112, dummy_result.f32_1, 0.0001);
}

static void Dummy7() {
    dummy_result.i32_2 = 404;
    dummy_result.i32_1++;
}

TEST_F(CoroutineTest, BytecodeYield) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);

    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();
    builder.Add<kLdaConstPtr>(0);
    builder.Add<kCallNativeFunction>(0);
    builder.Add<kYield>(YIELD_FORCE);
    builder.Add<kLdaConstPtr>(0);
    builder.Add<kCallNativeFunction>(0);
    builder.Add<kReturn>();

    Local<Closure> dummy(FunctionTemplate::New(Dummy7));
    Span32 span;
    span.ptr[0].any = *dummy;

    Local<Closure> entry(BuildDummyClosure(builder.Build(), {span}, {0x1}));
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    CallStub<intptr_t(Coroutine *)> trampoline(metadata_->trampoline_code());

    co->set_state(Coroutine::kRunnable);
    trampoline.entry()(co);
    ASSERT_EQ(Coroutine::kInterrupted, co->state());
    ASSERT_EQ(1, dummy_result.i32_1);

    co->set_state(Coroutine::kRunnable);
    trampoline.entry()(co);
    ASSERT_EQ(Coroutine::kDead, co->state());
    ASSERT_EQ(2, dummy_result.i32_1);
}

static void Dummy8() {
    dummy_result.i32_2 = 108;
    dummy_result.i32_1++;
    //printf("Hello, Internal\n");
}

TEST_F(CoroutineTest, BytecodeCallBytecodeFunction) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    
    Local<Closure> callee;
    {
        BytecodeArrayBuilder builder(arena_);
        builder.Add<kCheckStack>();
        builder.Add<kLdaConstPtr>(0);
        builder.Add<kCallNativeFunction>(0);
        builder.Add<kYield>(YIELD_FORCE);
        builder.Add<kLdaConstPtr>(0);
        builder.Add<kCallNativeFunction>(0);
        builder.Add<kReturn>();

        Local<Closure> dummy(FunctionTemplate::New(Dummy8));
        Span32 span;
        span.ptr[0].any = *dummy;
        
        callee = BuildDummyClosure(builder.Build(), {span}, {0x1});
    }

    Local<Closure> caller;
    {
        BytecodeArrayBuilder builder(arena_);
        builder.Add<kCheckStack>();
        builder.Add<kLdaConstPtr>(0);
        builder.Add<kCallBytecodeFunction>(0);
        builder.Add<kReturn>();
        
        Span32 span;
        span.ptr[0].any = *callee;
        
        caller = BuildDummyClosure(builder.Build(), {span}, {0x1});
    }

    Coroutine *co = scheduler_->NewCoroutine(*caller, false/*co0*/);
    CallStub<intptr_t(Coroutine *)> trampoline(metadata_->trampoline_code());

    co->set_state(Coroutine::kRunnable);
    trampoline.entry()(co);
    ASSERT_EQ(Coroutine::kInterrupted, co->state());

    co->SwitchState(Coroutine::kInterrupted, Coroutine::kRunnable);
    trampoline.entry()(co);
    ASSERT_EQ(Coroutine::kDead, co->state());
    
    ASSERT_EQ(108, dummy_result.i32_2);
    ASSERT_EQ(2, dummy_result.i32_1);
}

TEST_F(CoroutineTest, ScheduleSanity) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();
    builder.Add<kLdaConstPtr>(0);
    builder.Add<kCallNativeFunction>(0);
    builder.Add<kReturn>();

    Local<Closure> dummy(FunctionTemplate::New(Dummy5));
    Span32 span;
    span.ptr[0].any = *dummy;
    
    Local<Closure> entry(BuildDummyClosure(builder.Build(), {span}, {0x1}));
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
    scheduler_->machine(0)->PostRunnable(co);
    
    co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
    scheduler_->machine(1)->PostRunnable(co);

    STATE->Run();
}

static void Dummy9() {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    
    dummy_result.i32_2 = 250;
    dummy_result.i32_1++;
    
    Local<Throwable> exception(Panic::New(1, String::NewUtf8("Test")));
    Throwable::Throw(exception);
}

TEST_F(CoroutineTest, ThrowException) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();
    builder.Add<kLdaConstPtr>(0);
    builder.Add<kCallNativeFunction>(0);
    builder.Add<kReturn>();

    Local<Closure> dummy(FunctionTemplate::New(Dummy9));
    Span32 span;
    span.ptr[0].any = *dummy;
    
    Local<Closure> entry(BuildDummyClosure(builder.Build(), {span}, {0x1}));
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
    scheduler_->machine(0)->PostRunnable(co);
    
    isolate_->Run();
}

static String *Dummy10() {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    STATE->heap()->TEST_set_trap(AllocationResult::OOM, 0);
    return *String::NewUtf8("Name ok");
}

TEST_F(CoroutineTest, OOMThrowPanic) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();
    builder.Add<kLdaConstPtr>(0);
    builder.Add<kCallNativeFunction>(0);
    builder.Add<kReturn>();

    Local<Closure> dummy(FunctionTemplate::New(Dummy10));
    Span32 span;
    span.ptr[0].any = *dummy;

    Local<Closure> entry(BuildDummyClosure(builder.Build(), {span}, {0x1}));
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
    scheduler_->machine(0)->PostRunnable(co);

    isolate_->Run();

    ASSERT_EQ(1, scheduler_->machine0()->uncaught_count());
}

TEST_F(CoroutineTest, NilPointerPanic) {
    constexpr int32_t kLocalVarBase = RoundUp(BytecodeStackFrame::kOffsetHeaderSize,
                                              kStackAligmentSize);
    
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();
    builder.Add<kLdaZero>();
    int32_t v1 = stack_offset(kLocalVarBase + 8);
    builder.Add<kStarPtr>(v1);
    builder.Add<kAssertNotNull>(v1);
    builder.Add<kLdaPropertyPtr>(v1, 0);
    builder.Add<kReturn>();
    
    Local<Closure> entry(BuildDummyClosure(builder.Build(), {}, {}));
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
    scheduler_->machine(0)->PostRunnable(co);

    isolate_->Run();
}

//static void Dummy11(String *message) {
//    HandleScope handle_scpoe(HandleScope::INITIALIZER);
//
//    Local<Throwable> e(Exception::New(Local<String>(message), Local<Exception>::Null()));
//    if (e.is_empty()) {
//        return;
//    }
//    Throwable::Throw(e);
//}
//
//TEST_F(CoroutineTest, ThrowUserException) {
//    HandleScope handle_scpoe(HandleScope::INITIALIZER);
//    BytecodeArrayBuilder builder(arena_);
//    builder.Add<kCheckStack>();
//
//    builder.Add<kLdaConstPtr>(2);
//    builder.Add<kStarPtr>(stack_offset(kStackSize + 8));
//
//    builder.Add<kLdaConstPtr>(0);
//    builder.Add<kCallNativeFunction>(kPointerSize);
//    builder.Add<kReturn>();
//
//    Local<Closure> dummy(FunctionTemplate::New(Dummy11));
//    Span32 span;
//    span.ptr[0].any = *dummy;
//    span.ptr[1].any = *String::NewUtf8("Test exception");
//
//    Local<Closure> entry(BuildDummyClosure(builder.Build(), {span}, {0x3}));
//    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
//    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
//    scheduler_->machine(0)->PostRunnable(co);
//
//    isolate_->Run();
//}

TEST_F(CoroutineTest, NewChannel) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();
    builder.Add<kLdaSmi32>(kType_int);
    builder.Add<kStar32>(stack_offset(kStackSize + 4));
    builder.Add<kLdaZero>();
    builder.Add<kStar32>(stack_offset(kStackSize + 8));
    builder.Add<kLdaConstPtr>(0);
    builder.Add<kCallNativeFunction>(8); // NewChannel
    builder.Add<kStarPtr>(stack_offset(kStackSize + 8));
    builder.Add<kLdaConstPtr>(2);
    builder.Add<kCallNativeFunction>(8); // ChannelClose
    builder.Add<kReturn>();

    Span32 span;
    span.ptr[0].any = *FunctionTemplate::New(&Runtime::NewChannel);
    span.ptr[1].any = *FunctionTemplate::New(&Runtime::ChannelClose);

    Local<Closure> entry(BuildDummyClosure(builder.Build(), {span}, {0x3}));
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
    scheduler_->machine(0)->PostRunnable(co);

    isolate_->Run();
}

static void Dummy13(int32_t value) {
    //printf("Recv: %d\n", value);
    dummy_result.i32_1 = value;
}

TEST_F(CoroutineTest, SendRecvChannel) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    
    Span32 span;
    span.ptr[0].any = *FunctionTemplate::New(&Runtime::ChannelSend32);
    span.ptr[1].any = *FunctionTemplate::New(&Runtime::ChannelRecv32);
    span.ptr[2].any = Machine::This()->NewChannel(kType_int, 0, 0);
    span.ptr[3].any = *FunctionTemplate::New(Dummy13);
    
    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();
    builder.Add<kLdaConstPtr>(4);
    builder.Add<kStarPtr>(stack_offset(kStackSize + 8));
    builder.Add<kLdaSmi32>(404);
    builder.Add<kStar32>(stack_offset(kStackSize + 12));
    builder.Add<kLdaConstPtr>(0);
    builder.Add<kCallNativeFunction>(12);
    builder.Add<kYield>(YIELD_PROPOSE);
    builder.Add<kReturn>();

    Local<Closure> entry(BuildDummyClosure(builder.Build(), {span}, {0x3}));
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
    scheduler_->machine(0)->PostRunnable(co);

    builder.Abandon();
    builder.Add<kCheckStack>();
    builder.Add<kLdaConstPtr>(4);
    builder.Add<kStarPtr>(stack_offset(kStackSize + 8));
    builder.Add<kLdaConstPtr>(2);
    builder.Add<kCallNativeFunction>(8);
    builder.Add<kYield>(YIELD_PROPOSE);
    builder.Add<kStar32>(stack_offset(kStackSize + 4));
    builder.Add<kLdaConstPtr>(6);
    builder.Add<kCallNativeFunction>(4);
    builder.Add<kReturn>();
    
    entry = BuildDummyClosure(builder.Build(), {span}, {0x3});
    co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
    scheduler_->machine(1)->PostRunnable(co);

    isolate_->Run();
    
    ASSERT_EQ(404, dummy_result.i32_1);
}

static void Dummy14(int32_t a, double b) {
    //printf("Recv: %x %d %f\n", a, a, b);
    dummy_result.i32_1 = a;
    dummy_result.f64_1 = b;
}

TEST_F(CoroutineTest, LoadArgumentToACC) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);

    Span32 span;
    span.ptr[0].any = *FunctionTemplate::New(Dummy14);

    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();
    builder.Add<kMove32>(stack_offset(kStackSize + 4), parameter_offset(16 - 4));
    builder.Add<kMove64>(stack_offset(kStackSize + 12), parameter_offset(16 - 12));
    builder.Add<kLdaConstPtr>(0);
    builder.Add<kCallNativeFunction>(12);
    builder.Add<kReturn>();

    uint8_t mock_data[16];
    ::memset(mock_data, 0xcc, sizeof(mock_data));

    Local<Closure> entry(BuildDummyClosure(builder.Build(), {span}, {0x3}));
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);

    *reinterpret_cast<int32_t *>(&mock_data[12]) = 999810; // arg0
    *reinterpret_cast<double *>(&mock_data[4]) = 3.1415f; // arg1
    co->CopyArgv(mock_data, sizeof(mock_data));

    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
    scheduler_->machine(0)->PostRunnable(co);

    isolate_->Run();

    ASSERT_EQ(999810, dummy_result.i32_1);
    ASSERT_NEAR(3.1415f, dummy_result.f64_1, 0.0001);
}

static void Dummy15(Any *any) {
    DummyClass *d = static_cast<DummyClass *>(any);
    dummy_result.any_1 = d;
}

TEST_F(CoroutineTest, NewObject) {
    const Class *clazz = RegisterDummyClass();

    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    
    Span32 span;
    span.ptr[0].any = *FunctionTemplate::New(Dummy15);
    span.ptr[1].pv = const_cast<Class *>(clazz);
    
    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();
    builder.Add<kNewObject>(Heap::kOld/*flags*/, 2/*const_offset*/);
    builder.Add<kStarPtr>(stack_offset(kStackSize + 8));
    builder.Add<kLdaConstPtr>(0);
    builder.Add<kCallNativeFunction>(8);
    builder.Add<kReturn>();
    
    Local<Closure> entry(BuildDummyClosure(builder.Build(), {span}, {0x1}));
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    
    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
    scheduler_->machine(0)->PostRunnable(co);

    isolate_->Run();
    
    ASSERT_NE(nullptr, dummy_result.any_1);
    DummyClass *d = static_cast<DummyClass *>(dummy_result.any_1);
    ASSERT_EQ(0, d->i8_1_);
    ASSERT_EQ(0, d->i8_2_);
    ASSERT_EQ(0, d->i16_1_);
    ASSERT_EQ(0, d->i32_1_);
    ASSERT_EQ(0, d->i64_1_);
}

static void Dummy16(int8_t a, int16_t b, int32_t c, int64_t d) {
    dummy_result.i8_1 = a;
    dummy_result.i16_1 = b;
    dummy_result.i32_1 = c;
    dummy_result.i64_1 = d;
}

TEST_F(CoroutineTest, LoadPropertyToACC) {
    constexpr int32_t kLocalVarBase = RoundUp(BytecodeStackFrame::kOffsetHeaderSize,
                                              kStackAligmentSize);
    const Class *clazz = RegisterDummyClass();

    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    Local<DummyClass> obj(static_cast<DummyClass *>(Machine::This()->NewObject(clazz, 0)));
    
    obj->i8_1_ = 112;
    obj->i8_2_ = 119;
    obj->i16_1_ = 226;
    obj->i32_1_ = 321;
    obj->i64_1_ = 641;
    
    Span32 span;
    span.ptr[0].any = *FunctionTemplate::New(Dummy16);
    span.ptr[1].any = *obj;

    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();
    builder.Add<kLdaConstPtr>(2);
    auto v1 = stack_offset(kLocalVarBase + 8);
    builder.Add<kStarPtr>(v1);
    builder.Add<kLdaProperty8>(v1, clazz->field(0)->offset());
    builder.Add<kStar32>(stack_offset(kStackSize + 4));
    builder.Add<kLdaProperty16>(v1, clazz->field(2)->offset());
    builder.Add<kStar32>(stack_offset(kStackSize + 8));
    builder.Add<kLdaProperty32>(v1, clazz->field(3)->offset());
    builder.Add<kStar32>(stack_offset(kStackSize + 12));
    builder.Add<kLdaProperty64>(v1, clazz->field(4)->offset());
    builder.Add<kStar64>(stack_offset(kStackSize + 20));
    builder.Add<kLdaConstPtr>(0);
    builder.Add<kCallNativeFunction>(20);
    builder.Add<kReturn>();
    
    Local<Closure> entry(BuildDummyClosure(builder.Build(), {span}, {0x1}));
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    
    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
    scheduler_->machine(0)->PostRunnable(co);

    isolate_->Run();
    
    ASSERT_EQ(112, dummy_result.i8_1);
    ASSERT_EQ(226, dummy_result.i16_1);
    ASSERT_EQ(321, dummy_result.i32_1);
    ASSERT_EQ(641, dummy_result.i64_1);
}

static void Dummy17(float a, double b, Any *c) {
    dummy_result.f32_1 = a;
    dummy_result.f64_1 = b;
    dummy_result.any_1 = c;
}

TEST_F(CoroutineTest, LoadPropertyToACC2) {
    constexpr int32_t kLocalVarBase = RoundUp(BytecodeStackFrame::kOffsetHeaderSize,
                                              kStackAligmentSize);
    const Class *clazz = RegisterDummyClass();

    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    Local<DummyClass> obj(static_cast<DummyClass *>(Machine::This()->NewObject(clazz, 0)));

    obj->i32_1_ = 321;
    obj->i64_1_ = 641;
    obj->f32_1_ = 2.718281828f;
    obj->f64_1_ = 22.0f/7.0f;
    Local<String> handle(String::NewUtf8("22.0f/7.0f"));
    obj->any_1_ = *handle;

    Span32 span;
    span.ptr[0].any = *FunctionTemplate::New(Dummy17);
    span.ptr[1].any = *obj;
    
    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();
    builder.Add<kLdaConstPtr>(2);
    auto v1 = stack_offset(kLocalVarBase + 8);
    builder.Add<kStarPtr>(v1);
    builder.Add<kLdaPropertyf32>(v1, clazz->field(5)->offset());
    builder.Add<kStaf32>(stack_offset(kStackSize + 4));
    builder.Add<kLdaPropertyf64>(v1, clazz->field(6)->offset());
    builder.Add<kStaf64>(stack_offset(kStackSize + 12));
    builder.Add<kLdaPropertyPtr>(v1, clazz->field(7)->offset());
    builder.Add<kStarPtr>(stack_offset(kStackSize + 20));
    builder.Add<kLdaConstPtr>(0);
    builder.Add<kCallNativeFunction>(20);
    builder.Add<kReturn>();
    
    Local<Closure> entry(BuildDummyClosure(builder.Build(), {span}, {0x1}));
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    
    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
    scheduler_->machine(0)->PostRunnable(co);

    isolate_->Run();
    
    ASSERT_NEAR(2.718281828f, dummy_result.f32_1, 0.00000001);
    ASSERT_NEAR(3.142857f, dummy_result.f64_1, 0.000001);
    ASSERT_EQ(*handle, dummy_result.any_1);
}

TEST_F(CoroutineTest, StorePropertyFromACC) {
    constexpr int32_t kLocalVarBase = RoundUp(BytecodeStackFrame::kOffsetHeaderSize,
                                              kStackAligmentSize);
    const Class *clazz = RegisterDummyClass();

    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    Local<DummyClass> obj(static_cast<DummyClass *>(Machine::This()->NewObject(clazz, 0)));
    
    Span32 span;
    span.ptr[0].any = *obj;
    
    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();
    builder.Add<kLdaConstPtr>(0);
    auto v1 = stack_offset(kLocalVarBase + 8);
    builder.Add<kStarPtr>(v1);
    builder.Add<kLdaSmi32>(81);
    builder.Add<kStaProperty8>(v1, clazz->field(0)->offset());
    builder.Add<kLdaSmi32>(82);
    builder.Add<kStaProperty8>(v1, clazz->field(1)->offset());
    builder.Add<kLdaSmi32>(161);
    builder.Add<kStaProperty16>(v1, clazz->field(2)->offset());
    builder.Add<kLdaSmi32>(321);
    builder.Add<kStaProperty32>(v1, clazz->field(3)->offset());
    builder.Add<kLdaSmi32>(641);
    builder.Add<kStaProperty64>(v1, clazz->field(4)->offset());
    builder.Add<kReturn>();
    
    Local<Closure> entry(BuildDummyClosure(builder.Build(), {span}, {0x1}));
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    
    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
    scheduler_->machine(0)->PostRunnable(co);

    isolate_->Run();
    
    ASSERT_EQ(81, obj->i8_1_);
    ASSERT_EQ(82, obj->i8_2_);
    ASSERT_EQ(161, obj->i16_1_);
    ASSERT_EQ(321, obj->i32_1_);
    ASSERT_EQ(641, obj->i64_1_);
}

static void Dummy18(int32_t a, int64_t b, float c, double d) {
    //printf("%d %lld %f %f\n", a, b, c, d);
    dummy_result.i32_1 = a;
    dummy_result.i64_1 = b;
    dummy_result.f32_1 = c;
    dummy_result.f64_1 = d;
}

TEST_F(CoroutineTest, LoadCapturedVarToACC) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    
    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();
    
    builder.Add<kLdaCaptured32>(0);
    builder.Add<kStar32>(stack_offset(kStackSize + 4));

    builder.Add<kLdaCaptured64>(1);
    builder.Add<kStar64>(stack_offset(kStackSize + 12));
    
    builder.Add<kLdaCapturedf32>(2);
    builder.Add<kStaf32>(stack_offset(kStackSize + 16));
    
    builder.Add<kLdaCapturedf64>(3);
    builder.Add<kStaf64>(stack_offset(kStackSize + 24));
    
    builder.Add<kLdaConstPtr>(0);
    builder.Add<kCallNativeFunction>(24);
    builder.Add<kReturn>();
    
    Span32 span;
    span.ptr[0].any = *FunctionTemplate::New(Dummy18);
    
    Local<Closure> entry(BuildDummyClosure(builder.Build(), {span}, {0x1}));
    constexpr int32_t i = 100102;
    entry->set_captured_var_no_barrier(0, *NewCapturedValue(i));
    constexpr int64_t l = 640001;
    entry->set_captured_var_no_barrier(1, *NewCapturedValue(l));
    constexpr float   f = 0.1754321;
    entry->set_captured_var_no_barrier(2, *NewCapturedValue(f));
    constexpr double  d = 22/7.0;
    entry->set_captured_var_no_barrier(3, *NewCapturedValue(d));
    
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
    scheduler_->machine(0)->PostRunnable(co);

    isolate_->Run();
    
    ASSERT_EQ(i, dummy_result.i32_1);
    ASSERT_EQ(l, dummy_result.i64_1);
    ASSERT_EQ(f, dummy_result.f32_1);
    ASSERT_EQ(d, dummy_result.f64_1);
}

TEST_F(CoroutineTest, StoreCapturedVarFromACC) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    
    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();

    builder.Add<kLdaConst32>(0);
    builder.Add<kStaCaptured32>(0);

    builder.Add<kLdaConstf32>(1);
    builder.Add<kStaCapturedf32>(2);

    builder.Add<kLdaConst64>(2);
    builder.Add<kStaCaptured64>(1);

    builder.Add<kLdaConstf64>(4);
    builder.Add<kStaCapturedf64>(3);

    builder.Add<kReturn>();

    //builder.Print(new base::StdFilePrinter(stdout), true);

    Span32 span;
    span.v32[0].i32 = 132001;
    span.v32[1].f32 = 0.1754321;
    span.v64[1].i64 = 164001;
    span.v64[2].f64 = 22/7.0;
    
    Local<Closure> entry(BuildDummyClosure(builder.Build(), {span}, {0}));
    entry->set_captured_var_no_barrier(0, *NewCapturedValue(0)); // i32
    entry->set_captured_var_no_barrier(1, *NewCapturedValue(0)); // i64
    entry->set_captured_var_no_barrier(2, *NewCapturedValue(0)); // f32
    entry->set_captured_var_no_barrier(3, *NewCapturedValue(0)); // f64
    
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
    scheduler_->machine(0)->PostRunnable(co);

    isolate_->Run();
    
    ASSERT_EQ(span.v32[0].i32, entry->captured_var(0)->unsafe_typed_value<int32_t>());
    ASSERT_EQ(span.v64[1].i64, entry->captured_var(1)->unsafe_typed_value<int64_t>());
    ASSERT_EQ(span.v32[1].f32, entry->captured_var(2)->unsafe_typed_value<float>());
    ASSERT_EQ(span.v64[2].f64, entry->captured_var(3)->unsafe_typed_value<double>());
}

static void Dummy19(int32_t a, int64_t b, float c, double d) {
    dummy_result.i32_1 = a;
    dummy_result.i64_1 = b;
    dummy_result.f32_1 = c;
    dummy_result.f64_1 = d;
}

TEST_F(CoroutineTest, LoadGlobalSpace) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    
    GlobalSpaceBuilder global;
    int l1 = global.AppendI32(100);
    int l2 = global.AppendI64(996);
    int l3 = global.AppendF32(9.99);
    int l4 = global.AppendF64(3.14);
    isolate_->SetGlobalSpace(global.TakeSpans(), global.TakeBitmap(), global.capacity(),
                             global.length());

    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();
    builder.Add<kLdaGlobal32>(global_offset(l1));
    builder.Add<kStar32>(stack_offset(kStackSize + 4));
    builder.Add<kLdaGlobal64>(global_offset(l2));
    builder.Add<kStar64>(stack_offset(kStackSize + 12));
    builder.Add<kLdaGlobalf32>(global_offset(l3));
    builder.Add<kStaf32>(stack_offset(kStackSize + 16));
    builder.Add<kLdaGlobalf64>(global_offset(l4));
    builder.Add<kStaf64>(stack_offset(kStackSize + 24));
    builder.Add<kLdaConstPtr>(0);
    builder.Add<kCallNativeFunction>(24);
    builder.Add<kReturn>();

    Span32 span;
    span.ptr[0].any = *FunctionTemplate::New(Dummy19);

    Local<Closure> entry(BuildDummyClosure(builder.Build(), {span}, {0x1}));
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
    scheduler_->machine(0)->PostRunnable(co);

    isolate_->Run();

    ASSERT_EQ(100, dummy_result.i32_1);
    ASSERT_EQ(996, dummy_result.i64_1);
    ASSERT_NEAR(9.99, dummy_result.f32_1, 0.001);
    ASSERT_NEAR(3.14, dummy_result.f64_1, 0.001);
}

TEST_F(CoroutineTest, StoreGlobalSpace) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);

    GlobalSpaceBuilder global;
    int l1 = global.AppendI32(0);
    int l2 = global.AppendI64(0);
    int l3 = global.AppendF32(0);
    int l4 = global.AppendAny(nullptr);
    isolate_->SetGlobalSpace(global.TakeSpans(), global.TakeBitmap(), global.capacity(),
                             global.length());
    
    ConstantPoolBuilder pool;
    int k1 = pool.FindOrInsertI32(199);
    int k2 = pool.FindOrInsertI64(2789999);
    int k3 = pool.FindOrInsertF32(3.14);
    int k4 = pool.FindOrInsertString(*String::NewUtf8("Hello"));
    
    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();
    builder.Add<kLdaConst32>(const_offset(k1));
    builder.Add<kStaGlobal32>(global_offset(l1));
    builder.Add<kLdaConst64>(const_offset(k2));
    builder.Add<kStaGlobal64>(global_offset(l2));
    builder.Add<kLdaConstf32>(const_offset(k3));
    builder.Add<kStaGlobalf32>(global_offset(l3));
    builder.Add<kLdaConstPtr>(const_offset(k4));
    builder.Add<kStaGlobalPtr>(global_offset(l4));
    builder.Add<kReturn>();
    
    Local<Closure> entry(BuildDummyClosure(builder.Build(), pool.ToSpanVector(),
                                           pool.ToBitmapVector()));
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    co->SwitchState(Coroutine::kDead, Coroutine::kRunnable);
    scheduler_->machine(0)->PostRunnable(co);

    isolate_->Run();
    
    ASSERT_EQ(199, *isolate_->global_offset<int32_t>(l1));
    ASSERT_EQ(2789999, *isolate_->global_offset<int64_t>(l2));
    ASSERT_NEAR(3.14, *isolate_->global_offset<float>(l3), 0.001);
    Local<String> s(*isolate_->global_offset<String *>(l4));
    ASSERT_TRUE(s.is_not_empty());
    ASSERT_TRUE(s.is_value_not_null());
    ASSERT_STREQ("Hello", s->data());
}

} // namespace lang

} // namespace mai
