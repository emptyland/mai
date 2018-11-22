#include "db/db-impl.h"
#include "mai/env.h"
#include "gtest/gtest.h"
#include <vector>

namespace mai {
    
namespace db {
    
class DBImplTest : public ::testing::Test {
public:
    DBImplTest() {
        ColumnFamilyDescriptor desc;
        desc.name = kDefaultColumnFamilyName;
        descs_.push_back(desc);
    }
    
    void TearDown() override {
        int i = 0;
        while (tmp_dirs[i]) {
            env_->DeleteFile(tmp_dirs[i++], true);
            //ASSERT_TRUE(rs.ok()) << rs.ToString();
        }
    }
    
    Env *env_ = Env::Default();
    std::vector<ColumnFamilyDescriptor> descs_;
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
    nullptr,
};
    
TEST_F(DBImplTest, Sanity) {
    std::vector<ColumnFamily *> cfs;
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[0], env_));
    
    Options opts;
    opts.create_if_missing = true;
    Error rs = impl->Open(opts, descs_, &cfs);
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
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[1], env_));
    
    Options opts;
    opts.create_if_missing = true;
    Error rs = impl->Open(opts, descs_, &cfs);
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
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[2], env_));
    
    Options opts;
    opts.create_if_missing = true;
    Error rs = impl->Open(opts, descs_, &cfs);
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
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[3], env_));
    
    Options opts;
    opts.create_if_missing = true;
    Error rs = impl->Open(opts, descs_, &cfs);
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
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[4], env_));
    
    Options opts;
    opts.create_if_missing = true;
    Error rs = impl->Open(opts, descs_, &cfs);
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
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[5], env_));
    
    Options opts;
    opts.create_if_missing = true;
    Error rs = impl->Open(opts, descs_, &cfs);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    for (auto cf : cfs) {
        impl->ReleaseColumnFamily(cf);
    }
    
    impl.reset(new DBImpl(tmp_dirs[5], env_));
    rs = impl->Open(opts, descs_, &cfs);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ(1, cfs.size());
    ASSERT_EQ(0, cfs[0]->id());
    ASSERT_EQ(kDefaultColumnFamilyName, cfs[0]->name());
    
    for (auto cf : cfs) {
        impl->ReleaseColumnFamily(cf);
    }
}
    
TEST_F(DBImplTest, RecoveryData) {
    std::vector<ColumnFamily *> cfs;
    std::unique_ptr<DBImpl> impl(new DBImpl(tmp_dirs[5], env_));
    
    Options opts;
    opts.create_if_missing = true;
    Error rs = impl->Open(opts, descs_, &cfs);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    auto cf0 = cfs[0];
    
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
    
    for (auto cf : cfs) {
        impl->ReleaseColumnFamily(cf);
    }
    
    impl.reset(new DBImpl(tmp_dirs[5], env_));
    rs = impl->Open(opts, descs_, &cfs);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    cf0 = cfs[0];
    
    std::string value;
    ReadOptions rd_opts;
    rs = impl->Get(rd_opts, cf0, "aaaa", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("100", value);
    rs = impl->Get(rd_opts, cf0, "eeee", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("500", value);
    
    for (auto cf : cfs) {
        impl->ReleaseColumnFamily(cf);
    }
}
    
} // namespace db
    
} // namespace mai
