#include "txn/pessimistic-transaction-db.h"
#include "txn/pessimistic-transaction.h"
#include "mai/transaction.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace txn {
    
class PessimisticTransactionDBTest : public ::testing::Test {
public:
    PessimisticTransactionDBTest() {
        ColumnFamilyDescriptor desc;
        desc.name = kDefaultColumnFamilyName;
        descs_.push_back(desc);
        desc.name = "unordered";
        desc.options.use_unordered_table = true;
        descs_.push_back(desc);
        desc.name = "ordered";
        descs_.push_back(desc);

        txn_db_opts_.optimism = false;
        options_.create_if_missing = true;
    }
    
    ~PessimisticTransactionDBTest() override {
        if (txn_db_) {
            for (auto cf : cfs_) {
                txn_db_->ReleaseColumnFamily(cf);
            }
            delete txn_db_;
        }
        
        int i = 0;
        while (tmp_dirs[i]) {
            env_->DeleteFile(tmp_dirs[i++], true);
        }
    }
    
    void Open(int idx) {
        TransactionDB *db = nullptr;
        auto rs = TransactionDB::Open(options_, txn_db_opts_, tmp_dirs[idx],
                                      descs_, &cfs_, &db);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        txn_db_ = down_cast<PessimisticTransactionDB>(db);
    }
    
    Env *env_ = Env::Default();
    PessimisticTransactionDB *txn_db_ = nullptr;
    std::vector<ColumnFamily *> cfs_;
private:
    std::vector<ColumnFamilyDescriptor> descs_;
    Options options_;
    TransactionDBOptions txn_db_opts_;
    static const char *tmp_dirs[];
};


const char *PessimisticTransactionDBTest::tmp_dirs[] = {
    "tests/00-pessimistic-txn-db-sanity",
    "tests/01-pessimistic-txn-db-put",
    "tests/02-pessimistic-txn-db-put-wait",
    "tests/03-pessimistic-txn-db-put-commit",
    "tests/04-pessimistic-txn-db-write",
    "tests/05-pessimistic-txn-db-write-wait",
    nullptr,
};
    
TEST_F(PessimisticTransactionDBTest, Sanity) {
    Open(0);
    ASSERT_NE(nullptr, txn_db_);
    
    WriteOptions wr_opts;
    TransactionOptions txn_opts;
    std::unique_ptr<PessimisticTransaction>
        txn(down_cast<PessimisticTransaction>(txn_db_->BeginTransaction(wr_opts, txn_opts, nullptr)));
    
    ASSERT_NE(nullptr, txn.get());
    
    txn->SetName("hello");
    ASSERT_EQ("hello", txn->name());
    ASSERT_EQ(1, txn->id());
    
    auto found = txn_db_->GetTransactionByName("hello");
    ASSERT_NE(nullptr, found);
    ASSERT_EQ(txn.get(), found);
}

TEST_F(PessimisticTransactionDBTest, Put) {
    Open(1);
    
    Error rs;
    auto cf = cfs_[0];
    
    WriteOptions wr_opts;
    rs = txn_db_->Put(wr_opts, cf, "aaaa", "cccc");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    TransactionOptions txn_opts;
    std::unique_ptr<Transaction> txn(txn_db_->BeginTransaction(wr_opts, txn_opts, nullptr));

    rs = txn->Put(cf, "aaaa", "bbbb");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = txn->Commit();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    ReadOptions rd_opts;
    std::string value;
    rs = txn_db_->Get(rd_opts, cf, "aaaa", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    EXPECT_EQ("bbbb", value);
}
    
TEST_F(PessimisticTransactionDBTest, PutWait) {
    Open(2);
    
    WriteOptions wr_opts;
    TransactionOptions txn_opts;
    txn_opts.lock_timeout = 1000; // 1s
    std::unique_ptr<Transaction> txn1(txn_db_->BeginTransaction(wr_opts, txn_opts, nullptr));
    std::unique_ptr<Transaction> txn2(txn_db_->BeginTransaction(wr_opts, txn_opts, nullptr));
    
    Error rs;
    auto cf = cfs_[0];
    
    rs = txn1->Put(cf, "aaaa", "bbbb");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = txn2->Put(cf, "aaaa", "cccc");
    ASSERT_TRUE(rs.fail());
    EXPECT_TRUE(rs.IsTimeout());
}
    
TEST_F(PessimisticTransactionDBTest, PutCommit) {
    Open(3);
    
    WriteOptions wr_opts;
    wr_opts.sync = true;
    TransactionOptions txn_opts;
    std::unique_ptr<Transaction> txn(txn_db_->BeginTransaction(wr_opts, txn_opts, nullptr));

    auto cf = cfs_[0];
    auto rs = txn->Put(cf, "aaaa", "1111");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = txn->Put(cf, "bbbb", "2222");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = txn->Put(cf, "aaaa", "0000");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = txn->Put(cf, "cccc", "3333");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = txn->Commit();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    ReadOptions rd_opts;
    std::string value;
    rs = txn_db_->Get(rd_opts, cf, "aaaa", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    EXPECT_EQ("0000", value);
}
    
TEST_F(PessimisticTransactionDBTest, WriteCommit) {
    Open(4);
    
    WriteOptions wr_opts;
    wr_opts.sync = true;
    TransactionOptions txn_opts;
    std::unique_ptr<Transaction> txn(txn_db_->BeginTransaction(wr_opts, txn_opts, nullptr));
    
    auto cf = cfs_[0];
    auto rs = txn->Put(cf, "aaaa", "0000");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = txn->Put(cf, "bbbb", "1111");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = txn->Put(cf, "cccc", "2222");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = txn->Commit();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    WriteBatch updates;
    updates.Put(cf, "bbbb", "----");
    updates.Delete(cf, "cccc");
    
    rs = txn_db_->Write(wr_opts, &updates);
    ASSERT_TRUE(rs.ok()) << rs.ToString();

    ReadOptions rd_opts;
    std::string value;
    rs = txn_db_->Get(rd_opts, cf, "bbbb", &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    EXPECT_EQ("----", value);
    
    rs = txn_db_->Get(rd_opts, cf, "cccc", &value);
    ASSERT_TRUE(rs.IsNotFound()) << rs.ToString();
}
    
} // namespace txn

} // namespace mai
