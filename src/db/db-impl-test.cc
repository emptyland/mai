#include "db/db-impl.h"
#include "db/column-family.h"
#include "db/table-cache.h"
#include "base/slice.h"
#include "mai/iterator.h"
#include "mai/env.h"
#include "mai/helper.h"
#include "gtest/gtest.h"
#include <vector>
#include <thread>

namespace mai {
    
namespace db {
    
class DBImplTest : public ::testing::Test {
public:
    DBImplTest() {
        ColumnFamilyDescriptor desc;
        desc.name = kDefaultColumnFamilyName;
        descs_.push_back(desc);
        options_.create_if_missing = true;
        options_.allow_mmap_reads = true;
    }
    
    ~DBImplTest() override {
        int i = 0;
        while (tmp_dirs[i]) {
            env_->DeleteFile(tmp_dirs[i++], true);
        }
    }
    
    void PrintFiles(DB *db, ColumnFamily *cf) {
        std::string key(base::Slice::Sprintf("db.cf.%s.levels",
                                             cf->name().c_str()));
        std::string value;
        Error rs = db->GetProperty(key, &value);
        printf("%s\n", value.c_str());
    }
    
    Env *env_ = Env::Default();
    std::vector<ColumnFamilyDescriptor> descs_;
    Options options_;
    static const char *tmp_dirs[];
};
    
const char *DBImplTest::tmp_dirs[] = {
    "tests/00-db-sanity",
    "tests/01-db-add-cfs",
    "tests/02-db-add-err-cfs",
    "tests/03-db-mem-put-key",
    "tests/04-db-all-cfs",
    "tests/05-db-recovery-manifest",
    "tests/06-db-recovery-data",
    "tests/07-db-write-lv0",
    "tests/08-db-drop-cfs",
    "tests/09-db-mutil-cf-recovery",
    "tests/10-db-concurrent-putting",
    "tests/11-db-unordered-cf",
    "tests/12-db-unordered-cf-recovery",
    "tests/13-db-ordered-cf-recovery",
    "tests/14-db-unordered-put-unordered-cf",
    "tests/15-db-snapshot-get",
    "tests/16-db-two-cf-write",
    "tests/17-db-concurrent-get",
    "tests/18-db-get-properties",
    nullptr,
};
    
TEST_F(DBImplTest, Sanity) {
    std::vector<ColumnFamily *> cfs;
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[0], options_));
    
    Error rs = impl->Open(descs_, &cfs);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    auto cf = cfs[0];
    ASSERT_EQ("default", cf->name());
    ASSERT_EQ(0, cf->id());
    EXPECT_EQ(cf->name(), impl->DefaultColumnFamily()->name());
    EXPECT_EQ(cf->id(), impl->DefaultColumnFamily()->id());
    
    for (auto cf : cfs) {
        impl->ReleaseColumnFamily(cf);
    }
}
    
TEST_F(DBImplTest, AddColumnFamilies) {
    std::vector<ColumnFamily *> cfs;
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[1], options_));
    
    Error rs = impl->Open(descs_, &cfs);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    for (auto cf : cfs) {
        impl->ReleaseColumnFamily(cf);
    }
    
    rs = impl->NewColumnFamilies({"d1", "d2", "d3"}, ColumnFamilyOptions{}, &cfs);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    EXPECT_EQ(3, cfs.size());
    EXPECT_EQ("d1", cfs[0]->name());
    EXPECT_EQ(1, cfs[0]->id());
    EXPECT_EQ("d2", cfs[1]->name());
    EXPECT_EQ(2, cfs[1]->id());
    EXPECT_EQ("d3", cfs[2]->name());
    EXPECT_EQ(3, cfs[2]->id());
    
    for (auto cf : cfs) {
        impl->ReleaseColumnFamily(cf);
    }
}
    
TEST_F(DBImplTest, AddErrorColumnFamilies) {
    std::vector<ColumnFamily *> cfs;
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[2], options_));
    Error rs = impl->Open(descs_, &cfs);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    for (auto cf : cfs) {
        impl->ReleaseColumnFamily(cf);
    }
    
    rs = impl->NewColumnFamilies({"default"}, ColumnFamilyOptions{}, &cfs);
    ASSERT_TRUE(rs.fail());
    ASSERT_TRUE(rs.IsCorruption());
}
    
TEST_F(DBImplTest, PutKey) {
    std::vector<ColumnFamily *> cfs;
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[3], options_));
    Error rs = impl->Open(descs_, &cfs);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    for (auto cf : cfs) {
        impl->ReleaseColumnFamily(cf);
    }
    
    auto cf0 = impl->DefaultColumnFamily();
    ASSERT_TRUE(!!cf0);
    
    WriteOptions write_opts;
    rs = impl->Put(write_opts, cf0, "aaaa", "100");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = impl->Put(write_opts, cf0, "aaaa", "101");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
}
    
TEST_F(DBImplTest, AllColumnFamilies) {
    std::vector<ColumnFamily *> cfs;
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[4], options_));
    Error rs = impl->Open(descs_, &cfs);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    for (auto cf : cfs) {
        impl->ReleaseColumnFamily(cf);
    }
    
    rs = impl->NewColumnFamilies({"cf1", "cf2", "cf3"}, ColumnFamilyOptions{}, &cfs);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    for (auto cf : cfs) {
        impl->ReleaseColumnFamily(cf);
    }
    
    rs = impl->GetAllColumnFamilies(&cfs);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    EXPECT_EQ(4, cfs.size());
    EXPECT_EQ(0, cfs[0]->id());
    EXPECT_EQ("default", cfs[0]->name());
    
    for (auto cf : cfs) {
        impl->ReleaseColumnFamily(cf);
    }
}
    
TEST_F(DBImplTest, RecoveryManifest) {
    std::vector<ColumnFamily *> cfs;
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[5], options_));
    Error rs = impl->Open(descs_, &cfs);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    for (auto cf : cfs) {
        impl->ReleaseColumnFamily(cf);
    }
    
    impl.reset(new DBImpl(tmp_dirs[5], options_));
    rs = impl->Open(descs_, &cfs);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ(1, cfs.size());
    ASSERT_EQ(0, cfs[0]->id());
    ASSERT_EQ(kDefaultColumnFamilyName, cfs[0]->name());
    
    for (auto cf : cfs) {
        impl->ReleaseColumnFamily(cf);
    }
}
    
TEST_F(DBImplTest, RecoveryData) {
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[6], options_));
    ColumnFamilyCollection scope(impl.get());
    Error rs = impl->Open(descs_, scope.ReceiveAll());
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    auto cf0 = scope.newest();
    
    WriteOptions wr_opts;
    rs = impl->Put(wr_opts, cf0, "aaaa", "100");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    rs = impl->Put(wr_opts, cf0, "bbbb", "200");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    rs = impl->Put(wr_opts, cf0, "cccc", "300");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    rs = impl->Put(wr_opts, cf0, "dddd", "400");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    rs = impl->Put(wr_opts, cf0, "eeee", "500");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    scope.ReleaseAll();
    
    impl.reset(new DBImpl(tmp_dirs[6], options_));
    scope.Reset(impl.get());
    rs = impl->Open(descs_, scope.ReceiveAll());
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    cf0 = scope.newest();
    
    std::string value;
    ReadOptions rd_opts;
    rs = impl->Get(rd_opts, cf0, "aaaa", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("100", value);
    rs = impl->Get(rd_opts, cf0, "eeee", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("500", value);
}
    
TEST_F(DBImplTest, WriteLevel0Table) {
    std::vector<ColumnFamily *> cfs;
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[7], options_));
    Error rs = impl->Open(descs_, &cfs);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    auto cf0 = cfs[0];
    
    WriteOptions wr_opts;
    impl->Put(wr_opts, cf0, "aaaa", "100");
    impl->Put(wr_opts, cf0, "aaab", "200");
    impl->Put(wr_opts, cf0, "aaac", "300");
    impl->Put(wr_opts, cf0, "aaad", "400");
    impl->Put(wr_opts, cf0, "aaae", "500");
    rs = impl->TEST_ForceDumpImmutableTable(cf0, true);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    impl->Put(wr_opts, cf0, "bbbb", "1000");
    impl->Put(wr_opts, cf0, "bbbb", "2000");
    impl->Put(wr_opts, cf0, "bbbb", "3000");
    //impl->TEST_MakeImmutablePipeline(cf0);
    rs = impl->TEST_ForceDumpImmutableTable(cf0, true);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    static const char *kv[] = {
        "aaaa", "100",
        "aaab", "200",
        "aaac", "300",
        "aaad", "400",
        "aaae", "500",
        "bbbb", "3000",
    };
    
    int i = 0;
    std::unique_ptr<Iterator> iter(impl->NewIterator(ReadOptions{}, cf0));
    ASSERT_TRUE(iter->error().ok()) << iter->error().ToString();
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        std::string k(iter->key());
        std::string v(iter->value());
        
        EXPECT_EQ(kv[i++], k) << i;
        EXPECT_EQ(kv[i++], v) << i;
    }
    ASSERT_TRUE(iter->error().ok()) << iter->error().ToString();
    
    for (auto cf : cfs) {
        impl->ReleaseColumnFamily(cf);
    }
    
    ASSERT_TRUE(env_->FileExists(impl->abs_db_path() + "/default/3.sst").ok());
    ASSERT_TRUE(env_->FileExists(impl->abs_db_path() + "/default/5.sst").ok());
    //ASSERT_TRUE(env_->FileExists(impl->abs_db_path() + "/default/4.sst").ok());
    //ASSERT_TRUE(env_->FileExists(impl->abs_db_path() + "/0.log").ok());
    ASSERT_TRUE(env_->FileExists(impl->abs_db_path() + "/4.log").ok());
}

TEST_F(DBImplTest, DropColumnFamily) {
    std::vector<ColumnFamily *> cfs;
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[8], options_));
    std::vector<ColumnFamilyDescriptor> descs = {
        {kDefaultColumnFamilyName, ColumnFamilyOptions{}},
        {"cf1", ColumnFamilyOptions{}}
    };
    Error rs = impl->Open(descs, &cfs);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    auto cf0 = cfs[0];
    auto cf1 = cfs[1];
    
    rs = impl->DropColumnFamily(cf0);
    ASSERT_TRUE(rs.fail());
    ASSERT_TRUE(rs.IsCorruption());
    
    rs = impl->DropColumnFamily(cf1);
    ASSERT_TRUE(rs.ok()) << rs.ToString();

    impl->ReleaseColumnFamily(cf0);
}
    
TEST_F(DBImplTest, MutilCFRecovery) {
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[9], options_));
    Error rs = impl->Open(descs_, nullptr);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    ColumnFamily *cf1;
    rs = impl->NewColumnFamily("cf1", ColumnFamilyOptions{}, &cf1);
    ASSERT_TRUE(rs.ok()) << rs.ToString();

    static const char *kv1[] = {
        "aaaa", "100",
        "aaab", "200",
        "aaac", "300",
        "aaad", "400",
        "aaae", "500",
        "bbbb", "3000",
    };
    for (int i = 0; i < arraysize(kv1); i += 2) {
        rs = impl->Put(WriteOptions{}, cf1, kv1[i], kv1[i + 1]);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
    }
    
    static const char *kv2[] = {
        "bbba", "100",
        "bbbb", "200",
        "bbbc", "300",
        "bbbd", "400",
        "bbbe", "500",
        "cccc", "3000",
    };
    auto cf0 = impl->DefaultColumnFamily();
    for (int i = 0; i < arraysize(kv2); i += 2) {
        rs = impl->Put(WriteOptions{}, cf0, kv2[i], kv2[i + 1]);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
    }
    //impl->TEST_MakeImmutablePipeline(cf0);
    rs = impl->TEST_ForceDumpImmutableTable(cf0, true);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    impl->ReleaseColumnFamily(cf1);
    
    std::string value;
    rs = impl->GetProperty("db.versions.last-sequence-number", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("13", value);
    
    rs = impl->GetProperty("db.log.active", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("2,0", value);
    
    
    //---------------Recovery---------------------------------------------------
    std::vector<ColumnFamilyDescriptor> descs;
    ColumnFamilyDescriptor desc;
    desc.name = "default";
    descs.push_back(desc);
    desc.name = "cf1";
    descs.push_back(desc);
    
    std::vector<ColumnFamily *> cfs;
    impl.reset(new DBImpl(tmp_dirs[9], options_));
    rs = impl->Open(descs, &cfs);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    int i = 0;
    std::unique_ptr<Iterator> iter(impl->NewIterator(ReadOptions{}, cfs[0]));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        std::string k(iter->key());
        std::string v(iter->value());
        
        EXPECT_EQ(kv2[i++], k) << i / 2;
        EXPECT_EQ(kv2[i++], v) << i / 2;
    }
    
    i = 0;
    iter.reset(impl->NewIterator(ReadOptions{}, cfs[1]));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        std::string k(iter->key());
        std::string v(iter->value());
        
        EXPECT_EQ(kv1[i++], k) << i / 2;
        EXPECT_EQ(kv1[i++], v) << i / 2;
    }
    
    for (auto cf : cfs) {
        impl->ReleaseColumnFamily(cf);
    }
}
    
TEST_F(DBImplTest, ConcurrentPutting) {
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[10], options_));
    Error rs = impl->Open(descs_, nullptr);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    std::atomic<size_t> written(0);
    
    auto jiffies = env_->CurrentTimeMicros();
    auto cf0 = impl->DefaultColumnFamily();
    WriteOptions wr{};
    std::thread worker_thrds[8];
    std::string value(10240, 'A');
    for (int i = 0; i < arraysize(worker_thrds); ++i) {
        worker_thrds[i] = std::thread([&] (auto slot) {
            for (int j = 0; j < 10240; ++j) {
                std::string key = base::Slice::Sprintf("k.%d.%d", slot, j);

                rs = impl->Put(wr, cf0, key, value);
                EXPECT_TRUE(rs.ok()) << rs.ToString();
                
                written.fetch_add(key.size() + value.size());
            }
        }, i);
    }
    
    for (int i = 0; i < arraysize(worker_thrds); ++i) {
        worker_thrds[i].join();
    }
    printf("total: %lu cost: %f ms\n", written.load(),
           (env_->CurrentTimeMicros() - jiffies) / 1000.0);
}
    
TEST_F(DBImplTest, UnorderedColumnFamily) {
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[11], options_));
    ColumnFamilyCollection scope(impl.get());
    
    Error rs = impl->Open(descs_, nullptr);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    ColumnFamilyOptions opts;
    opts.use_unordered_table = true;

    rs = impl->NewColumnFamily("cf1", opts, scope.Receive());
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    ColumnFamily *cf0 = scope.newest();
    for (int i = 0; i < opts.number_of_hash_slots; ++i) {
        std::string key = base::Slice::Sprintf("k.%03d", i);
        std::string val = base::Slice::Sprintf("v.%03d", i);
        
        rs = impl->Put(WriteOptions{}, cf0, key, val);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
    }
    
    for (int i = 0; i < opts.number_of_hash_slots; ++i) {
        std::string key = base::Slice::Sprintf("k.%03d", i);
        std::string val = base::Slice::Sprintf("v.%03d", i);
        
        std::string value;
        rs = impl->Get(ReadOptions{}, cf0, key, &value);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        EXPECT_EQ(val, value) << i;
    }
}

TEST_F(DBImplTest, UnorderedColumnFamilyRecovery) {
    static const auto kN = 1024 * 1024;
    
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[12], options_));
    ColumnFamilyCollection scope(impl.get());

    Error rs = impl->Open(descs_, nullptr);
    ASSERT_TRUE(rs.ok()) << rs.ToString();

    ColumnFamilyOptions opts;
    opts.use_unordered_table = true;
    opts.number_of_hash_slots = 1024 * 30 + 1;

    rs = impl->NewColumnFamily("unordered", opts, scope.Receive());
    ASSERT_TRUE(rs.ok()) << rs.ToString();

    ColumnFamily *cf0 = scope.newest();

    size_t total_size = 0;
    uint64_t jiffies = env_->CurrentTimeMicros();
    WriteOptions wr;
    for (int i = 0; i < kN; ++i) {
        std::string key = base::Slice::Sprintf("k.%07d", i);
        std::string val = base::Slice::Sprintf("v.%d", i);

        rs = impl->Put(wr, cf0, key, val);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        total_size += key.size() + val.size();
    }
    printf("total size: %lu cost: %f ms\n", total_size,
           (env_->CurrentTimeMicros() - jiffies) / 1000.0f);

    scope.ReleaseAll();
    impl.reset(new DBImpl(tmp_dirs[12], options_));
    scope.Reset(impl.get());
    
//    Error rs;
//    ColumnFamily *cf0;
//    std::unique_ptr<DBImpl> impl(new DBImpl("tests/12-db-unordered-cf-recovery", options_));
//    ColumnFamilyCollection scope(impl.get());
    
    std::vector<ColumnFamilyDescriptor> descs;
    ColumnFamilyDescriptor desc;
    desc.name = kDefaultColumnFamilyName;
    desc.options.use_unordered_table = false;
    descs.push_back(desc);
    desc.name = "unordered";
    desc.options.use_unordered_table = true;
    desc.options.number_of_hash_slots = opts.number_of_hash_slots;
    descs.push_back(desc);
    
    // 1048577
    rs = impl->Open(descs, scope.ReceiveAll());
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    cf0 = scope.GetOrNull("unordered");
    PrintFiles(impl.get(), cf0);
    
    std::string value;
    rs = impl->Get(ReadOptions{}, cf0, "k.1000000", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = impl->Get(ReadOptions{}, cf0, "k.0999999", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = impl->Get(ReadOptions{}, cf0, "k.0000000", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = impl->Get(ReadOptions{}, cf0, base::Slice::Sprintf("k.%d", kN - 1),
                   &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    for (int i = 0; i < 1000; ++i) {
        std::string key = base::Slice::Sprintf("k.%07d", rand() % kN);

        rs = impl->Get(ReadOptions{}, cf0, key, &value);
        ASSERT_TRUE(rs.ok()) << rs.ToString() << " : " << key;
    }
}
    
TEST_F(DBImplTest, OrderedColumnFamilyRecovery) {
    static const auto kN = 1024 * 1024;
    
    Options options;
    options.write_buffer_size = 20 * base::kMB;
    options.allow_mmap_reads = true;
    options.create_if_missing = true;
    
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[13], options));
    ColumnFamilyCollection scope(impl.get());
    
    Error rs = impl->Open({}, nullptr);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    auto cf0 = impl->DefaultColumnFamily();
    
    size_t total_size = 0;
    uint64_t jiffies = env_->CurrentTimeMicros();
    std::string val(1024, 'E');
    WriteOptions wr;
    for (int i = 0; i < kN; ++i) {
        std::string key = base::Slice::Sprintf("k.%d", i);
        //std::string val = base::Slice::Sprintf("v.%d", i);
        
        rs = impl->Put(wr, cf0, key, val);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        total_size += key.size() + val.size();
    }
    printf("total size: %lu cost: %f ms\n", total_size,
           (env_->CurrentTimeMicros() - jiffies) / 1000.0f);
    PrintFiles(impl.get(), cf0);
    
    //----------------Recovery--------------------------------------------------
    
    scope.ReleaseAll();
    impl.reset(new DBImpl(tmp_dirs[13], options));
    scope.Reset(impl.get());
    
    rs = impl->Open(descs_, scope.ReceiveAll());
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    cf0 = impl->DefaultColumnFamily();
    
    std::string value;
    rs = impl->Get(ReadOptions{}, cf0, "k.1000000", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = impl->Get(ReadOptions{}, cf0, "k.999999", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = impl->Get(ReadOptions{}, cf0, "k.0", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = impl->Get(ReadOptions{}, cf0, base::Slice::Sprintf("k.%d", kN - 1),
                   &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    for (int i = 0; i < 1000; ++i) {
        std::string key = base::Slice::Sprintf("k.%d", rand() % kN);
        
        rs = impl->Get(ReadOptions{}, cf0, key, &value);
        ASSERT_TRUE(rs.ok()) << rs.ToString() << " : " << key;
    }
}
    
TEST_F(DBImplTest, UnorderedPutUnorderedCF) {
    static const auto kN = 1024 * 1024;
    
    Options options;
    options.number_of_hash_slots = 1024 * 2 + 1;
    options.use_unordered_table = true;
    options.allow_mmap_reads = true;
    options.create_if_missing = true;
    
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[14], options));
    ColumnFamilyCollection scope(impl.get());
    
    Error rs = impl->Open({}, nullptr);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    auto cf0 = impl->DefaultColumnFamily();
    
    size_t total_size = 0;
    uint64_t jiffies = env_->CurrentTimeMicros();
    WriteOptions wr;
    for (int i = 0; i < kN; ++i) {
        std::string key = base::Slice::Sprintf("k.%d", i);
        std::string val = base::Slice::Sprintf("v.%d", i);
        
        rs = impl->Put(wr, cf0, key, val);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        total_size += key.size() + val.size();
    }
    printf("total size: %lu cost: %f ms\n", total_size,
           (env_->CurrentTimeMicros() - jiffies) / 1000.0f);
    PrintFiles(impl.get(), cf0);
    
    std::string value;
    rs = impl->Get(ReadOptions{}, cf0, "k.1000000", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = impl->Get(ReadOptions{}, cf0, "k.999999", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = impl->Get(ReadOptions{}, cf0, "k.0", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = impl->Get(ReadOptions{}, cf0, base::Slice::Sprintf("k.%d", kN - 1),
                   &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    for (int i = 0; i < 1000; ++i) {
        std::string key = base::Slice::Sprintf("k.%d", rand() % kN);
        rs = impl->Get(ReadOptions{}, cf0, key, &value);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
    }
}
    
TEST_F(DBImplTest, SnapshotGetting) {
    static const auto kN = 1024;
    
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[15], options_));
    ColumnFamilyCollection scope(impl.get());
    impl->Open({}, scope.ReceiveAll());
    
    auto cf0 = impl->DefaultColumnFamily();
    const Snapshot *snapshot = nullptr;
    for (int i = 0; i < kN; ++i) {
        std::string key = base::Slice::Sprintf("key.%d", i);
        std::string val = base::Slice::Sprintf("val.%d", i);
        if (i == kN / 2) {
            snapshot = impl->GetSnapshot();
        }
        Error rs = impl->Put(WriteOptions{}, cf0, key, val);
    }
    ASSERT_NE(nullptr, snapshot);
    
    ReadOptions rd;
    rd.snapshot = snapshot;
    std::string value;
    for (int i = 0; i < kN; ++i) {
        std::string key = base::Slice::Sprintf("key.%d", i);
        std::string val = base::Slice::Sprintf("val.%d", i);
        
        Error rs = impl->Get(rd, cf0, key, &value);
        if (i <= kN / 2) {
            ASSERT_TRUE(rs.ok()) << rs.ToString();
            ASSERT_EQ(val, value);
        } else {
            ASSERT_TRUE(rs.fail());
            ASSERT_TRUE(rs.IsNotFound());
        }
    }
}
    
TEST_F(DBImplTest, TwoCFWriting) {
    std::vector<ColumnFamilyDescriptor> descs;
    ColumnFamilyDescriptor desc;
    desc.name = kDefaultColumnFamilyName;
    //desc.options.use_unordered_table = true;
    descs.push_back(desc);
    desc.name = "cf1";
    //desc.options.use_unordered_table = true;
    descs.push_back(desc);
    
    Options options;
    options.create_if_missing = true;
    
    static const auto kN = 1024 * 100 ;
    
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[16], options_));
    ColumnFamilyCollection scope(impl.get());
    auto rs = impl->Open(descs, scope.ReceiveAll());
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    auto jiffies = env_->CurrentTimeMicros();
    auto cf0 = scope.GetOrNull(kDefaultColumnFamilyName);
    auto cf1 = scope.GetOrNull("cf1");
    WriteBatch batch;
    for (int i = 0; i < kN; ++i) {
        std::string key,val;
        if (i < kN / 2) {
            key = base::Slice::Sprintf("cf0.key.%d", i);
            val = base::Slice::Sprintf("cf0.val.%d", i);

            batch.Put(cf0, key, val);
        }

        key = base::Slice::Sprintf("cf1.key.%d", i);
        val = base::Slice::Sprintf("cf1.val.%d", i);
        
        batch.Put(cf1, key, val);
        
        if (batch.redo().size() > 10 * base::kMB) {
            rs = impl->Write(WriteOptions{}, &batch);
            ASSERT_TRUE(rs.ok()) << rs.ToString();
            batch.Clear();
        }
    }
    if (batch.redo().size() > 0) {
        rs = impl->Write(WriteOptions{}, &batch);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
    }
    printf("Putting cost: %f ms\n",
           (env_->CurrentTimeMicros() - jiffies) / 1000.0f);
    
    //----------------Recovery--------------------------------------------------
    
    scope.ReleaseAll();
    impl.reset(new DBImpl(tmp_dirs[16], options));
    scope.Reset(impl.get());
    
    rs = impl->Open(descs, scope.ReceiveAll());
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    jiffies = env_->CurrentTimeMicros();
    cf0 = scope.GetOrNull(kDefaultColumnFamilyName);
    cf1 = scope.GetOrNull("cf1");
    std::string value;
    for (int i = 0; i < kN; ++i) {
        std::string key,val;
        if (i < kN / 2) {
            key = base::Slice::Sprintf("cf0.key.%d", i);
            val = base::Slice::Sprintf("cf0.val.%d", i);
            
            rs = impl->Get(ReadOptions{}, cf0, key, &value);
            ASSERT_TRUE(rs.ok()) << rs.ToString();
            ASSERT_EQ(val, value);
        }
        
        key = base::Slice::Sprintf("cf1.key.%d", i);
        val = base::Slice::Sprintf("cf1.val.%d", i);
        
        rs = impl->Get(ReadOptions{}, cf1, key, &value);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        ASSERT_EQ(val, value);
    }
    printf("getting cost: %f ms\n",
           (env_->CurrentTimeMicros() - jiffies) / 1000.0f);
}
    
TEST_F(DBImplTest, ConcurrentGetting) {
    static const auto kN = 1024 * 102;
    
    std::vector<ColumnFamilyDescriptor> descs;
    ColumnFamilyDescriptor desc;
    desc.name = kDefaultColumnFamilyName;
    //desc.options.use_unordered_table = true;
    descs.push_back(desc);
    //desc.name = "cf1";
    //desc.options.use_unordered_table = true;
    //descs.push_back(desc);
    
    Options options;
    options.create_if_missing = true;
    
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[17], options_));
    ColumnFamilyCollection scope(impl.get());
    auto rs = impl->Open(descs, scope.ReceiveAll());
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    std::string val(10240, 'F');
    auto cf0 = impl->DefaultColumnFamily();
    std::atomic<int> current_n(0);
    std::thread wr_thrd([&] () {
        for (int i = 0; i < kN; ++i) {
            std::string key = base::Slice::Sprintf("key.%d", i);

            Error rs = impl->Put(WriteOptions{}, cf0, key, val);
            ASSERT_TRUE(rs.ok()) << rs.ToString();
            
            current_n.store(i, std::memory_order_release);
        }
    });
    
    std::string value;
    while (true) {
        auto n = current_n.load(std::memory_order_acquire);
        if (n == 0) {
            continue;
        }
        
        std::string key = base::Slice::Sprintf("key.%d", n);
        Error rs = impl->Get(ReadOptions{}, cf0, key, &value);
        EXPECT_TRUE(rs.ok()) << rs.ToString() << ":" << key;

        for (int i = 0; i < 10; i++) {
            key = base::Slice::Sprintf("key.%d", rand() % n);
            rs = impl->Get(ReadOptions{}, cf0, key, &value);
            EXPECT_TRUE(rs.ok()) << rs.ToString() << ":" << key;
        }
        if (n >= kN - 1) {
            break;
        }
    }
    wr_thrd.join();
    PrintFiles(impl.get(), cf0);
}
    
TEST_F(DBImplTest, GetProperties) {
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[18], options_));
    ColumnFamilyCollection scope(impl.get());
    auto rs = impl->Open(descs_, scope.ReceiveAll());
    ASSERT_TRUE(rs.ok()) << rs.ToString();

    std::string value;
    rs = impl->GetProperty("db.cf.default.levels", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = impl->GetProperty("db.log.total-size", &value);
    ASSERT_EQ("0", value);
}
    
} // namespace db
    
} // namespace mai
