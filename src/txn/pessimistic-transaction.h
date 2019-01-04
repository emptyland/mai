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
    DEF_VAL_GETTER(bool, deadlock_detect);
    DEF_VAL_GETTER(int64_t, deadlock_detect_depth)
    
    std::vector<TxnID> GetWaitingTxn(uint32_t *cfid, std::string *key) {
        std::lock_guard<std::mutex> lock(wait_mutex_);
        *cfid = waiting_cfid_;
        *key  = waiting_key_ ? *waiting_key_ : "";
        return waiting_txn_ids_;
    }
    
    void SetWaitingTxn(const std::vector<TxnID> &ids, uint32_t cfid,
                       const std::string *key) {
        std::lock_guard<std::mutex> lock(wait_mutex_);
        waiting_txn_ids_ = ids;
        waiting_cfid_    = cfid;
        waiting_key_     = key;
    }
    
    void ClearWaitingTxn() {
        std::lock_guard<std::mutex> lock(wait_mutex_);
        waiting_txn_ids_.clear();
        waiting_cfid_ = 0;
        waiting_key_  = nullptr;
    }
    
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
    uint32_t waiting_cfid_ = 0;
    const std::string *waiting_key_ = nullptr;
    std::mutex wait_mutex_;
    mutable std::mutex mutex_;
    int64_t lock_timeout_ = -1;
    uint64_t expiration_time_ = 0;
    bool deadlock_detect_ = false;
    int64_t deadlock_detect_depth_ = 50;
}; // class PessimisticTransaction
    
} // namespace txn
    
} // namespace mai

#endif // MAI_TXN_PESSIMISTIC_TRANSACTION_H_

