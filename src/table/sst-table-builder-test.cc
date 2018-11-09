#include "table/sst-table-builder.h"
#include "table/sst-table-builder.h"
#include "test/table-test.h"

namespace mai {
    
namespace table {

class SstTableBuilderTest : public test::TableTest {
public:
    SstTableBuilderTest() {}
    
    TableFactory default_factory_ = [](const core::InternalKeyComparator *ikcmp,
                                       WritableFile *file) {
        return new SstTableBuilder(ikcmp, file, 512, 3);
    };
};
    
TEST_F(SstTableBuilderTest, Sanity) {
    const char *kFileName = "tests/09-sst-table-builder-sanity.tmp";
    
    BuildTable({
        "k1", "v5", "5",
        "k2", "v4", "4",
        "k3", "v3", "3",
        "k4", "v2", "2",
        "k5", "v1", "1",
    }, kFileName, default_factory_);
    
    std::unique_ptr<TableProperties> props;
    uint32_t magic_number = 0;
    ReadProperties(kFileName, &magic_number, &props);
    
    ASSERT_FALSE(props->unordered);
    ASSERT_FALSE(props->last_level);
    
    EXPECT_EQ(5, props->last_version);
    EXPECT_EQ(5, props->num_entries);
    EXPECT_EQ(512, props->block_size);
}

} // namespace table
    
} // namespace mai
