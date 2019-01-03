#ifndef MAI_TXN_TRANSACTION_LOCK_MGR_H_
#define MAI_TXN_TRANSACTION_LOCK_MGR_H_


#include "txn/transaction-base.h"
#include "base/base.h"
#include "mai/transaction.h"
#include "mai/env.h"
#include <mutex>
#include <unordered_map>

namespace mai {
class ColumnFamily;
namespace txn {
    
struct LockInfo;
struct LockMap;
struct LockMapStripe;
    
class PessimisticTransactionDB;
class PessimisticTransaction;
    
class TransactionLockMgr final {
public:
    TransactionLockMgr(PessimisticTransactionDB *owns,
                       size_t default_num_stripes,
                       int64_t max_num_locks,
                       uint32_t max_num_deadlocks);
    ~TransactionLockMgr();
    
    void AddColumnFamily(uint32_t cfid);
    void RemoveColumnFamily(uint32_t cfid);
    
    Error TryLock(PessimisticTransaction *txn, uint32_t cfid,
                  const std::string &key, bool exclusive);
    void Unlock(PessimisticTransaction *txn, const TxnKeyMaps *tracked_keys);
    void UnlockKey(PessimisticTransaction *txn, const std::string &key,
                   LockMapStripe *stripe, LockMap *lock_map);
    
    bool IsLockExpired(TxnID txn_id, const LockInfo &lock_info,
                       uint64_t *expire_time);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(TransactionLockMgr);
private:
    using LockMaps = std::unordered_map<uint32_t, std::shared_ptr<LockMap>>;
    
    std::shared_ptr<LockMap> GetLockMap(uint32_t cfid);
    Error AcquireWithTimeout(PessimisticTransaction *txn, LockMap *lock_map,
                             LockMapStripe *stripe, uint32_t cfid,
                             const std::string &key, int64_t timeout,
                             const LockInfo &lock_info);
    Error AcquireLocked(LockMap *lock_map, LockMapStripe *stripe,
                        const std::string &key, const LockInfo &lock_info,
                        uint64_t *expire_time,
                        std::vector<TxnID> *txn_ids);
    
    static void LockMapsDeleter(void *d);
    
    PessimisticTransactionDB *const owns_;
    const size_t default_num_stripes_;
    const int64_t max_num_locks_;
    Env *const env_;
    
    std::mutex lock_map_mutex_;
    LockMaps lock_maps_;
    std::unique_ptr<ThreadLocalSlot> lock_maps_cache_;
    std::mutex wait_txn_map_mutex_;
    std::unordered_map<TxnID, int> rev_wait_txn_map_;
}; // class TransactionLockMgr

} // namespace txn
    
} // namespace mai


#endif // MAI_TXN_TRANSACTION_LOCK_MGR_H_

