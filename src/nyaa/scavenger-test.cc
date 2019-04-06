#include "nyaa/scavenger.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/object-factory.h"
#include "nyaa/string-pool.h"
#include "nyaa/heap.h"
#include "nyaa/spaces.h"
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
    
//    void SetUp() override {
//        NyaaTest::SetUp();
//    }
//
//    void TearDown() override {
//        NyaaTest::TearDown();
//    }
    
    Nyaa scope_;
    Nyaa *N_ = nullptr;
    NyaaCore *core_ = nullptr;
    ObjectFactory *factory_ = nullptr;
};
    
TEST_F(NyaaScavengerTest, Sanity) {
    Scavenger gc(core_, core_->heap());
    
    auto oldone = core_->kmt_pool()->kString;
    
    gc.Run();
    ASSERT_GT(core_->heap()->new_space_->latest_remaining_size(), 0);
    //printf("%lu\n", core_->heap()->new_space_->latest_remaining_size());
    ASSERT_GT(gc.time_cost(), 0);

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
    ASSERT_EQ(kNewSpace, core_->heap()->Contains(*s1));
    ASSERT_EQ(kNewSpace, core_->heap()->Contains(*s2));
    ASSERT_EQ(kNewSpace, core_->heap()->Contains(*s3));
}

TEST_F(NyaaScavengerTest, ForceUpgrade) {
    Scavenger gc(core_, core_->heap());
    
    HandleScope scope(isolate_);
    
    Handle<NyString> s1 = factory_->NewString("ok");
    NyString *p1 = *s1;
    Handle<NyString> s2 = factory_->NewString("yes");
    NyString *p2 = *s2;
    
    gc.set_force_upgrade(true);
    gc.Run();
    
    Handle<NyString> s3 = factory_->NewString("no~no~no~");
    ASSERT_NE(p1, *s1);
    ASSERT_STREQ("ok", s1->bytes());
    ASSERT_NE(p2, *s2);
    ASSERT_STREQ("yes", s2->bytes());
    ASSERT_STREQ("no~no~no~", s3->bytes());
    ASSERT_EQ(kOldSpace, core_->heap()->Contains(*s1));
    ASSERT_EQ(kOldSpace, core_->heap()->Contains(*s2));
    ASSERT_EQ(kNewSpace, core_->heap()->Contains(*s3));
}
    
TEST_F(NyaaScavengerTest, KzPoolSweep) {
    Scavenger gc(core_, core_->heap());

    HandleScope scope(isolate_);
    auto p1 = factory_->NewString("ok");
    ASSERT_EQ(p1, factory_->NewString("ok"));
    auto p2 = factory_->NewString("doom");
    ASSERT_EQ(p2, factory_->NewString("doom"));

    gc.Run();

    p1 = core_->kz_pool()->GetOrNull("ok");
    ASSERT_EQ(nullptr, p1);
    p2 = core_->kz_pool()->GetOrNull("doom");
    ASSERT_EQ(nullptr, p2);
}
    
} // namespace nyaa
    
} // namespace mai
