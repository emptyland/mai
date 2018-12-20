#include "table/sst-table-reader.h"
#include "table/sst-table-builder.h"
#include "core/key-boundle.h"
#include "core/key-filter.h"
#include "mai/iterator.h"
#include "test/table-test.h"
#include <map>

namespace mai {
    
namespace table {
    
class SstTableReaderTest : public test::TableTest {
public:
    SstTableReaderTest() {}
    
    ~SstTableReaderTest() override {
        int i = 0;
        while (tmp_dirs[i]) {
            env_->DeleteFile(tmp_dirs[i++], true);
        }
    }
    
    TableBuilderFactory default_tb_factory_
        = [](const core::InternalKeyComparator *ikcmp, WritableFile *file) {
            return new SstTableBuilder(ikcmp, file, 512, 3);
        };
    
    TableBuilderFactory k13_restart_tb_factory_
        = [](const core::InternalKeyComparator *ikcmp, WritableFile *file) {
            return new SstTableBuilder(ikcmp, file, 512, 13);
        };
    
    TableReaderFactory default_tr_factory_
        = [](RandomAccessFile *file, uint64_t file_size, table::BlockCache *cache) {
            return new SstTableReader(file, file_size, true, cache);
        };
    
    static const char *tmp_dirs[];
};
    
const char *SstTableReaderTest::tmp_dirs[] = {
    "tests/11-sst-table-reader-sanity.tmp",
    "tests/12-sst-table-reader-index.tmp",
    "tests/13-sst-table-reader-get.tmp",
    "tests/22-sst-table-reader-seq-iter.tmp",
    "tests/23-sst-table-reader-res-iter.tmp",
    nullptr,
};
    
using ::mai::core::KeyBoundle;
using ::mai::core::Tag;
    
TEST_F(SstTableReaderTest, Sanity) {
    const char *kFileName = tmp_dirs[0];
    
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
    
TEST_F(SstTableReaderTest, Index) {
    const char *kFileName = tmp_dirs[1];
    
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
    
    SstTableReader *srd = static_cast<SstTableReader *>(rd.get());
    Error rs = srd->Prepare();
    ASSERT_TRUE(rs.ok()) << rs.ToString();

    std::unique_ptr<Iterator> index_iter(srd->NewIndexIterator(&ikcmp_));
    
    index_iter->Seek(core::KeyBoundle::MakeKey("k001", 100, core::Tag::kFlagValueForSeek));
    ASSERT_TRUE(index_iter->Valid());
    ASSERT_EQ("k030", core::KeyBoundle::ExtractUserKey(index_iter->key()));
    
    index_iter->Seek(core::KeyBoundle::MakeKey("k030", 100, core::Tag::kFlagValueForSeek));
    ASSERT_TRUE(index_iter->Valid());
    ASSERT_EQ("k030", core::KeyBoundle::ExtractUserKey(index_iter->key()));
    
    index_iter->Seek(core::KeyBoundle::MakeKey("k031", 100, core::Tag::kFlagValueForSeek));
    ASSERT_TRUE(index_iter->Valid());
    ASSERT_EQ("k060", core::KeyBoundle::ExtractUserKey(index_iter->key()));
    
    index_iter->Seek(core::KeyBoundle::MakeKey("k060", 100, core::Tag::kFlagValueForSeek));
    ASSERT_TRUE(index_iter->Valid());
    ASSERT_EQ("k060", core::KeyBoundle::ExtractUserKey(index_iter->key()));
}
    
TEST_F(SstTableReaderTest, Get) {
    const char *kFileName = tmp_dirs[2];
    
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
    
    std::string_view value;
    std::string scratch;

    for (int i = 0; i < 100; ++i) {
        char ukey[64];
        snprintf(ukey, arraysize(ukey), "k%03d", i + 1);

        char uval[64];
        snprintf(uval, arraysize(uval), "v%d", i + 1);

        rs = rd->Get(ReadOptions{}, &ikcmp_,
                     core::KeyBoundle::MakeKey(ukey, 100, core::Tag::kFlagValueForSeek),
                     nullptr, &value, &scratch);
        EXPECT_TRUE(rs.ok()) << rs.ToString() << " key:" << ukey;
    }
}

TEST_F(SstTableReaderTest, SequenceIterator) {
    static auto kFileName = tmp_dirs[3];
    
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

    Error rs = down_cast<SstTableReader>(rd.get())->Prepare();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    std::unique_ptr<Iterator> iter(rd->NewIterator(ReadOptions{}, &ikcmp_));
    ASSERT_TRUE(iter->error().ok()) << iter->error().ToString();
    iter->SeekToFirst();
    ASSERT_TRUE(iter->Valid());
    ASSERT_EQ("aaaa", KeyBoundle::ExtractUserKey(iter->key()));
    ASSERT_EQ("v4", iter->value());
    
    iter->Next();
    ASSERT_TRUE(iter->Valid());
    ASSERT_EQ("aaab", KeyBoundle::ExtractUserKey(iter->key()));
    ASSERT_EQ("v3", iter->value());
    
    iter->Next();
    ASSERT_TRUE(iter->Valid());
    ASSERT_EQ("aaac", KeyBoundle::ExtractUserKey(iter->key()));
    ASSERT_EQ("v2", iter->value());
    
    iter->Next();
    ASSERT_TRUE(iter->Valid());
    ASSERT_EQ("aaad", KeyBoundle::ExtractUserKey(iter->key()));
    ASSERT_EQ("v1", iter->value());
    
    iter->Next();
    ASSERT_FALSE(iter->Valid());
}
    
TEST_F(SstTableReaderTest, ReserveIterator) {
    static auto kFileName = tmp_dirs[4];
    
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
    
    Error rs = down_cast<SstTableReader>(rd.get())->Prepare();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    std::unique_ptr<Iterator> iter(rd->NewIterator(ReadOptions{}, &ikcmp_));
    ASSERT_TRUE(iter->error().ok()) << iter->error().ToString();
    
    iter->SeekToLast();
    ASSERT_TRUE(iter->Valid());
    ASSERT_EQ("aaad", KeyBoundle::ExtractUserKey(iter->key()));
    ASSERT_EQ("v1", iter->value());
    
    iter->Prev();
    ASSERT_TRUE(iter->Valid());
    ASSERT_EQ("aaac", KeyBoundle::ExtractUserKey(iter->key()));
    ASSERT_EQ("v2", iter->value());
    
    iter->Prev();
    ASSERT_TRUE(iter->Valid());
    ASSERT_EQ("aaab", KeyBoundle::ExtractUserKey(iter->key()));
    ASSERT_EQ("v3", iter->value());
    
    iter->Prev();
    ASSERT_TRUE(iter->Valid());
    ASSERT_EQ("aaaa", KeyBoundle::ExtractUserKey(iter->key()));
    ASSERT_EQ("v4", iter->value());
    
    iter->Prev();
    ASSERT_FALSE(iter->Valid());
}

    
} // namespace table
    
} // namespace mai
