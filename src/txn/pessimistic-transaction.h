#ifndef MAI_TXN_PESSIMISTIC_TRANSACTION_H_
#define MAI_TXN_PESSIMISTIC_TRANSACTION_H_

#include "txn/transaction-base.h"
#include "txn/pessimistic-transaction-db.h"
#include "base/base.h"
#include <mutex>

namespace mai {

namespace txn {
    
class PessimisticTransaction final : public TransactionBase {
public:
    PessimisticTransaction(const TransactionOptions &options,
                           const WriteOptions &wr_opts,
                           PessimisticTransactionDB *db);
    virtual ~PessimisticTransaction() override;
    
    PessimisticTransactionDB *owns() const {
        return down_cast<PessimisticTransactionDB>(db());
    }
    
    DEF_VAL_PROP_RW(int64_t, lock_timeout);
    DEF_VAL_GETTER(uint64_t, expiration_time);
    
    void Initialize(const TransactionOptions &options);
    void Reinitialize(const TransactionOptions &options,
                      const WriteOptions &wr_opts, TransactionDB *db);
    
    bool IsExpired() const;
    virtual void Clear() override;
    
    virtual std::string name() override { return name_; }
    virtual TxnID id() const override { return txn_id_; }
    virtual Error SetName(const std::string &name) override;
    
    virtual Error Rollback() override;
    virtual Error Commit() override;
    virtual Error TryLock(ColumnFamily* cf, std::string_view key, bool read_only,
                          bool exclusive, const bool do_validate) override;
    
    bool TryStealingLocks() {
        DCHECK(IsExpired());
        return update_state(STARTED, LOCKS_STOLEN);
    }
    
    Error CommitBatch(WriteBatch *updates);
    Error LockBatch(WriteBatch *updates, TxnKeyMaps *keys_to_unlock);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(PessimisticTransaction);
private:
    TxnID txn_id_ = 0;
    std::string name_;
    std::vector<TxnID> waiting_txn_ids_;
    mutable std::mutex mutex_;
    int64_t lock_timeout_ = -1;
    uint64_t expiration_time_ = 0;
    bool deadlock_detect_ = false;
    int64_t deadlock_detect_depth_ = 50;
}; // class PessimisticTransaction
    
} // namespace txn
    
} // namespace mai

#endif // MAI_TXN_PESSIMISTIC_TRANSACTION_H_

