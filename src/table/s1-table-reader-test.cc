#include "table/s1-table-reader.h"
#include "table/s1-table-builder.h"
#include "core/key-filter.h"
#include "core/key-boundle.h"
#include "base/slice.h"
#include "test/table-test.h"
#include "mai/iterator.h"

namespace mai {
    
namespace table {
    
class S1TableReaderTest : public test::TableTest {
public:
    ~S1TableReaderTest() override {
        int i = 0;
        while (tmp_dirs[i]) {
            env_->DeleteFile(tmp_dirs[i++], true);
        }
    }
    
    TableBuilderFactory default_factory_ = [](const core::InternalKeyComparator *ikcmp,
                                              WritableFile *file) {
        return new S1TableBuilder(ikcmp, file, 17, 512);
    };
    
    TableBuilderFactory v2_factory_ = [](const core::InternalKeyComparator *ikcmp,
                                         WritableFile *file) {
        return new S1TableBuilder(ikcmp, file, 1024, 4096);
    };
    
    TableReaderFactory default_tr_factory_ = [](RandomAccessFile *file, uint64_t file_size) {
        return new S1TableReader(file, file_size, true);
    };
    
    static const char *tmp_dirs[];
}; // class S1TableBuilder

/*static*/ const char *S1TableReaderTest::tmp_dirs[] = {
    "tests/15-s1-table-reader-index.tmp",
    "tests/16-s1-table-reader-get.tmp",
    "tests/17-s1-table-reader-replace-get.tmp",
    "tests/18-s1-table-reader-iterator.tmp",
    "tests/19-s1-table-reader-iterator-v2.tmp",
    "tests/20-s1-table-reader-props.tmp",
    "tests/21-s1-table-reader-bloom-filter.tmp",
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
    
    rs = Get(reader.get(), "k5", 6, &value, nullptr);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("v1", value);
}
    
TEST_F(S1TableReaderTest, ReplacedGet) {
    BuildTable({
        "kkkk1", "v5-target", "5",
        "kkkk1", "v4", "4",
        "kkk2", "v3-target", "3",
        "kkk2", "v2", "2",
        "kkk2", "v1", "1",
    }, tmp_dirs[2], default_factory_);
    
    std::unique_ptr<RandomAccessFile> file;
    std::unique_ptr<TableReader> reader;
    NewReader(tmp_dirs[2], &file, &reader, default_tr_factory_);
    
    auto rd = static_cast<S1TableReader *>(reader.get());
    auto rs = rd->Prepare();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    std::string value;
    rs = Get(reader.get(), "k1", 6, &value, nullptr);
    ASSERT_TRUE(rs.fail());
    
    rs = Get(reader.get(), "kkkk1", 6, &value, nullptr);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("v5-target", value);
    
    rs = Get(reader.get(), "kkkk1", 4, &value, nullptr);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("v4", value);
    
    rs = Get(reader.get(), "kkk2", 6, &value, nullptr);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("v3-target", value);
}
    
TEST_F(S1TableReaderTest, Iterator) {
    BuildTable({
        "k1", "v5", "5",
        "k2", "v4", "4",
        "k3", "v3", "3",
        "k4", "v2", "2",
        "k5", "v1", "1",
    }, tmp_dirs[3], default_factory_);
    
    std::unique_ptr<RandomAccessFile> file;
    std::unique_ptr<TableReader> reader;
    NewReader(tmp_dirs[3], &file, &reader, default_tr_factory_);
    
    auto rd = static_cast<S1TableReader *>(reader.get());
    auto rs = rd->Prepare();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    std::unique_ptr<Iterator> iter(reader->NewIterator(ReadOptions{}, &ikcmp_));
    ASSERT_TRUE(iter->error().ok()) << iter->error().ToString();
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        std::string k(core::KeyBoundle::ExtractUserKey(iter->key()));
        std::string v(iter->value());
        
        EXPECT_EQ('k', k[0]);
        EXPECT_EQ('v', v[0]);
    }
}
    
TEST_F(S1TableReaderTest, IteratorV2) {
    std::vector<std::string> kvs;
    std::set<std::string> keys;
    for (int i = 0; i < 34; ++i) {
        std::string key = base::Slice::Sprintf("k.%03d", i);
        kvs.push_back(key);
        kvs.push_back(base::Slice::Sprintf("v.%03d", i));
        kvs.push_back(base::Slice::Sprintf("%d", i + 1));
        keys.insert(key);
    }
    ASSERT_EQ(34, keys.size());
    
    BuildTable(kvs, tmp_dirs[4], default_factory_);
    
    std::unique_ptr<RandomAccessFile> file;
    std::unique_ptr<TableReader> reader;
    NewReader(tmp_dirs[4], &file, &reader, default_tr_factory_);
    
    auto rd = static_cast<S1TableReader *>(reader.get());
    auto rs = rd->Prepare();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    std::unique_ptr<Iterator> iter(reader->NewIterator(ReadOptions{}, &ikcmp_));
    ASSERT_TRUE(iter->error().ok()) << iter->error().ToString();
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        std::string k(core::KeyBoundle::ExtractUserKey(iter->key()));
        std::string v(iter->value());
        
        EXPECT_EQ('k', k[0]);
        EXPECT_EQ('v', v[0]);
        keys.erase(k);
    }

    for (auto s : keys) {
        printf("remain: %s\n", s.c_str());
    }
    ASSERT_TRUE(keys.empty());
}

TEST_F(S1TableReaderTest, Properties) {
    std::string v(1024, 'C');
    std::vector<std::string> kvs;
    for (int i = 0; i < 1024 * 2; ++i) {
        kvs.push_back(base::Slice::Sprintf("k.%04d", i));
        kvs.push_back(v);
        kvs.push_back(base::Slice::Sprintf("%d", i + 1));
    }
    
    BuildTable(kvs, tmp_dirs[5], v2_factory_);
    
    std::unique_ptr<RandomAccessFile> file;
    std::unique_ptr<TableReader> reader;
    NewReader(tmp_dirs[5], &file, &reader, default_tr_factory_);
    
    auto rd = static_cast<S1TableReader *>(reader.get());
    auto rs = rd->Prepare();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    auto props = reader->GetTableProperties()->mutable_data();
    ASSERT_EQ(4096, props->block_size);
    ASSERT_EQ(1024 * 2, props->num_entries);
    ASSERT_TRUE(props->unordered);
    ASSERT_FALSE(props->last_level);
    ASSERT_EQ("k.0000", core::KeyBoundle::ExtractUserKey(props->smallest_key));
    ASSERT_EQ("k.2047", core::KeyBoundle::ExtractUserKey(props->largest_key));
    ASSERT_NE(0, props->last_version);

    std::string value;
    for (int i = 0; i < 1024 * 2; ++i) {
        auto k = base::Slice::Sprintf("k.%04d", i);
        rs = Get(reader.get(), k, core::Tag::kMaxSequenceNumber, &value, nullptr);
        EXPECT_TRUE(rs.ok()) << k;
    }
}
    
TEST_F(S1TableReaderTest, BloomFilter) {
    std::vector<std::string> kvs;
    std::set<std::string> keys;
    for (int i = 0; i < 17 * 4; ++i) {
        std::string key = base::Slice::Sprintf("k.%03d", i);
        kvs.push_back(key);
        kvs.push_back(base::Slice::Sprintf("v.%03d", i));
        kvs.push_back(base::Slice::Sprintf("%d", i + 1));
        keys.insert(key);
    }
    
    BuildTable(kvs, tmp_dirs[6], default_factory_);
    
    std::unique_ptr<RandomAccessFile> file;
    std::unique_ptr<TableReader> reader;
    NewReader(tmp_dirs[6], &file, &reader, default_tr_factory_);
    
    auto rd = static_cast<S1TableReader *>(reader.get());
    auto rs = rd->Prepare();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    for (auto k : keys) {
        ASSERT_TRUE(reader->GetKeyFilter()->MayExists(k)) << k;
    }
    ASSERT_TRUE(reader->GetKeyFilter()->EnsureNotExists("kkk"));
    ASSERT_TRUE(reader->GetKeyFilter()->EnsureNotExists("ddd"));
    ASSERT_TRUE(reader->GetKeyFilter()->EnsureNotExists("ensure not exists!"));
}

#if 0
TEST_F(S1TableReaderTest, RealFile) {
    std::unique_ptr<RandomAccessFile> file;
    env_->NewRandomAccessFile("tests/179.s1t", &file);
    
    uint64_t size;
    file->GetFileSize(&size);
    S1TableReader reader(file.get(), size, true);
    reader.Prepare();
    
    std::unique_ptr<Iterator> iter(reader.NewIterator(ReadOptions{}, &ikcmp_));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        std::string k(core::KeyBoundle::ExtractUserKey(iter->key()));
        if (k.length() >= 9) {
            printf("hit! %s\n", k.c_str());
        }
        //printf("%s\n", k.c_str());
    }
    auto prop = reader.GetTableProperties();
    
    std::string_view value;
    std::string scatch;
    std::string lookup_key(core::KeyBoundle::MakeKey("k.1000000",
                                                     core::Tag::kMaxSequenceNumber,
                                                     core::Tag::kFlagValueForSeek));
    Error rs = reader.Get(ReadOptions{}, &ikcmp_, lookup_key,
                          nullptr, &value, &scatch);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    auto k1 = core::KeyBoundle::MakeKey("k.1000000", core::Tag::kMaxSequenceNumber, core::Tag::kFlagValueForSeek);
    printf("%d, %d\n", ikcmp_.Compare(k1, prop->smallest_key),
           ikcmp_.Compare(k1, prop->largest_key));
}
#endif
    

} // namespace table
    
} // namespace mai
