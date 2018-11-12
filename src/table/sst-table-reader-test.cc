#include "table/sst-table-reader.h"
#include "table/sst-table-builder.h"
#include "core/key-boundle.h"
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
    
size_t Binsearch(std::vector<int> vals, int target) {
    size_t left = 0;
    size_t right = vals.size();
    while (left < right) {
        size_t middle = (left + right) / 2;
        if (vals[middle] < target) {
            left = middle + 1;
        } else {
            right = middle;
        }
    }
    return right;
}
    
TEST_F(SstTableReaderTest, GetBinsearch) {
    EXPECT_EQ(1, Binsearch({1,3,5,7,9}, 2));
    EXPECT_EQ(2, Binsearch({1,3,5,7,9}, 4));
    
    EXPECT_EQ(0, Binsearch({1,3,5,7,9}, 0));
    EXPECT_EQ(0, Binsearch({1,3,5,7,9}, 1));
}
    
TEST_F(SstTableReaderTest, Get) {
    const char *kFileName = "tests/12-sst-table-reader-get.tmp";
    
    std::vector<std::string> kvs;
    for (int i = 0; i < 100; ++i) {
        char buf[64];
        snprintf(buf, arraysize(buf), "k%03d", i + 1);
        kvs.push_back(buf);
        
        snprintf(buf, arraysize(buf), "v%d", i + 1);
        kvs.push_back(buf);
        
        snprintf(buf, arraysize(buf), "%d", i + 1);
        kvs.push_back(buf);
    }
    BuildTable(kvs, kFileName, default_tb_factory_);
    
    std::unique_ptr<RandomAccessFile> file;
    std::unique_ptr<TableReader> rd;
    
    NewReader(kFileName, &file, &rd, default_tr_factory_);
    ASSERT_NE(nullptr, file.get());
    ASSERT_NE(nullptr, rd.get());
    
    Error rs = static_cast<SstTableReader *>(rd.get())->Prepare();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    static_cast<SstTableReader *>(rd.get())->TEST_PrintAll(ikcmp_.get());
    
    std::string_view value;
    std::string scratch;
    
//    rs = rd->Get(ReadOptions{}, ikcmp_.get(),
//                 core::KeyBoundle::MakeKey("k091", 100), nullptr, &value,
//                 &scratch);
//    EXPECT_TRUE(rs.ok()) << rs.ToString();
//    rs = rd->Get(ReadOptions{}, ikcmp_.get(),
//                 core::KeyBoundle::MakeKey("k031", 100), nullptr, &value,
//                 &scratch);
//    EXPECT_TRUE(rs.ok()) << rs.ToString();
//    rs = rd->Get(ReadOptions{}, ikcmp_.get(),
//                 core::KeyBoundle::MakeKey("k061", 100), nullptr, &value,
//                 &scratch);
//    EXPECT_TRUE(rs.ok()) << rs.ToString();

    for (int i = 0; i < 100; ++i) {
        char ukey[64];
        snprintf(ukey, arraysize(ukey), "k%03d", i + 1);

        char uval[64];
        snprintf(uval, arraysize(uval), "v%d", i + 1);

        rs = rd->Get(ReadOptions{}, ikcmp_.get(),
                     core::KeyBoundle::MakeKey(ukey, 100), nullptr, &value,
                     &scratch);
        EXPECT_TRUE(rs.ok()) << rs.ToString() << " key:" << ukey;
    }
}
    

    
} // namespace table
    
} // namespace mai
