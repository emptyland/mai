#include "txn/pessimistic-transaction-db.h"
#include "txn/pessimistic-transaction.h"

namespace mai {
    
namespace txn {

PessimisticTransactionDB::PessimisticTransactionDB(const TransactionDBOptions &opts,
                                                   DB *db)
    
    : TransactionDB(db)
    , options_(opts)
    , lock_mgr_(this, opts.num_stripes, opts.max_num_locks,
                opts.max_num_deadlocks)
    , next_txn_id_(1) { // Initialize txn id
}
    
/*virtual*/ PessimisticTransactionDB::~PessimisticTransactionDB() {
    DCHECK(txns_.empty());
    DCHECK(expirable_txns_.empty());
}
    
/*virtual*/ Error
PessimisticTransactionDB::NewColumnFamily(const std::string &name,
                                          const ColumnFamilyOptions &options,
                                          ColumnFamily **result) {
    std::lock_guard<std::mutex> lock(cf_mutex_);
    
    Error rs = GetDB()->NewColumnFamily(name, options, result);
    if (!rs) {
        return rs;
    }
    
    lock_mgr_.AddColumnFamily((*result)->id());
    return rs;
}
    
/*virtual*/ Error PessimisticTransactionDB::DropColumnFamily(ColumnFamily *cf) {
    std::lock_guard<std::mutex> lock(cf_mutex_);

    Error rs = GetDB()->DropColumnFamily(cf);
    if (!rs) {
        return rs;
    }
    lock_mgr_.RemoveColumnFamily(cf->id());
    return rs;
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
    // TODO:
    return MAI_NOT_SUPPORTED("TODO:");
}

/*virtual*/ Transaction *
PessimisticTransactionDB::BeginTransaction(const WriteOptions &wr_opts,
                                         const TransactionOptions &txn_opts,
                                         Transaction *old_txn) {
    if (old_txn) {
        down_cast<PessimisticTransaction>(old_txn)->Reinitialize(txn_opts,
                                                                 wr_opts, this);
        return old_txn;
    }
    return new PessimisticTransaction(txn_opts, wr_opts, this);
}
    
Transaction *PessimisticTransactionDB::GetTransactionByName(const std::string &name) {
    std::lock_guard<std::mutex> lock(name_mutex_);
    auto iter = txns_.find(name);
    return iter == txns_.end() ? nullptr : iter->second;
}

Error
PessimisticTransactionDB::TryLock(PessimisticTransaction *txn, uint32_t cfid,
                                  const std::string &hold_key, bool exclusive) {
    return lock_mgr_.TryLock(txn, cfid, hold_key, exclusive);
}
    
void
PessimisticTransactionDB::InsertExpirableTransaction(TxnID id,
                                                     PessimisticTransaction *txn) {
    std::lock_guard<std::mutex> lock(map_mutex_);
    DCHECK_GT(txn->expiration_time(), 0);
    expirable_txns_.insert({id, txn});
}
    
void PessimisticTransactionDB::RemoveExpirableTransaction(TxnID id) {
    std::lock_guard<std::mutex> lock(map_mutex_);
    expirable_txns_.erase(id);
}
    
void PessimisticTransactionDB::RegisterTransaction(Transaction *txn) {
    DCHECK_GT(DCHECK_NOTNULL(txn)->name().size(), 0);
    DCHECK(GetTransactionByName(txn->name()) == nullptr);
    DCHECK_EQ(Transaction::STARTED, txn->state());
    
    std::lock_guard<std::mutex> lock(name_mutex_);
    txns_.insert({txn->name(), txn});
}
    
void PessimisticTransactionDB::UnregisterTransaction(Transaction *txn) {
    DCHECK_NOTNULL(txn);
    std::lock_guard<std::mutex> lock(name_mutex_);
    auto iter = txns_.find(txn->name());
    DCHECK(iter != txns_.end());
    txns_.erase(iter);
}
    
bool PessimisticTransactionDB::TryStealingExpiredTransactionLocks(TxnID id) {
    std::lock_guard<std::mutex> lock(map_mutex_);
    auto iter = expirable_txns_.find(id);
    if (iter == expirable_txns_.end()) {
        return true;
    }
    return iter->second->TryStealingLocks();
}
    
void
PessimisticTransactionDB::UnLock(PessimisticTransaction *txn,
                                 const std::unordered_map<uint32_t, TxnKeyMap> *tracked_keys) {
    lock_mgr_.Unlock(txn, tracked_keys);
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
