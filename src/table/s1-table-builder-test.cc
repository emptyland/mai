#include "table/s1-table-builder.h"
#include "test/table-test.h"

namespace mai {
    
namespace table {
    
class S1TableBuilderTest : public test::TableTest {
public:
    
    TableBuilderFactory default_factory_ = [](const core::InternalKeyComparator *ikcmp,
                                              WritableFile *file) {
        return new S1TableBuilder(ikcmp, file, 17, 512);
    };

    static const char *tmp_dirs[];
}; // class S1TableBuilder
    
/*static*/ const char *S1TableBuilderTest::tmp_dirs[] = {
    "tests/14-s1-table-builder-sanity.tmp",
    nullptr,
};
    
TEST_F(S1TableBuilderTest, Sanity) {
    BuildTable({
        "k1", "v5", "5",
        "k2", "v4", "4",
        "k3", "v3", "3",
        "k4", "v2", "2",
        "k5", "v1", "1",
    }, tmp_dirs[0], default_factory_);
}
    
} // namespace table
    
} // namespace mai
