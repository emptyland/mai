#include "test/nyaa-test.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/object-factory.h"
#include "nyaa/function.h"
#include "asm/x64/asm-x64.h"
#include "mai-lang/nyaa.h"
#include <limits>

namespace mai {
    
namespace nyaa {

class NyaaValuesTest : public test::NyaaTest {
public:
    void SetUp() override {
        NyaaTest::SetUp();
        N_ = new Nyaa(Nyaa::Options(), isolate_);
        core_ = N_->core();
        factory_ = N_->core()->factory();
    }
    
    void TearDown() override {
        delete N_;
        NyaaTest::TearDown();
    }
    
    Nyaa *N_ = nullptr;
    NyaaCore *core_ = nullptr;
    ObjectFactory *factory_ = nullptr;
};

TEST_F(NyaaValuesTest, Sanity) {
    HandleScope scope(N_);
    Handle<NyString> h(factory_->NewString("abcd", 4, false));

    ASSERT_NE(nullptr, *h);
    ASSERT_STREQ("abcd", h->bytes());
    
    Local<String> s = Local<String>::New(reinterpret_cast<String *>(*h));

    ASSERT_NE(nullptr, *s);
    ASSERT_STREQ("abcd", s->Bytes());
}
    
TEST_F(NyaaValuesTest, Table) {
    HandleScope scope(N_);
    Handle<NyTable> h(factory_->NewTable(12, 0));
    
    h = h->RawPut(NyInt32::New(1), factory_->NewString("aaaa"), N_->core());
    h = h->RawPut(NyInt32::New(2), factory_->NewString("bbbb"), N_->core());
    h = h->RawPut(NyInt32::New(3), factory_->NewString("cccc"), N_->core());
    
    auto v = Local<NyString>::New(h->RawGet(NyInt32::New(2), N_->core()));
    ASSERT_TRUE(v.is_not_null());
    ASSERT_TRUE(v.is_not_empty());
    ASSERT_STREQ("bbbb", v->bytes());
}
    
TEST_F(NyaaValuesTest, TablePut) {
    HandleScope scope(N_);
    Handle<NyTable> h(factory_->NewTable(8, 0));
    
    NyTable *const kOrigin = *h;
    Object *value = factory_->NewString("cccc");
    h = h->RawPut(NyInt32::New(0), value, N_->core());
    h = h->RawPut(NyInt32::New(1), value, N_->core());
    ASSERT_EQ(kOrigin, *h);
    
    Object *v = h->RawGet(NyInt32::New(0), N_->core());
    ASSERT_EQ(v, value);
    v = h->RawGet(NyInt32::New(1), N_->core());
    ASSERT_EQ(v, value);
}
    
TEST_F(NyaaValuesTest, TableAutoRehash) {
    HandleScope scope(N_);
    Handle<NyTable> h(factory_->NewTable(8, 0));
    
    NyTable *const kOrigin = *h;
    Object *value = factory_->NewString("cccc");
    for (int i = 0; i < 100; ++i) {
        h = h->RawPut(NyInt32::New(i), value, N_->core());
    }
    ASSERT_NE(kOrigin, *h);
    
    for (int i = 0; i < 100; ++i) {
        Object *v = h->RawGet(NyInt32::New(i), N_->core());
        ASSERT_EQ(v, value) << i;
    }
}
    
TEST_F(NyaaValuesTest, Smi) {
    Object *val = NySmi::New(-1);
    ASSERT_EQ(-1, val->ToSmi());
    
    val = NySmi::New(NySmi::kMaxValue);
    ASSERT_EQ(NySmi::kMaxValue, val->ToSmi());
    
    val = NySmi::New(NySmi::kMinValue);
    ASSERT_EQ(NySmi::kMinValue, val->ToSmi());
}

#if defined(NYAA_USE_POINTER_COLOR)

TEST_F(NyaaValuesTest, NyObjectColor) {
    NyString *s = factory_->NewString("cccc");
    NyMap *mt = s->GetMetatable();
    ASSERT_EQ(mt, N_->core()->kmt_pool()->kString);
    
    s->SetColor(kColorWhite);
    ASSERT_EQ(kColorWhite, s->GetColor());
    
    ASSERT_EQ(mt, s->GetMetatable());
    
    s->SetMetatable(N_->core()->kmt_pool()->kString, N_->core());
    ASSERT_EQ(kColorWhite, s->GetColor());
    ASSERT_EQ(s->GetMetatable(), N_->core()->kmt_pool()->kString);
}

#endif // defined(NYAA_USE_POINTER_COLOR)

TEST_F(NyaaValuesTest, ObjectDiv) {
    Object *rv = Object::Div(NySmi::New(1), NySmi::New(1), core_);
    ASSERT_EQ(1, rv->ToSmi());
    
    rv = Object::Div(NySmi::New(NySmi::kMinValue), NySmi::New(2), core_);
    ASSERT_EQ(-1152921504606846976, rv->ToSmi());
}

#if defined(MAI_ARCH_X64)
    
#define __ masm.

TEST_F(NyaaValuesTest, CodeObject) {
    using namespace ::mai::x64;
    
    Assembler masm;
    
    __ movl(rax, kRegArgv[0]);
    __ addl(rax, kRegArgv[1]);
    __ ret(0);
    
    auto code = factory_->NewCode(NyCode::kOptimizedFunction, nullptr,
                                  reinterpret_cast<const uint8_t *>(masm.buf().data()),
                                  masm.buf().size());
    CallStub<int (int, int)> stub(code);
    EXPECT_EQ(200, stub.entry_fn()(199, 1));
}
    
#endif

} // namespace nyaa
    
} // namespace mai
