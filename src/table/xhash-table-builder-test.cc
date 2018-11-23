#include "table/xhash-table-builder.h"
#include "table/table.h"
#include "core/key-boundle.h"
#include "core/internal-key-comparator.h"
#include "base/slice.h"
#include "base/hash.h"
#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {

namespace table {
    
class XhashTableBuilderTest : public ::testing::Test {
public:
    void SetUp() override {
        
    }
    
    void TearDown() override {
        
    }
    
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
    
    void BuildTable(const std::vector<std::string> &kvs,
                    const std::string &file_name) {
        std::unique_ptr<WritableFile> file;
        auto rs = env_->NewWritableFile(file_name, false, &file);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        
        file->Truncate(0);
        XhashTableBuilder builder(ikcmp_, file.get(), 23, &base::Hash::Js, 512);
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

TEST_F(XhashTableBuilderTest, Sanity) {

    std::unique_ptr<WritableFile> file;
    auto rs = env_->NewWritableFile("tests/03-xhash-table-file.tmp", false, &file);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    file->Truncate(0);
    XhashTableBuilder builder(ikcmp_, file.get(), 13, &base::Hash::Js, 512);
    
    Add(&builder, "aaaa", "a1111", 1, core::Tag::kFlagValue);
    Add(&builder, "bbbb", "b1111", 2, core::Tag::kFlagValue);
    Add(&builder, "cccc", "c1111", 3, core::Tag::kFlagValue);
    Add(&builder, "dddd", "d1111", 4, core::Tag::kFlagValue);
    Add(&builder, "eeee", "e1111", 5, core::Tag::kFlagValue);
    Add(&builder, "ffff", "f1111", 6, core::Tag::kFlagValue);
    Add(&builder, "gggg", "g1111", 7, core::Tag::kFlagValue);
    
    rs = builder.Finish();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
}
    
TEST_F(XhashTableBuilderTest, KeyReplaced) {
    
    std::unique_ptr<WritableFile> file;
    auto rs = env_->NewWritableFile("tests/04-xhash-table-key-replaced.tmp",
                                    false, &file);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    file->Truncate(0);
    XhashTableBuilder builder(ikcmp_, file.get(), 13, &base::Hash::Js, 512);
    
    Add(&builder, "aaaa", "a1111", 5, core::Tag::kFlagValue);
    Add(&builder, "aaaa", "b1111", 4, core::Tag::kFlagValue);
    Add(&builder, "aaaa", "c1111", 3, core::Tag::kFlagValue);
    Add(&builder, "bbbb", "d1111", 2, core::Tag::kFlagValue);
    Add(&builder, "bbbb", "e1111", 1, core::Tag::kFlagValue);
    
    rs = builder.Finish();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
}
    
TEST_F(XhashTableBuilderTest, Properties) {
    using base::Slice;
    
    std::unique_ptr<RandomAccessFile> file;
    auto rs = env_->NewRandomAccessFile("tests/03-xhash-table-file.tmp", &file);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    uint64_t file_size = 0;
    rs = file->GetFileSize(&file_size);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    ASSERT_GT(file_size, 12);
    
    std::string_view result;
    std::string scratch;
    rs = file->Read(file_size - 4, 4, &result, &scratch);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ(Table::kXmtMagicNumber, Slice::SetU32(result));
    
    rs = file->Read(file_size - 12, 8, &result, &scratch);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    auto props_pos = Slice::SetU64(result);
    ASSERT_NE(0, props_pos);
    
    TableProperties props;
    rs = Table::ReadProperties(file.get(), &props_pos, &props);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    EXPECT_TRUE(props.unordered);
    EXPECT_FALSE(props.last_level);
    EXPECT_EQ(7, props.last_version);
    EXPECT_EQ(7, props.num_entries);
    EXPECT_EQ(512, props.block_size);
    
    core::ParsedTaggedKey ikey;
    core::KeyBoundle::ParseTaggedKey(props.smallest_key, &ikey);
    EXPECT_EQ("aaaa", ikey.user_key);
    EXPECT_EQ(1, ikey.tag.sequence_number());
    core::KeyBoundle::ParseTaggedKey(props.largest_key, &ikey);
    EXPECT_EQ("gggg", ikey.user_key);
    EXPECT_EQ(7, ikey.tag.sequence_number());
}
    
TEST_F(XhashTableBuilderTest, LastLevelBuild) {
    const auto kFileName = "tests/08-xhash-table-last-level.tmp";
    
    BuildTable({
        "aaaa", "a", "0",
        "bbbb", "b", "0",
        "cccc", "c", "0",
        "dddd", "d", "0",
        "eeee", "e", "0",
    }, kFileName);
}

} // namespace table
    
} // namespace mai
