#include "txn/transaction-lock-mgr.h"
#include "txn/pessimistic-transaction.h"
#include "txn/pessimistic-transaction-db.h"
#include "db/db-impl.h"
#include "base/hash.h"

namespace mai {
    
namespace txn {
    
struct LockInfo {
    bool exclusive;
    std::vector<TxnID> txn_ids;
    uint64_t expiration_time;
    
    LockInfo(TxnID id, uint64_t time, bool ex)
        : exclusive(ex)
        , expiration_time(time) {
        txn_ids.push_back(id);
    }
}; // struct LockInfo
    
struct LockMapStripe {
    std::mutex strip_mutex;
    std::condition_variable stripe_cv;
    std::unordered_map<std::string, LockInfo> keys;
    
    void Wait() {
        std::unique_lock<std::mutex> lock(strip_mutex, std::adopt_lock);
        stripe_cv.wait(lock);
        lock.release();
    }
    
    Error WaitFor(uint64_t micro_secs) {
        std::unique_lock<std::mutex> lock(strip_mutex, std::adopt_lock);
        auto rv = stripe_cv.wait_for(lock,
                                     std::chrono::microseconds(micro_secs));
        lock.release();
        return rv == std::cv_status::timeout ?
               MAI_TIMEOUT("CV timeout.") : Error::OK();
    }
}; // struct LockMapStripe
    
struct LockMap {
    const size_t n_stripes;
    std::atomic<int64_t> lock_count{0};
    std::vector<LockMapStripe *> lock_map_stripes;
    
    explicit LockMap(size_t n)
        : n_stripes(n) {
        lock_map_stripes.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            lock_map_stripes.push_back(new LockMapStripe);
        }
    }
    
    ~LockMap() {
        for (auto stripe : lock_map_stripes) {
            delete stripe;
        }
    }
    
    size_t GetStripe(const std::string &key) const {
        return base::Hash::Js(key.data(), key.size()) % n_stripes;
    }
}; // struct LockMap
    
struct DeadLockInfo {
    TxnID txn_id;
    uint32_t cfid;
    bool exclusive;
    std::string waiting_key;
}; // struct DeadLockInfo

struct DeadLockPath {
    std::vector<DeadLockInfo> path;
    int64_t deadlock_time;
    bool limit_exceeded;
    
    DeadLockPath(const std::vector<DeadLockInfo> &entry, int64_t dl_time)
    : path(entry)
    , deadlock_time(dl_time) {}
    
    DeadLockPath(int64_t dl_time = 0, bool limit = false)
    : path(0)
    , limit_exceeded(limit)
    , deadlock_time(dl_time) {}
    
    bool empty() { return path.empty() && !limit_exceeded; }
}; // struct DeadLockPath
    
class DeadLockInfoBuffer {
public:
    explicit DeadLockInfoBuffer(uint32_t n_latest_dlocks)
        : paths_buffer_(n_latest_dlocks)
        , buffer_idx_(0) {}
    
    void AddNewPath(const DeadLockPath &path) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (paths_buffer_.empty()) {
            return;
        }
        
        paths_buffer_[buffer_idx_] = std::move(path);
        buffer_idx_ = (buffer_idx_ + 1) % paths_buffer_.size();
    }
    
    void Resize(uint32_t size) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        paths_buffer_ = Normalize();
        
        if (size < paths_buffer_.size()) {
            paths_buffer_.erase(paths_buffer_.begin(),
                                paths_buffer_.begin() +
                                (paths_buffer_.size() - size));
            buffer_idx_ = 0;
        } else {
            auto prev_size = paths_buffer_.size();
            paths_buffer_.resize(size);
            buffer_idx_ = static_cast<uint32_t>(prev_size);
        }
    }
    
    std::vector<DeadLockPath> PrepareBuffer() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto working(Normalize());
        std::reverse(working.begin(), working.end());
        return working;
    }
    
private:
    std::vector<DeadLockPath> Normalize() {
        if (paths_buffer_.empty()) {
            return paths_buffer_;
        }
        
        auto working(paths_buffer_);
        if (paths_buffer_[buffer_idx_].empty()) {
            working.resize(buffer_idx_);
        } else {
            std::rotate(working.begin(), working.begin() + buffer_idx_,
                        working.end());
        }
        return working;
    }
    
    std::vector<DeadLockPath> paths_buffer_;
    uint32_t buffer_idx_;
    std::mutex mutex_;
}; // class DeadLockInfoBuffer
    
TransactionLockMgr::TransactionLockMgr(PessimisticTransactionDB *owns,
                                       size_t default_num_stripes,
                                       int64_t max_num_locks,
                                       uint32_t max_num_deadlocks)
    : owns_(DCHECK_NOTNULL(owns))
    , default_num_stripes_(default_num_stripes)
    , max_num_locks_(max_num_locks)
    , env_(down_cast<db::DBImpl>(owns->GetDB())->env())
    , dlock_buffer_(new DeadLockInfoBuffer(max_num_deadlocks)) {

    Error rs = env_->NewThreadLocalSlot("lock-mgr", LockMapsDeleter,
                                               &lock_maps_cache_);
    DCHECK(rs.ok()) << rs.ToString();
}

TransactionLockMgr::~TransactionLockMgr() {
    
}

void TransactionLockMgr::AddColumnFamily(uint32_t cfid) {
    std::lock_guard<std::mutex> lock(lock_map_mutex_);
    DCHECK(lock_maps_.find(cfid) == lock_maps_.end());

    lock_maps_.insert({cfid,
                       std::shared_ptr<LockMap>(new LockMap(default_num_stripes_))});
}

void TransactionLockMgr::RemoveColumnFamily(uint32_t cfid) {
    lock_map_mutex_.lock();
    auto iter = lock_maps_.find(cfid);
    DCHECK(iter != lock_maps_.end());
    lock_maps_.erase(iter);
    lock_map_mutex_.unlock();
}
    
Error TransactionLockMgr::TryLock(PessimisticTransaction *txn, uint32_t cfid,
                                  const std::string &key, bool exclusive) {
    std::shared_ptr<LockMap> lock_map = GetLockMap(cfid);
    if (!lock_map) {
        return MAI_CORRUPTION("Column family not found!");
    }
    
    auto stripe_idx = lock_map->GetStripe(key);
    DCHECK_GT(lock_map->lock_map_stripes.size(), stripe_idx);
    
    auto stripe = lock_map->lock_map_stripes.at(stripe_idx);
    LockInfo lock_info(txn->id(), txn->expiration_time(), exclusive);
    
    return AcquireWithTimeout(txn, lock_map.get(), stripe, cfid, key,
                              txn->lock_timeout(), lock_info);
}
    
void TransactionLockMgr::Unlock(PessimisticTransaction *txn,
                                const TxnKeyMaps *tracked_keys) {
    for (auto &key_map : *tracked_keys) {
        uint32_t cfid = key_map.first;
        auto &keys = key_map.second;
        
        std::shared_ptr<LockMap> lock_map = GetLockMap(cfid);
        if (!lock_map) {
            return;
        }
        
        std::unordered_map<size_t, std::vector<const std::string *>>
            keys_by_stripe(std::max(keys.size(), lock_map->n_stripes));
        
        for (const auto &pair : keys) {
            size_t stripe_idx = lock_map->GetStripe(pair.first); // key;
            keys_by_stripe[stripe_idx].push_back(&pair.first);
        }
        
        for (const auto &pair : keys_by_stripe) {
            size_t stripe_idx = pair.first;
            const auto &stripe_keys = pair.second;
            
            DCHECK_GT(lock_map->lock_map_stripes.size(), stripe_idx);
            LockMapStripe *stripe = lock_map->lock_map_stripes.at(stripe_idx);
            
            stripe->strip_mutex.lock();
            
            for (auto key : stripe_keys) {
                UnlockKey(txn, *key, stripe, lock_map.get());
            }
            
            stripe->strip_mutex.unlock();
            stripe->stripe_cv.notify_all();
        }
    }
}
    
void TransactionLockMgr::UnlockKey(PessimisticTransaction *txn,
                                   const std::string &key,
                                   LockMapStripe *stripe, LockMap *lock_map) {
    auto key_found = stripe->keys.find(key);
    if (key_found == stripe->keys.end()) {
#if defined(DEBUG) || defined(_DEBUG)
        DCHECK_GT(txn->expiration_time(), 0);
        DCHECK_LT(txn->expiration_time(), env_->CurrentTimeMicros());
#endif
        return;
    }
    
    auto txn_ids = &key_found->second.txn_ids;
    auto iter = std::find(txn_ids->begin(), txn_ids->end(),txn->id());
    if (iter != txn_ids->end()) {
        if (txn_ids->size() == 1) {
            stripe->keys.erase(key_found);
        } else {
            auto last_it = txn_ids->end() - 1;
            if (iter != last_it) {
                *iter = *last_it;
            }
            txn_ids->pop_back();
        }
        
        if (max_num_locks_ > 0) {
            DCHECK_GT(lock_map->lock_count.load(std::memory_order_relaxed), 0);
            lock_map->lock_count.fetch_sub(1);
        }
    }
}
    
bool TransactionLockMgr::IsLockExpired(TxnID txn_id, const LockInfo &lock_info,
                                       uint64_t *expire_time) {
    uint64_t now = env_->CurrentTimeMicros();
    
    bool expired = (lock_info.expiration_time > 0 &&
                    lock_info.expiration_time <= now);
    
    if (!expired && lock_info.expiration_time > 0) {
        *expire_time = lock_info.expiration_time;
    } else {
        for (auto id : lock_info.txn_ids) {
            if (txn_id == id) {
                continue;
            }
            
            bool success = owns_->TryStealingExpiredTransactionLocks(id);
            if (!success) {
                expired = false;
                break;
            }
            *expire_time = 0;
        }
    }
    return expired;
}
    
std::shared_ptr<LockMap> TransactionLockMgr::GetLockMap(uint32_t cfid) {
    if (lock_maps_cache_->Get() == nullptr) {
        lock_maps_cache_->Set(new LockMaps);
    }
    
    auto cache = static_cast<LockMaps *>(lock_maps_cache_->Get());
    auto iter = cache->find(cfid);
    if (iter != cache->end()) {
        return iter->second;
    }
    
    std::lock_guard<std::mutex> lock(lock_map_mutex_);
    iter = lock_maps_.find(cfid);
    if (iter == lock_maps_.end()) {
        return std::shared_ptr<LockMap>(nullptr);
    }
    
    cache->insert({cfid, iter->second});
    return iter->second;
}

Error
TransactionLockMgr::AcquireWithTimeout(PessimisticTransaction *txn,
                                       LockMap *lock_map,
                                       LockMapStripe *stripe, uint32_t cfid,
                                       const std::string &key, int64_t timeout,
                                       const LockInfo &lock_info) {
    uint64_t end_time = timeout > 0 ? env_->CurrentTimeMicros() + timeout : 0;
    
    if (timeout < 0) {
        stripe->strip_mutex.lock();
    } else {
        bool locked = true;
        if (timeout == 0) {
            locked = stripe->strip_mutex.try_lock();
        } else {
            stripe->strip_mutex.lock();
        }
        if (!locked) {
            return MAI_TIMEOUT("Mutex timeout.");
        }
    }
    
    
    uint64_t expire_time_hint = 0;
    std::vector<TxnID> wait_ids;
    Error rs = AcquireLocked(lock_map, stripe, key, lock_info,
                             &expire_time_hint, &wait_ids);
    if (rs.ok() || timeout == 0) {
        stripe->strip_mutex.unlock();
        return rs;
    }
    
    bool timed_out = false;
    do {
        int64_t cv_end_time = -1;
        
        if (expire_time_hint > 0 &&
            (timeout < 0 || (timeout > 0 && expire_time_hint < end_time))) {
            cv_end_time = expire_time_hint;
        } else if (timeout >= 0) {
            cv_end_time = end_time;
        }
        
        DCHECK(rs.IsBusy() || !wait_ids.empty());
        
        if (!wait_ids.empty()) {
            if (txn->deadlock_detect()) {
                if (IncrementWaiters(txn, wait_ids, key, cfid,
                                     lock_info.exclusive)) {
                    stripe->strip_mutex.unlock();
                    return MAI_BUSY("Dead lock.");
                }
            }
            txn->SetWaitingTxn(wait_ids, cfid, &key);
        }
        
        if (cv_end_time < 0) {
            stripe->Wait();
        } else {
            uint64_t now = env_->CurrentTimeMicros();
            if (static_cast<uint64_t>(cv_end_time) > now) {
                rs = stripe->WaitFor(cv_end_time - now);
            }
        }
        
        if (!wait_ids.empty()) {
            txn->ClearWaitingTxn();
            if (txn->deadlock_detect()) {
                DecrementWaiters(txn, wait_ids);
            }
        }
        
        if (rs.IsTimeout()) {
            timed_out = true;
        }
        if (rs.ok() || rs.IsTimeout()) {
            rs = AcquireLocked(lock_map, stripe, key, lock_info,
                               &expire_time_hint, &wait_ids);
        }
    } while (!rs && !timed_out);
    
    stripe->strip_mutex.unlock();
    return rs;
}
    
Error TransactionLockMgr::AcquireLocked(LockMap *lock_map, LockMapStripe *stripe,
                                        const std::string &key,
                                        const LockInfo &txn_lock_info,
                                        uint64_t *expire_time,
                                        std::vector<TxnID> *txn_ids) {
    DCHECK_EQ(txn_lock_info.txn_ids.size(), 1);
    
    Error rs;
    auto key_found = stripe->keys.find(key);
    if (key_found != stripe->keys.end()) {
        
        LockInfo *lock_info = &key_found->second;
        DCHECK(lock_info->txn_ids.size() == 1 || !lock_info->exclusive);
        
        if (lock_info->exclusive || txn_lock_info.exclusive) {
            
            if (lock_info->txn_ids.size() == 1 &&
                lock_info->txn_ids[0] == txn_lock_info.txn_ids[0]) {
                
                lock_info->exclusive = txn_lock_info.exclusive;
                lock_info->expiration_time = txn_lock_info.expiration_time;
            } else {
                
                if (IsLockExpired(txn_lock_info.txn_ids[0], *lock_info,
                                  expire_time)) {
                    lock_info->txn_ids = txn_lock_info.txn_ids;
                    lock_info->exclusive = txn_lock_info.exclusive;
                    lock_info->expiration_time = txn_lock_info.expiration_time;
                } else {
                    *txn_ids = lock_info->txn_ids;
                    rs = MAI_TIMEOUT("Lock time out!");
                }
            }
        } else {
            lock_info->txn_ids.push_back(txn_lock_info.txn_ids[0]);
            lock_info->expiration_time =
                std::max(lock_info->expiration_time,
                         txn_lock_info.expiration_time);
        }
    } else {
        if (max_num_locks_ > 0 &&
            lock_map->lock_count.load(std::memory_order_acquire) >= max_num_locks_) {
            rs = MAI_BUSY("Lock limit");
        } else {
            stripe->keys.insert({key, txn_lock_info});
            
            if (max_num_locks_) {
                lock_map->lock_count.fetch_add(1);
            }
        }
    }
    return rs;
}
    
bool TransactionLockMgr::IncrementWaiters(const PessimisticTransaction *txn,
                                          const std::vector<TxnID> &wait_ids,
                                          const std::string &key, uint32_t cfid,
                                          bool exclusive) {
    TxnID txn_id = txn->id();
    
    std::vector<int> queue_parents(txn->deadlock_detect_depth());
    std::vector<TxnID> queue_values(txn->deadlock_detect_depth());
    
    std::lock_guard<std::mutex> lock(wait_txn_map_mutex_);
    DCHECK(wait_txn_map_.find(txn_id) == wait_txn_map_.end());
    
    wait_txn_map_.insert({txn_id, {wait_ids, cfid, key, exclusive}});
    
    for (auto wid : wait_ids) {
        if (rev_wait_txn_map_.find(wid) != rev_wait_txn_map_.end()) {
            rev_wait_txn_map_[wid]++;
        } else {
            rev_wait_txn_map_[wid] = 1;
        }
    }
    if (rev_wait_txn_map_.find(txn_id) == rev_wait_txn_map_.end()) {
        return false;
    }
    
    const auto *next_ids = &wait_ids;
    int parent = -1;
    int64_t dlock_time = 0;
    for (int tail = 0, head = 0; head < txn->deadlock_detect_depth(); head++) {
        int i = 0;
        if (next_ids) {
            for (; i < next_ids->size() &&
                 tail + i < txn->deadlock_detect_depth(); ++i) {
                queue_values[tail + i] = next_ids->at(i);
                queue_parents[tail + i] = parent;
            }
            tail += i;
        }
        
        if (tail == head) {
            return false;
        }
        
        TxnID next = queue_values[head];
        if (next == txn_id) {
            std::vector<DeadLockInfo> path;
            while (head != -1) {
                DCHECK(wait_txn_map_.find(queue_values[head])
                       != wait_txn_map_.end());
                TrackedTxnInfo extracted_info
                    = wait_txn_map_[queue_values[head]];
                path.push_back({
                    queue_values[head],
                    extracted_info.cfid,
                    extracted_info.exclusive,
                    extracted_info.waiting_key
                });
                head = queue_parents[head];
            }
            dlock_time = env_->CurrentTimeMicros();
            std::reverse(path.begin(), path.end());
            dlock_buffer_->AddNewPath(DeadLockPath(path, dlock_time));
            dlock_time = 0;
            DecrementWaitersLockless(txn, wait_ids);
            return true;
        } else if (wait_txn_map_.find(next) == wait_txn_map_.end()) {
            next_ids = nullptr;
            continue;
        } else {
            parent = head;
            next_ids = &wait_txn_map_[next].neighbors;
        }
    }
    
    dlock_time = env_->CurrentTimeMicros();
    dlock_buffer_->AddNewPath(DeadLockPath(dlock_time, true));
    DecrementWaitersLockless(txn, wait_ids);
    return true;
}
    
void TransactionLockMgr::DecrementWaiters(const PessimisticTransaction *txn,
                                          const std::vector<TxnID> &wait_ids) {
    std::lock_guard<std::mutex> lock(wait_txn_map_mutex_);
    DecrementWaitersLockless(txn, wait_ids);
}

void
TransactionLockMgr::DecrementWaitersLockless(const PessimisticTransaction *txn,
                                             const std::vector<TxnID> &wait_ids) {
    DCHECK(wait_txn_map_.find(txn->id()) != wait_txn_map_.end());
    wait_txn_map_.erase(txn->id());
    
    for (auto wid : wait_ids) {
        rev_wait_txn_map_[wid]--;
        if (rev_wait_txn_map_[wid] == 0) {
            rev_wait_txn_map_.erase(wid);
        }
    }
}
    
/*static*/ void TransactionLockMgr::LockMapsDeleter(void *d) {
    delete static_cast<LockMaps *>(d);
}

} // namespace txn
    
} // namespace mai
