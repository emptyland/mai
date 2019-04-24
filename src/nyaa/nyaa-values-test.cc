#include "test/nyaa-test.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/object-factory.h"
#include "mai-lang/nyaa.h"

namespace mai {
    
namespace nyaa {

class NyaaValuesTest : public test::NyaaTest {
public:
    void SetUp() override {
        NyaaTest::SetUp();
        N_ = new Nyaa(Nyaa::Options(), isolate_);
        factory_ = N_->core()->factory();
    }
    
    void TearDown() override {
        delete N_;
        NyaaTest::TearDown();
    }
    
    Nyaa *N_ = nullptr;
    ObjectFactory *factory_ = nullptr;
};

TEST_F(NyaaValuesTest, Sanity) {
    HandleScope scope(isolate_);
    Handle<NyString> h(factory_->NewString("abcd", 4, false));

    ASSERT_NE(nullptr, *h);
    ASSERT_STREQ("abcd", h->bytes());
    
    Local<String> s = Local<String>::New(h);

    ASSERT_NE(nullptr, *s);
    ASSERT_STREQ("abcd", s->Bytes());
}
    
TEST_F(NyaaValuesTest, Table) {
    HandleScope scope(isolate_);
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
    HandleScope scope(isolate_);
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
    HandleScope scope(isolate_);
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
    
    s->SetMetatable(N_->core()->kmt_pool()->kInt, N_->core());
    ASSERT_EQ(kColorWhite, s->GetColor());
    ASSERT_EQ(s->GetMetatable(), N_->core()->kmt_pool()->kInt);
}

#endif // defined(NYAA_USE_POINTER_COLOR)
    
} // namespace nyaa
    
} // namespace mai
