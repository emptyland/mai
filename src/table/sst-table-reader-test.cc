#include "table/sst-table-reader.h"
#include "table/sst-table-builder.h"
#include "core/key-filter.h"
#include "test/table-test.h"

namespace mai {
    
namespace table {
    
class SstTableReaderTest : public test::TableTest {
public:
    SstTableReaderTest() {}
    
    TableBuilderFactory default_tb_factory_
    = [](const core::InternalKeyComparator *ikcmp, WritableFile *file) {
        return new SstTableBuilder(ikcmp, file, 512, 3);
    };
    
    TableBuilderFactory k13_restart_tb_factory_
    = [](const core::InternalKeyComparator *ikcmp, WritableFile *file) {
        return new SstTableBuilder(ikcmp, file, 512, 13);
    };
    
    TableReaderFactory default_tr_factory_
    = [](RandomAccessFile *file, uint64_t file_size) {
        return new SstTableReader(file, file_size, true);
    };
};
    
TEST_F(SstTableReaderTest, Sanity) {
    const char *kFileName = "tests/11-sst-table-reader-sanity.tmp";
    
    BuildTable({
        "aaaa", "v4", "4",
        "aaab", "v3", "3",
        "aaac", "v2", "2",
        "aaad", "v1", "1",
    }, kFileName, default_tb_factory_);
    
    std::unique_ptr<RandomAccessFile> file;
    std::unique_ptr<TableReader> rd;
    
    NewReader(kFileName, &file, &rd, default_tr_factory_);
    ASSERT_NE(nullptr, file.get());
    ASSERT_NE(nullptr, rd.get());
    
    Error rs = static_cast<SstTableReader *>(rd.get())->Prepare();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    EXPECT_TRUE(rd->GetKeyFilter()->MayExists("aaaa"));
    EXPECT_TRUE(rd->GetKeyFilter()->MayExists("aaab"));
    EXPECT_TRUE(rd->GetKeyFilter()->MayExists("aaac"));
    EXPECT_TRUE(rd->GetKeyFilter()->MayExists("aaad"));
    EXPECT_TRUE(rd->GetKeyFilter()->EnsureNotExists("aaae"));
    
    file->Close();
}
    
} // namespace table
    
} // namespace mai
