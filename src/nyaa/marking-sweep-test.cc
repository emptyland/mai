#include "nyaa/marking-sweep.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/object-factory.h"
#include "nyaa/string-pool.h"
#include "nyaa/heap.h"
#include "test/nyaa-test.h"
#include "mai-lang/nyaa.h"

namespace mai {
    
namespace nyaa {

class NyaaMarkingSweepTest : public test::NyaaTest {
public:
    NyaaMarkingSweepTest()
        : scope_(Nyaa::Options{}, isolate_)
        , N_(&scope_)
        , core_(scope_.core())
        , factory_(core_->factory()) {
    }
    
    Nyaa scope_;
    Nyaa *N_ = nullptr;
    NyaaCore *core_ = nullptr;
    ObjectFactory *factory_ = nullptr;
};

TEST_F(NyaaMarkingSweepTest, Sanity) {
    factory_->NewMap(128, 0, 0, false, true);

    MarkingSweep gc(core_, core_->heap());
    gc.Run();
    ASSERT_LT(0, gc.collected_bytes());
    ASSERT_EQ(2, gc.collected_objs());
}
    
TEST_F(NyaaMarkingSweepTest, RunAgain) {
    factory_->NewMap(128, 0, 0, false, true);
    {
        MarkingSweep gc(core_, core_->heap());
        gc.Run();
        ASSERT_LT(0, gc.collected_bytes());
        ASSERT_EQ(2, gc.collected_objs());
    }

    
    factory_->NewMap(128, 0, 0, false, true);
    {
        MarkingSweep gc(core_, core_->heap());
        gc.Run();
        ASSERT_LT(0, gc.collected_bytes());
        ASSERT_EQ(2, gc.collected_objs());
    }
}
    
TEST_F(NyaaMarkingSweepTest, ClearRememberSet) {
    HandleScope scope(isolate_);
    
    Handle<NyMap> h1 = factory_->NewMap(128, 0, 0, false, true);
    h1->RawPut(factory_->NewString("ok"), NyInt32::New(100), core_);
    h1->RawPut(factory_->NewString("ye"), NyInt32::New(200), core_);
    
    auto remembers = core_->heap()->GetRememberHosts(h1->generic());
    ASSERT_EQ(2, remembers.size());
    
    h1 = nullptr;
    {
        MarkingSweep gc(core_, core_->heap());
        gc.Run();
        ASSERT_LT(0, gc.collected_bytes());
        ASSERT_EQ(2, gc.collected_objs());
    }
}

}

}
