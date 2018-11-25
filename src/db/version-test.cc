#include "db/version.h"
#include "db/column-family.h"
#include "db/table-cache.h"
#include "db/factory.h"
#include "db/compaction.h"
#include "db/config.h"
#include "mai/options.h"
#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace db {
    
using ::mai::core::KeyBoundle;
using ::mai::core::ParsedTaggedKey;

class VersionTest : public ::testing::Test {
public:
    VersionTest()
        : abs_db_path_(env_->GetAbsolutePath("tests/demo"))
        , factory_(Factory::NewDefault())
        , table_cache_(new TableCache(abs_db_path_, options_, factory_.get()))
    {
        int i = 0;
        while (tmp_dirs[i]) {
            env_->MakeDirectory(tmp_dirs[i++], false);
        }
    }
    
    virtual ~VersionTest() {
        int i = 0;
        while (tmp_dirs[i]) {
            env_->DeleteFile(tmp_dirs[i++], true);
        }
    }
    
    void SetUp() override {
        versions_.reset(new VersionSet(abs_db_path_, Options{}, table_cache_.get()));
        versions_->column_families()->NewColumnFamily(ColumnFamilyOptions{},
                                                      "default", 0,
                                                      versions_.get());
    }
    
    void TearDown() override {
        versions_.reset();
    }

    Env *const env_ = Env::Default();
    std::string abs_db_path_;
    Options options_;
    std::unique_ptr<Factory> factory_;
    std::unique_ptr<TableCache> table_cache_;
    std::unique_ptr<VersionSet> versions_;
    
    static std::string l0_range_v1[];
    static std::string l1_range_v1[];
    static const char *tmp_dirs[];
};
    
const char *VersionTest::tmp_dirs[] = {
    "tests/00-ves-pick-compaction-l0",
    "tests/01-ves-pick-compaction-l0-v2",
    "tests/02-ves-pick-compaction-l0l1",
    nullptr,
};
    
std::string VersionTest::l0_range_v1[Config::kMaxNumberLevel0File * 2] = {
    core::KeyBoundle::MakeKey("aaaa", 1, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaab", 2, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaac", 3, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaad", 4, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaae", 5, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaaf", 6, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaag", 7, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaah", 8, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaai", 9, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaaj", 10, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaak", 11, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaal", 12, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaam", 13, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaan", 14, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaao", 15, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaap", 16, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaaq", 17, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaar", 18, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaas", 19, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaat", 20, core::Tag::kFlagValue),
};
    
std::string VersionTest::l1_range_v1[4] = {
    core::KeyBoundle::MakeKey("aaab", 21, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaac", 22, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaau", 23, core::Tag::kFlagValue),
    core::KeyBoundle::MakeKey("aaav", 24, core::Tag::kFlagValue),
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
    VersionSet versions(abs_db_path_, Options{}, table_cache_.get());
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
    
TEST_F(VersionTest, PickCompactionL0) {
    VersionSet vets(tmp_dirs[0], Options{}, table_cache_.get());
    
    VersionPatch patch;
    patch.AddColumnFamily(kDefaultColumnFamilyName, 0, "cc");
    auto smallest = core::KeyBoundle::MakeKey("aaaa", 1, core::Tag::kFlagValue);
    auto largest  = core::KeyBoundle::MakeKey("bbbb", 2, core::Tag::kFlagValue);
    for (int i = 0; i < Config::kMaxNumberLevel0File; ++i) {
        patch.CreateFile(0, 0, i + 1, smallest, largest, 40 * base::kMB,
                         env_->CurrentTimeMicros());
        
    }
    vets.LogAndApply(options_, &patch, nullptr);
    patch.Reset();
    
    CompactionContext ctx;
    auto cfd = vets.column_families()->GetDefault();
    ASSERT_TRUE(cfd->NeedsCompaction());
    ASSERT_TRUE(cfd->PickCompaction(&ctx));
    ASSERT_EQ(0, ctx.level);
    ASSERT_EQ(cfd->current(), ctx.input_version);
    ASSERT_EQ(10, ctx.inputs[0].size());
    ASSERT_EQ(largest, cfd->compaction_point(0));
}

TEST_F(VersionTest, PickCompactionL0_V2) {
    VersionSet vets(tmp_dirs[1], Options{}, table_cache_.get());

    VersionPatch patch;
    patch.AddColumnFamily(kDefaultColumnFamilyName, 0, "cc");
    for (int i = 0; i < Config::kMaxNumberLevel0File; ++i) {
        patch.CreateFile(0, 0, i + 1, l0_range_v1[i * 2], l0_range_v1[i * 2 + 1],
                         80 * base::kMB,
                         env_->CurrentTimeMicros());
        
    }
    vets.LogAndApply(options_, &patch, nullptr);
    patch.Reset();
    
    CompactionContext ctx;
    auto cfd = vets.column_families()->GetDefault();
    ASSERT_TRUE(cfd->NeedsCompaction());
    ASSERT_TRUE(cfd->PickCompaction(&ctx));
    ASSERT_EQ(0, ctx.level);
    ASSERT_EQ(cfd->current(), ctx.input_version);
    ASSERT_EQ(1, ctx.inputs[0].size());
    ASSERT_EQ(0, ctx.inputs[1].size());
    ASSERT_EQ(l0_range_v1[1], cfd->compaction_point(0));
}

TEST_F(VersionTest, PickCompactionL0L1) {
    VersionSet vets(tmp_dirs[1], Options{}, table_cache_.get());
    
    VersionPatch patch;
    patch.AddColumnFamily(kDefaultColumnFamilyName, 0, "cc");
    for (int i = 0; i < Config::kMaxNumberLevel0File; ++i) {
        patch.CreateFile(0, 0, i + 1, l0_range_v1[i * 2], l0_range_v1[i * 2 + 1],
                         80 * base::kMB,
                         env_->CurrentTimeMicros());
        
    }
    patch.CreateFile(0, 1, 22, l1_range_v1[0], l1_range_v1[1], base::kGB,
                     env_->CurrentTimeMicros());
    vets.LogAndApply(options_, &patch, nullptr);
    patch.Reset();
    
    CompactionContext ctx;
    auto cfd = vets.column_families()->GetDefault();
    ASSERT_TRUE(cfd->NeedsCompaction());
    ASSERT_TRUE(cfd->PickCompaction(&ctx));
    ASSERT_EQ(0, ctx.level);
    ASSERT_EQ(cfd->current(), ctx.input_version);
    ASSERT_EQ(1, ctx.inputs[0].size());
    ASSERT_EQ(1, ctx.inputs[1].size());
    ASSERT_EQ(22, ctx.inputs[1][0]->number);
    ASSERT_EQ(l0_range_v1[1], cfd->compaction_point(0));
}

} // namespace db
    
} // namespace mai
