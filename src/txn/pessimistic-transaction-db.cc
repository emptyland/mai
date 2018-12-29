#include "txn/pessimistic-transaction-db.h"
#include "txn/pessimistic-transaction.h"

namespace mai {
    
namespace txn {
    
PessimisticTransactionDB::PessimisticTransactionDB(const TransactionDBOptions &opts,
                                                   DB *db)
    
    : TransactionDB(db)
    , options_(opts)
    , next_txn_id_(0) {
}
    
/*virtual*/ PessimisticTransactionDB::~PessimisticTransactionDB() {
}

/*virtual*/ Error
PessimisticTransactionDB::Put(const WriteOptions &opts, ColumnFamily *cf,
                            std::string_view key, std::string_view value) {
    PessimisticTransaction *txn = NewAutoTransaction(opts);
    auto rs = txn->PutUntracked(cf, key, value);
    if (rs.ok()) {
        rs = txn->Commit();
    }
    return rs;
}

/*virtual*/ Error
PessimisticTransactionDB::Delete(const WriteOptions &opts, ColumnFamily *cf,
                     std::string_view key) {
    PessimisticTransaction *txn = NewAutoTransaction(opts);
    auto rs = txn->DeleteUntracked(cf, key);
    if (rs.ok()) {
        rs = txn->Commit();
    }
    return rs;
}

/*virtual*/ Error
PessimisticTransactionDB::Write(const WriteOptions& opts, WriteBatch* updates) {
    return TransactionDB::Write(opts, updates);
}

/*virtual*/ Transaction *
PessimisticTransactionDB::BeginTransaction(const WriteOptions &wr_opts,
                                         const TransactionOptions &txn_opts,
                                         Transaction *old_txn) {
    return nullptr;
}
    
Transaction *PessimisticTransactionDB::GetTransactionByName(const std::string &name) {
    return nullptr;
}

Error
PessimisticTransactionDB::TryLock(Transaction *txn, uint32_t cfid,
                                  const std::string &hold_key, bool exclusive) {
    return MAI_NOT_SUPPORTED("TODO:");
}
    
void PessimisticTransactionDB::InsertExpirableTransaction(TxnID id,
                                                          Transaction *txn) {
    
}
    
void PessimisticTransactionDB::RemoveExpirableTransaction(TxnID id) {
    
}
    
void PessimisticTransactionDB::RegisterTransaction(Transaction *txn) {
    
}
    
void PessimisticTransactionDB::UnregisterTransaction(Transaction *txn) {
    
}
    
void PessimisticTransactionDB::UnLock(Transaction *txn,
                                      const std::unordered_map<uint32_t, TxnKeyMap> *tracked_keys) {

}
    
PessimisticTransaction *
PessimisticTransactionDB::NewAutoTransaction(const WriteOptions &wr_opts) {
    Transaction *x = BeginTransaction(wr_opts, TransactionOptions{}, nullptr);
    PessimisticTransaction *txn = down_cast<PessimisticTransaction>(x);
    txn->set_lock_timeout(options_.default_lock_timeout);
    return txn;
}
    
} // namespace txn
    
} // namespace mai
