#ifndef MAI_TXN_PESSIMISTIC_TRANSACTION_DB_H_
#define MAI_TXN_PESSIMISTIC_TRANSACTION_DB_H_

#include "txn/transaction-lock-mgr.h"
#include "txn/transaction-base.h"
#include "base/base.h"
#include "mai/transaction.h"
#include "mai/transaction-db.h"
#include <atomic>
#include <mutex>
#include <unordered_map>

namespace mai {
    
namespace txn {
    
class PessimisticTransaction;

class PessimisticTransactionDB final : public TransactionDB {
public:
    PessimisticTransactionDB(const TransactionDBOptions &opts, DB *db);
    virtual ~PessimisticTransactionDB() override;
    
    DEF_VAL_GETTER(TransactionDBOptions, options);
    
    virtual Error NewColumnFamily(const std::string &name,
                                  const ColumnFamilyOptions &options,
                                  ColumnFamily **result) override;
    virtual Error DropColumnFamily(ColumnFamily *cf) override;
    virtual Error Put(const WriteOptions &opts, ColumnFamily *cf,
                      std::string_view key, std::string_view value) override;
    virtual Error Delete(const WriteOptions &opts, ColumnFamily *cf,
                         std::string_view key) override;
    virtual Error Write(const WriteOptions& opts, WriteBatch* updates) override;
    virtual Transaction *BeginTransaction(const WriteOptions &wr_opts,
                                          const TransactionOptions &txn_opts,
                                          Transaction *old_txn) override;
    
    Transaction *GetTransactionByName(const std::string &name);
    
    Error TryLock(PessimisticTransaction *txn, uint32_t cfid,
                  const std::string &hold_key,
                  bool exclusive);
    void UnLock(PessimisticTransaction *txn,
                const TxnKeyMaps *tracked_keys);
    void InsertExpirableTransaction(TxnID id, PessimisticTransaction *txn);
    void RemoveExpirableTransaction(TxnID id);
    void RegisterTransaction(Transaction *txn);
    void UnregisterTransaction(Transaction *txn);
    bool TryStealingExpiredTransactionLocks(TxnID id);
    
    TxnID GenerateTxnID() { return next_txn_id_.fetch_add(1); }
    
    void AddColumnFamily(uint32_t cfid) {
        lock_mgr_.AddColumnFamily(cfid);
    }

    DISALLOW_IMPLICIT_CONSTRUCTORS(PessimisticTransactionDB);
private:
    PessimisticTransaction *NewAutoTransaction(const WriteOptions &wr_opts);
    
    TransactionDBOptions const options_;
    TransactionLockMgr lock_mgr_;
    std::atomic<TxnID> next_txn_id_;
    std::mutex cf_mutex_;
    std::mutex map_mutex_;
    std::unordered_map<TxnID, PessimisticTransaction *> expirable_txns_;
    std::mutex name_mutex_;
    std::unordered_map<std::string, Transaction *> txns_;
}; // class PessimismTransactionDB

} // namespace txn
    
} // namespace mai


#endif // MAI_TXN_PESSIMISTIC_TRANSACTION_DB_H_
