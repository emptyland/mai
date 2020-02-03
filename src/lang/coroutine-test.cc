#include "lang/coroutine.h"
#include "lang/scheduler.h"
#include "lang/metadata-space.h"
#include "lang/value-inl.h"
#include "test/isolate-initializer.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class CoroutineTest : public test::IsolateInitializer {
public:
    // Dummy
    void SetUp() override {
        test::IsolateInitializer::SetUp();
        scheduler_ = isolate_->scheduler();
        metadata_ = isolate_->metadata_space();
    }
    
    void TearDown() override {
        scheduler_ = nullptr;
        metadata_ = nullptr;
        test::IsolateInitializer::TearDown();
    }

    Scheduler *scheduler_ = nullptr;
    MetadataSpace *metadata_ = nullptr;
};

struct DummyResult {
    int8_t  i8_1,  i8_2,  i8_3;
    int16_t i16_1, i16_2, i16_3;
    int32_t i32_1, i32_2, i32_3;
    int64_t i64_1, i64_2, i64_3;
    float   f32_1, f32_2, f32_3;
    double  f64_1, f64_2, f64_3;
};

static DummyResult dummy_result;

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

} // namespace lang

} // namespace mai
