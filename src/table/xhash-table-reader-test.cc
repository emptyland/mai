#include "table/xhash-table-reader.h"
#include "table/xhash-table-builder.h"
#include "table/table.h"
#include "core/key-boundle.h"
#include "core/internal-key-comparator.h"
#include "base/slice.h"
#include "base/hash.h"
#include "mai/env.h"
#include "mai/iterator.h"
#include "mai/options.h"
#include "gtest/gtest.h"
#include <map>

namespace mai {
    
namespace table {
    
class XhashTableReaderTest : public ::testing::Test {
public:
    void Add(XhashTableBuilder *builder,
             std::string_view key,
             std::string_view value,
             core::SequenceNumber version,
             uint8_t flag) {
        base::ScopedMemory scope;
        auto ikey = core::KeyBoundle::New(key, value, version, flag,
                                          base::ScopedAllocator{&scope});
        builder->Add(ikey->key(), ikey->value());
        ASSERT_TRUE(builder->error().ok()) << builder->error().ToString();
    }
    
    Error Get(XhashTableReader *reader,
             std::string_view key,
             core::SequenceNumber version,
             std::string *value,
             core::Tag *tag) {
        base::ScopedMemory scope;
        auto ikey = core::KeyBoundle::New(key, version);
        
        std::string_view result;
        std::string scratch;
        auto rs = reader->Get(ReadOptions{}, ikcmp_, ikey->key(), tag, &result,
                              &scratch);
        *value = result;
        return rs;
    }
    
    void BuildTable(const std::map<std::string, std::string> &kvs,
                    const std::string &file_name) {
        std::unique_ptr<WritableFile> file;
        auto rs = env_->NewWritableFile(file_name, false, &file);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        
        file->Truncate(0);
        XhashTableBuilder builder(ikcmp_, file.get(), 23, 512);
        
        core::SequenceNumber version = kvs.size();
        for (const auto &kv : kvs) {
            Add(&builder, kv.first, kv.second, version--, core::Tag::kFlagValue);
        }
        rs = builder.Finish();
        ASSERT_TRUE(rs.ok()) << rs.ToString();
    }
    
    void BuildTable(const std::vector<std::string> &kvs,
                    const std::string &file_name) {
        std::unique_ptr<WritableFile> file;
        auto rs = env_->NewWritableFile(file_name, false, &file);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        
        file->Truncate(0);
        XhashTableBuilder builder(ikcmp_, file.get(), 23, 512);
        ASSERT_GE(kvs.size(), 3);
        for (int i = 0; i < kvs.size(); i += 3) {
            auto k = kvs[i];
            auto v = kvs[i + 1];
            auto s = ::atoi(kvs[i + 2].c_str());

            Add(&builder, k, v, s, core::Tag::kFlagValue);
        }
        
        rs = builder.Finish();
        ASSERT_TRUE(rs.ok()) << rs.ToString();
    }

    Env *env_ = Env::Default();
    core::InternalKeyComparator *ikcmp_ = new core::InternalKeyComparator(Comparator::Bytewise());
};
    
TEST_F(XhashTableReaderTest, Sanity) {
    std::map<std::string, std::string> kvs;
    
    kvs["aaaa"] = "a1111";
    kvs["bbbb"] = "b1111";
    kvs["cccc"] = "c1111";
    kvs["dddd"] = "d1111";
    kvs["eeee"] = "e1111";
    kvs["ffff"] = "f1111";
    BuildTable(kvs, "tests/05-xhash-table-reader.tmp");
    
    std::unique_ptr<RandomAccessFile> file;
    auto rs = env_->NewRandomAccessFile("tests/05-xhash-table-reader.tmp", &file);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    uint64_t file_size;
    rs = file->GetFileSize(&file_size);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    XhashTableReader reader(file.get(), file_size);
    rs = reader.Prepare();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    auto props = reader.GetTableProperties();
    ASSERT_TRUE(!!props);
    ASSERT_EQ(6, props->num_entries);
    ASSERT_EQ(6, props->last_version);
    ASSERT_FALSE(props->last_level);

    std::string value;
    core::Tag tag;
    rs = Get(&reader, "aaaa", 6, &value, &tag);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ(6, tag.sequence_number());
    ASSERT_EQ(core::Tag::kFlagValue, tag.flag());
    ASSERT_EQ("a1111", value);
    
    rs = Get(&reader, "bbbb", 6, &value, &tag);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("b1111", value);

    rs = Get(&reader, "ffff", 6, &value, &tag);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("f1111", value);
}
    
TEST_F(XhashTableReaderTest, MutilVersion) {
    BuildTable({
        "aaaa", "v5", "5",
        "aaaa", "v4", "4",
        "aaaa", "v3", "3",
        "bbbb", "v2", "2",
        "bbbb", "v1", "1",
    }, "tests/06-xhash-table-reader.tmp");
    
    std::unique_ptr<RandomAccessFile> file;
    auto rs = env_->NewRandomAccessFile("tests/06-xhash-table-reader.tmp", &file);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    uint64_t file_size;
    rs = file->GetFileSize(&file_size);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    XhashTableReader reader(file.get(), file_size);
    rs = reader.Prepare();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    std::string value;
    core::Tag tag;
    rs = Get(&reader, "aaaa", 6, &value, &tag);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("v5", value);
    ASSERT_EQ(core::Tag::kFlagValue, tag.flag());
    ASSERT_EQ(5, tag.sequence_number());
    
    rs = Get(&reader, "aaaa", 4, &value, &tag);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("v4", value);
    ASSERT_EQ(core::Tag::kFlagValue, tag.flag());
    ASSERT_EQ(4, tag.sequence_number());
    
    rs = Get(&reader, "aaaa", 1, &value, nullptr);
    ASSERT_TRUE(rs.IsNotFound());
}
    
TEST_F(XhashTableReaderTest, Iterator) {
    using core::KeyBoundle;
    using core::Tag;
    
    const auto kFileName = "tests/07-xhash-table-reader-iterate.tmp";
    
    BuildTable({
        "aaaa", "v5", "5",
        "aaaa", "v4", "4",
        "aaaa", "v3", "3",
        "bbbb", "v2", "2",
        "bbbb", "v1", "1",
    }, kFileName);
    
    std::unique_ptr<RandomAccessFile> file;
    auto rs = env_->NewRandomAccessFile(kFileName, &file);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    uint64_t file_size;
    rs = file->GetFileSize(&file_size);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    XhashTableReader reader(file.get(), file_size);
    rs = reader.Prepare();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    std::unique_ptr<Iterator> iter(reader.NewIterator(ReadOptions{}, ikcmp_));
    ASSERT_TRUE(iter->error().ok()) << iter->error().ToString();

    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        auto user_key = KeyBoundle::ExtractUserKey(iter->key());
        ASSERT_EQ(4, user_key.size()) << user_key.data();
    }
    
    iter->Seek(KeyBoundle::MakeKey("aaaa", 5, Tag::kFlagValueForSeek));
    ASSERT_TRUE(iter->Valid());
    ASSERT_EQ("aaaa", KeyBoundle::ExtractUserKey(iter->key()));
    ASSERT_EQ(5, KeyBoundle::ExtractTag(iter->key()).sequence_number());
    
    iter->Next();
    ASSERT_TRUE(iter->Valid());
    ASSERT_EQ("aaaa", KeyBoundle::ExtractUserKey(iter->key()));
    ASSERT_EQ(4, KeyBoundle::ExtractTag(iter->key()).sequence_number());

    iter->Next();
    ASSERT_TRUE(iter->Valid());
    ASSERT_EQ("aaaa", KeyBoundle::ExtractUserKey(iter->key()));
    ASSERT_EQ(3, KeyBoundle::ExtractTag(iter->key()).sequence_number());
}
    
} // namespace table
    
} // namespace mai
