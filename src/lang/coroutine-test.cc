#include "lang/coroutine.h"
#include "lang/scheduler.h"
#include "lang/metadata-space.h"
#include "lang/value-inl.h"
#include "lang/bytecode-array-builder.h"
#include "lang/stack-frame.h"
#include "test/isolate-initializer.h"
#include "base/arenas.h"
#include "gtest/gtest.h"
#include <thread>

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

    Handle<Closure> BuildDummyClosure(const std::vector<BytecodeInstruction> &instrs,
                                      const std::vector<Span32> &const_pool = {},
                                      const std::vector<uint32_t> &const_pool_bitmap = {}) {
        Function *func = BuildDummyFunction(instrs, const_pool, const_pool_bitmap);
        Closure *closure = Machine::Get()->NewClosure(func, 0, Closure::kMaiFunction);
        return Handle<Closure>(closure);
    }

    Function *BuildDummyFunction(const std::vector<BytecodeInstruction> &instrs,
                                 const std::vector<Span32> &const_pool,
                                 const std::vector<uint32_t> &const_pool_bitmap) {
        BytecodeArray *bc_array = metadata_->NewBytecodeArray(instrs);
        return FunctionBuilder("test.dummy")
            .prototype({}, false, kType_void)
            .stack_size(RoundUp(BytecodeStackFrame::kOffsetHeaderSize + 32, kStackAligmentSize))
            .stack_bitmap({0})
            .const_pool(const_pool)
            .const_pool_bitmap(const_pool_bitmap)
            .bytecode(bc_array)
        .Build(metadata_);
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
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    Handle<Closure> entry(FunctionTemplate::New(Dummy1));
    
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
    Handle<Closure> entry(FunctionTemplate::New(Dummy3));
    
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    ASSERT_NE(nullptr, co);
    ASSERT_EQ(*entry, co->entry());
    
    uint8_t mock_data[32] = {0};
    ::memset(mock_data, 0xcc, sizeof(mock_data));
    
    CallStub<intptr_t(Coroutine *)> call_stub(metadata_->function_template_dummy_code());
    Handle<String> handle(String::NewUtf8("Hello"));
    
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
    
    Handle<String> handle(String::NewUtf8("Hello"));
    dummy_result.any_1 = *handle;
}

TEST_F(CoroutineTest, CallNativeFunctionDummy4) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    Handle<Closure> entry(FunctionTemplate::New(Dummy4));
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
    Handle<String> handle(static_cast<String *>(dummy_result.any_1));
    ASSERT_STREQ("Hello", handle->data());
}

TEST_F(CoroutineTest, RunBytecodeFunctionSanity) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    
    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();
    builder.Add<kReturn>();
    
    Handle<Closure> entry(BuildDummyClosure(builder.Build()));
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    CallStub<intptr_t(Coroutine *)> trampoline(metadata_->trampoline_code());
    trampoline.entry()(co);
    ASSERT_EQ(0, co->yield());
    ASSERT_EQ(1, co->reentrant());
}

static void Dummy5() {
    printf("Hello, World\n");
}

TEST_F(CoroutineTest, BytecodeCallNativeFunction) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    
    int32_t local_var_base = RoundUp(BytecodeStackFrame::kOffsetHeaderSize, kStackAligmentSize);
    
    BytecodeArrayBuilder builder(arena_);
    builder.Add<kCheckStack>();
    builder.Add<kLdaConstPtr>(0);
    builder.Add<kStarPtr>(local_var_base);
    builder.Add<kCallNativeFunction>(local_var_base, 0);
    builder.Add<kReturn>();
    
    Handle<Closure> dummy(FunctionTemplate::New(Dummy5));
    Span32 span;
    span.ptr[0].any = *dummy;
    
    Handle<Closure> entry(BuildDummyClosure(builder.Build(), {span}, {0x1}));
    Coroutine *co = scheduler_->NewCoroutine(*entry, true/*co0*/);
    CallStub<intptr_t(Coroutine *)> trampoline(metadata_->trampoline_code());
    trampoline.entry()(co);
    ASSERT_EQ(0, co->yield());
    ASSERT_EQ(1, co->reentrant());
}

} // namespace lang

} // namespace mai
