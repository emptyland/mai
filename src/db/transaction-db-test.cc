#include "mai/transaction-db.h"
#include "mai/transaction.h"
#include "gtest/gtest.h"

namespace mai {

class TransactionDBTest : public ::testing::Test {
public:
    TransactionDBTest() {
        ColumnFamilyDescriptor desc;
        desc.name = kDefaultColumnFamilyName;
        descs_.push_back(desc);
        options_.create_if_missing = true;
    }
    
    ~TransactionDBTest() override {
        int i = 0;
        while (tmp_dirs[i]) {
            env_->DeleteFile(tmp_dirs[i++], true);
        }
    }
    
    Env *env_ = Env::Default();
    std::vector<ColumnFamilyDescriptor> descs_;
    Options options_;
    TransactionDBOptions txn_db_opts_;
    static const char *tmp_dirs[];
};


const char *TransactionDBTest::tmp_dirs[] = {
    "tests/00-txn-db-sanity",
    "tests/01-txn-db-put-without-txn",
    "tests/02-txn-db-optimism-put-with-txn",
    "tests/03-txn-db-optimism-write-conflict",
    nullptr,
};
    
TEST_F(TransactionDBTest, Sanity) {
    std::unique_ptr<TransactionDB> db;
    TransactionDB *result = nullptr;
    Error rs = TransactionDB::Open(options_, txn_db_opts_, tmp_dirs[0], {},
                                   nullptr, &result);
    db.reset(result);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    WriteOptions wr_opts;
    wr_opts.sync = true;
    std::unique_ptr<Transaction> txn(db->BeginTransaction(wr_opts));
    
    ASSERT_NE(nullptr, txn.get());
}
    
TEST_F(TransactionDBTest, PutWithoutTxn) {
    std::unique_ptr<TransactionDB> db;
    TransactionDB *result = nullptr;
    Error rs = TransactionDB::Open(options_, txn_db_opts_, tmp_dirs[1], {},
                                   nullptr, &result);
    db.reset(result);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    auto cf0 = db->DefaultColumnFamily();
    ASSERT_NE(nullptr, cf0);
    ASSERT_EQ(cf0->name(), kDefaultColumnFamilyName);
    
    WriteOptions wr_opts;
    db->Put(wr_opts, cf0, "aaaa", "v1");
    db->Put(wr_opts, cf0, "aaab", "v2");
    db->Put(wr_opts, cf0, "aaac", "v3");
    db->Put(wr_opts, cf0, "aaad", "v4");
    db->Put(wr_opts, cf0, "aaae", "v5");
    
    ReadOptions rd_opts;
    std::string value;
    rs = db->Get(rd_opts, cf0, "aaaa", &value);
    ASSERT_TRUE(rs.ok());
    ASSERT_EQ("v1", value);
    
    rs = db->Get(rd_opts, cf0, "aaad", &value);
    ASSERT_TRUE(rs.ok());
    ASSERT_EQ("v4", value);
}
    
TEST_F(TransactionDBTest, OptimismPutWithTxn) {
    std::unique_ptr<TransactionDB> db;
    TransactionDB *result = nullptr;
    Error rs = TransactionDB::Open(options_, txn_db_opts_, tmp_dirs[1], {},
                                   nullptr, &result);
    db.reset(result);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    auto cf0 = db->DefaultColumnFamily();
    
    WriteOptions wr_opts;
    wr_opts.sync = true;
    std::unique_ptr<Transaction> txn(db->BeginTransaction(wr_opts));
    
    txn->Put(cf0, "aaaa", "v1");
    txn->Put(cf0, "aaab", "v2");
    txn->Put(cf0, "aaac", "v3");
    
    std::string value;
    ReadOptions rd_opts;
    rs = db->Get(rd_opts, cf0, "aaaa", &value);
    ASSERT_TRUE(rs.fail());
    ASSERT_TRUE(rs.IsNotFound());
    
    rs = txn->Commit();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = db->Get(rd_opts, cf0, "aaaa", &value);
    ASSERT_TRUE(rs.ok());
    ASSERT_EQ("v1", value);
    
    rs = db->Get(rd_opts, cf0, "aaab", &value);
    ASSERT_TRUE(rs.ok());
    ASSERT_EQ("v2", value);
}
    
TEST_F(TransactionDBTest, OptimismWriteConflict) {
    std::unique_ptr<TransactionDB> db;
    TransactionDB *result = nullptr;
    Error rs = TransactionDB::Open(options_, txn_db_opts_, tmp_dirs[2], {},
                                   nullptr, &result);
    db.reset(result);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    auto cf0 = db->DefaultColumnFamily();
    
    WriteOptions wr_opts;
    wr_opts.sync = true;
    std::unique_ptr<Transaction> txn(db->BeginTransaction(wr_opts));
    
    txn->Put(cf0, "aaaa", "v1");
    txn->Put(cf0, "aaab", "v2");
    txn->Put(cf0, "aaac", "v3");
    
    db->Put(wr_opts, cf0, "aaaa", "vvv");
    
    rs = txn->Commit();
    ASSERT_TRUE(rs.fail());
    ASSERT_TRUE(rs.IsBusy());
    
    txn->Rollback();
    txn->Put(cf0, "aaaa", "xxx");
    
    rs = txn->Commit();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    std::string value;
    rs = db->Get(ReadOptions{}, cf0, "aaaa", &value);
    ASSERT_TRUE(rs.ok());
    ASSERT_EQ("xxx", value);
}
    
TEST_F(TransactionDBTest, OptimismReuse) {
    std::unique_ptr<TransactionDB> db;
    TransactionDB *result = nullptr;
    Error rs = TransactionDB::Open(options_, txn_db_opts_, tmp_dirs[2], {},
                                   nullptr, &result);
    db.reset(result);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    auto cf0 = db->DefaultColumnFamily();
    
    WriteOptions wr_opts;
    wr_opts.sync = true;
    std::unique_ptr<Transaction> txn(db->BeginTransaction(wr_opts));
    
    txn->Put(cf0, "aaaa", "v1");
    txn->Commit();
    
    TransactionOptions txn_opts;
    db->BeginTransaction(wr_opts, txn_opts, txn.get());
    
    txn->Put(cf0, "aaab", "v2");
    txn->Put(cf0, "aaac", "v3");
    txn->Commit();
    
    std::string value;
    rs = db->Get(ReadOptions{}, cf0, "aaaa", &value);
    ASSERT_TRUE(rs.ok());
    ASSERT_EQ("v1", value);
    
    rs = db->Get(ReadOptions{}, cf0, "aaab", &value);
    ASSERT_TRUE(rs.ok());
    ASSERT_EQ("v2", value);
    
    rs = db->Get(ReadOptions{}, cf0, "aaac", &value);
    ASSERT_TRUE(rs.ok());
    ASSERT_EQ("v3", value);
}
    
TEST_F(TransactionDBTest, TransactionGet) {
    std::unique_ptr<TransactionDB> db;
    TransactionDB *result = nullptr;
    Error rs = TransactionDB::Open(options_, txn_db_opts_, tmp_dirs[2], {},
                                   nullptr, &result);
    db.reset(result);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    auto cf0 = db->DefaultColumnFamily();
    
    WriteOptions wr_opts;
    wr_opts.sync = true;
    std::unique_ptr<Transaction> txn(db->BeginTransaction(wr_opts));
    
    db->Put(wr_opts, cf0, "aaaa", "v1");
    db->Put(wr_opts, cf0, "aaab", "v2");
    
    txn->Put(cf0, "aaaa", "vvv");
    txn->Delete(cf0, "aaab");
    
    ReadOptions rd_opts;
    std::string value;
    rs = txn->Get(rd_opts, cf0, "aaaa", &value);
    ASSERT_TRUE(rs.ok());
    ASSERT_EQ("vvv", value);
    
    rs = txn->Get(rd_opts, cf0, "aaab", &value);
    ASSERT_TRUE(rs.IsNotFound());
    
    rs = txn->Commit();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
}

}
