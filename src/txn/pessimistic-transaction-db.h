#ifndef MAI_TXN_PESSIMISTIC_TRANSACTION_DB_H_
#define MAI_TXN_PESSIMISTIC_TRANSACTION_DB_H_

#include "txn/transaction-base.h"
#include "base/base.h"
#include "mai/transaction.h"
#include "mai/transaction-db.h"
#include <atomic>

namespace mai {
    
namespace txn {
    
class PessimisticTransaction;

class PessimisticTransactionDB final : public TransactionDB {
public:
    PessimisticTransactionDB(const TransactionDBOptions &opts, DB *db);
    virtual ~PessimisticTransactionDB() override;
    
    DEF_VAL_GETTER(TransactionDBOptions, options);
    
    virtual Error Put(const WriteOptions &opts, ColumnFamily *cf,
                      std::string_view key, std::string_view value) override;
    virtual Error Delete(const WriteOptions &opts, ColumnFamily *cf,
                         std::string_view key) override;
    virtual Error Write(const WriteOptions& opts, WriteBatch* updates) override;
    virtual Transaction *BeginTransaction(const WriteOptions &wr_opts,
                                          const TransactionOptions &txn_opts,
                                          Transaction *old_txn) override;
    
    Transaction *GetTransactionByName(const std::string &name);
    
    Error TryLock(Transaction *txn, uint32_t cfid, const std::string &hold_key,
                  bool exclusive);
    void InsertExpirableTransaction(TxnID id, Transaction *txn);
    void RemoveExpirableTransaction(TxnID id);
    void RegisterTransaction(Transaction *txn);
    void UnregisterTransaction(Transaction *txn);
    void UnLock(Transaction *txn,
                const std::unordered_map<uint32_t, TxnKeyMap> *tracked_keys);
    
    TxnID GenerateTxnID() { return next_txn_id_.fetch_add(1); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(PessimisticTransactionDB);
private:
    PessimisticTransaction *NewAutoTransaction(const WriteOptions &wr_opts);
    
    TransactionDBOptions const options_;
    //TransactionOptions const txn_opts_;
    std::atomic<TxnID> next_txn_id_;
}; // class PessimismTransactionDB

} // namespace txn
    
} // namespace mai


#endif // MAI_TXN_PESSIMISTIC_TRANSACTION_DB_H_
