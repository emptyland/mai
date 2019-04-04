#include "nyaa/marking-sweeper.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/object-factory.h"
#include "nyaa/string-pool.h"
#include "nyaa/heap.h"
#include "test/nyaa-test.h"
#include "mai-lang/nyaa.h"

namespace mai {
    
namespace nyaa {

class NyaaMarkSweeperTest : public test::NyaaTest {
public:
    NyaaMarkSweeperTest()
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

TEST_F(NyaaMarkSweeperTest, Sanity) {
    factory_->NewMap(128, 0, 0, false, true);

    MarkingSweeper gc(core_, core_->heap());
    gc.Run();
    ASSERT_LT(0, gc.collected_bytes());
    ASSERT_EQ(2, gc.collected_objs());
}

}

}
