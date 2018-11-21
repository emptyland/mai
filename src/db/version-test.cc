#include "db/version.h"
#include "db/column-family.h"
#include "db/table-cache.h"
#include "db/factory.h"
#include "mai/options.h"
#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace db {

class VersionTest : public ::testing::Test {
public:
    void SetUp() override {
        table_cache_.reset(new TableCache("tests/demo", Env::Default(),
                                          Factory::Default(), true));
        versions_.reset(new VersionSet("tests/demo", Options{}, table_cache_.get()));
        versions_->column_families()->NewColumnFamily(ColumnFamilyOptions{},
                                                      "default", 0,
                                                      versions_.get());
    }
    
    void TearDown() override {
        versions_.reset();
    }

    std::unique_ptr<TableCache> table_cache_;
    std::unique_ptr<VersionSet> versions_;
};

TEST_F(VersionTest, VersionPatchEncodeDecode1) {
    VersionPatch patch;
    patch.set_last_sequence_number(100);
    patch.set_max_column_faimly(99);
    
    std::string buf;
    patch.Encode(&buf);
    ASSERT_EQ(4, buf.size());
    
    VersionPatch restore;
    restore.Decode(buf);
    
    EXPECT_EQ(patch.last_sequence_number() , restore.last_sequence_number());
    EXPECT_EQ(patch.max_column_family(), restore.max_column_family());
}
    
TEST_F(VersionTest, VersionPatchEncodeDecode2) {
    VersionPatch patch;
    patch.AddColumnFamily("default", 0, "def");
    ASSERT_TRUE(patch.has_add_column_family());
    ASSERT_EQ("default", patch.cf_creation().name);
    ASSERT_EQ(0, patch.cf_creation().cfid);
    ASSERT_EQ("def", patch.cf_creation().comparator_name);
    
    std::string buf;
    patch.Encode(&buf);
    ASSERT_EQ(14, buf.size());
    
    VersionPatch restore;
    restore.Decode(buf);
    ASSERT_TRUE(restore.has_add_column_family());
    ASSERT_EQ("default", restore.cf_creation().name);
    ASSERT_EQ(0, restore.cf_creation().cfid);
    ASSERT_EQ("def", restore.cf_creation().comparator_name);
}
    
TEST_F(VersionTest, VersionPatchEncodeDecode3) {
    VersionPatch patch;
    
    patch.CreateFile(0, 1, 1, "aaaa", "bbbb", 14, 1999);
    ASSERT_TRUE(patch.has_creation());
    patch.DeleteFile(0, 1, 1);
    ASSERT_TRUE(patch.has_deletion());
    
    std::string buf;
    patch.Encode(&buf);
    ASSERT_EQ(21, buf.size());
    
    VersionPatch restore;
    restore.Decode(buf);
    
    ASSERT_TRUE(restore.has_creation());
    ASSERT_TRUE(restore.has_deletion());
    
    EXPECT_EQ(1, restore.file_creation().size());
    EXPECT_EQ(1, restore.file_deletion().size());
}
    
TEST_F(VersionTest, LogAndApply) {
    VersionPatch patch;
    
    patch.CreateFile(0, 1, 1, "aaaa", "bbbb", 14, 1999);
    
    ColumnFamilyOptions opts{};
    auto rs = versions_->LogAndApply(opts, &patch, nullptr);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    auto cfid = versions_->column_families()->NextColumnFamilyId();
    patch.Reset();
    patch.AddColumnFamily("d", cfid, opts.comparator->Name());
    rs = versions_->LogAndApply(opts, &patch, nullptr);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    ASSERT_EQ(1, versions_->column_families()->max_column_family());
    
    auto cf0 = versions_->column_families()->GetColumnFamily("default");
    ASSERT_NE(nullptr, cf0);
    ASSERT_EQ(0, cf0->id());
    auto cf1 = versions_->column_families()->GetColumnFamily("d");
    ASSERT_NE(nullptr, cf1);
    ASSERT_EQ(1, cf1->id());
}

TEST_F(VersionTest, Recovery) {
    VersionSet versions("tests/demo",  Options{}, table_cache_.get());
    std::map<std::string, ColumnFamilyOptions> opts;
    std::vector<uint64_t> logs;

    opts["default"] = ColumnFamilyOptions{};
    opts["d"] = ColumnFamilyOptions{};
    
    Error rs = versions.Recovery(opts, 99, &logs);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    ASSERT_EQ(1, versions.column_families()->max_column_family());
    
    auto cf0 = versions.column_families()->GetColumnFamily("default");
    ASSERT_NE(nullptr, cf0);
    ASSERT_EQ(0, cf0->id());
    auto cf1 = versions.column_families()->GetColumnFamily("d");
    ASSERT_NE(nullptr, cf1);
    ASSERT_EQ(1, cf1->id());
}

} // namespace db
    
} // namespace mai
