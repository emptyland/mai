#include "nyaa/scavenger.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/object-factory.h"
#include "nyaa/heap.h"
#include "test/nyaa-test.h"
#include "mai-lang/nyaa.h"

namespace mai {
    
namespace nyaa {

class NyaaScavengerTest : public test::NyaaTest {
public:
    NyaaScavengerTest()
        : scope_(Nyaa::Options{}, isolate_)
        , N_(&scope_)
        , core_(scope_.core())
        , factory_(core_->factory()) {
    }
    
    void SetUp() override {
        NyaaTest::SetUp();
    }
    
    void TearDown() override {
        NyaaTest::TearDown();
    }
    
    Nyaa scope_;
    Nyaa *N_ = nullptr;
    NyaaCore *core_ = nullptr;
    ObjectFactory *factory_ = nullptr;
};
    
TEST_F(NyaaScavengerTest, Sanity) {
    Scavenger gc(core_, core_->heap());
    
    auto oldone = core_->kmt_pool()->kString;
    
    gc.Run();
    ASSERT_GT(core_->heap()->from_semi_area_remain_rate(), 0);
    ASSERT_GT(core_->heap()->major_gc_cost(), 0);

    auto s = core_->factory()->NewString("ok");
    ASSERT_EQ(core_->kmt_pool()->kString, s->GetMetatable());
    ASSERT_EQ(oldone, s->GetMetatable());
}

TEST_F(NyaaScavengerTest, MoveNewSpace) {
    Scavenger gc(core_, core_->heap());
    
    HandleScope scope(isolate_);
    
    Handle<NyString> s1 = factory_->NewString("ok");
    NyString *p1 = *s1;
    Handle<NyString> s2 = factory_->NewString("yes");
    NyString *p2 = *s2;

    gc.Run();

    Handle<NyString> s3 = factory_->NewString("no~no~no~");
    ASSERT_NE(p1, *s1);
    ASSERT_STREQ("ok", s1->bytes());
    ASSERT_NE(p2, *s2);
    ASSERT_STREQ("yes", s2->bytes());
    ASSERT_STREQ("no~no~no~", s3->bytes());
}
    
} // namespace nyaa
    
} // namespace mai
