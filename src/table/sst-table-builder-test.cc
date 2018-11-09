#include "table/sst-table-builder.h"
#include "table/key-bloom-filter.h"
#include "core/key-filter.h"
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
    
    TableFactory k13_restart_factory_ = [](const core::InternalKeyComparator *ikcmp,
                                       WritableFile *file) {
        return new SstTableBuilder(ikcmp, file, 512, 13);
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
    
TEST_F(SstTableBuilderTest, KeyFilter) {
    const char *kFileName = "tests/10-sst-table-builder-filter.tmp";
    
    std::vector<std::string> kvs;
    for (int i = 0; i < 130; ++i) {
        char buf[64];
        
        snprintf(buf, arraysize(buf), "k%d", i + 1);
        kvs.push_back(buf);
        
        snprintf(buf, arraysize(buf), "v%d", i + 1);
        kvs.push_back(buf);
        
        snprintf(buf, arraysize(buf), "%d", i + 1);
        kvs.push_back(buf);
    }
    BuildTable(kvs, kFileName, k13_restart_factory_);
    
    std::unique_ptr<TableProperties> props;
    uint32_t magic_number = 0;
    ReadProperties(kFileName, &magic_number, &props);
    
    ASSERT_FALSE(props->unordered);
    ASSERT_FALSE(props->last_level);
    
    std::unique_ptr<RandomAccessFile> file;
    auto rs = env_->NewRandomAccessFile(kFileName, &file);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    std::string_view result;
    std::string scratch;
    rs = file->Read(props->filter_position + 4, props->filter_size - 4, &result,
                    &scratch);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    KeyBloomFilter filter(reinterpret_cast<const uint32_t *>(result.data()),
                          result.size() / 4,
                          base::Hash::kBloomFilterHashs,
                          base::Hash::kNumberBloomFilterHashs);
    ASSERT_TRUE(filter.EnsureNotExists("k"));
    ASSERT_TRUE(filter.EnsureNotExists("kk"));
    ASSERT_TRUE(filter.EnsureNotExists("aaaaaaabbb"));
    
    for (int i = 0; i < 130; ++i) {
        char buf[64];
        
        snprintf(buf, arraysize(buf), "k%d", i + 1);
        kvs.push_back(buf);
        
        ASSERT_TRUE(filter.MayExists(buf)) << buf;
    }
    
}

} // namespace table
    
} // namespace mai
