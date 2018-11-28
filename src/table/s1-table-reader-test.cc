#include "table/s1-table-reader.h"
#include "table/s1-table-builder.h"
#include "test/table-test.h"

namespace mai {
    
namespace table {
    
class S1TableReaderTest : public test::TableTest {
public:
    
    TableBuilderFactory default_factory_ = [](const core::InternalKeyComparator *ikcmp,
                                              WritableFile *file) {
        return new S1TableBuilder(ikcmp, file, 17, 512);
    };
    
    TableReaderFactory default_tr_factory_ = [](RandomAccessFile *file, uint64_t file_size) {
        return new S1TableReader(file, file_size, true);
    };
    
    static const char *tmp_dirs[];
}; // class S1TableBuilder

/*static*/ const char *S1TableReaderTest::tmp_dirs[] = {
    "tests/15-s1-table-reader-index.tmp",
    "tests/16-s1-table-reader-get.tmp",
    nullptr,
};
    
TEST_F(S1TableReaderTest, Index) {
    BuildTable({
        "k1", "v5", "5",
        "k2", "v4", "4",
        "k3", "v3", "3",
        "k4", "v2", "2",
        "k5", "v1", "1",
    }, tmp_dirs[0], default_factory_);
    
    std::unique_ptr<RandomAccessFile> file;
    std::unique_ptr<TableReader> reader;
    NewReader(tmp_dirs[0], &file, &reader, default_tr_factory_);
    
    auto rd = static_cast<S1TableReader *>(reader.get());
    auto rs = rd->Prepare();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    ASSERT_EQ(17, rd->index().size());
}
    
TEST_F(S1TableReaderTest, Get) {
    BuildTable({
        "k1", "v5", "5",
        "k2", "v4", "4",
        "k3", "v3", "3",
        "k4", "v2", "2",
        "k5", "v1", "1",
    }, tmp_dirs[1], default_factory_);
    
    std::unique_ptr<RandomAccessFile> file;
    std::unique_ptr<TableReader> reader;
    NewReader(tmp_dirs[1], &file, &reader, default_tr_factory_);
    
    auto rd = static_cast<S1TableReader *>(reader.get());
    auto rs = rd->Prepare();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    std::string value;
    rs = Get(reader.get(), "k1", 6, &value, nullptr);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("v5", value);
}
    
} // namespace table
    
} // namespace mai
