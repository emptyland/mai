#include "txn/pessimistic-transaction.h"
#include "db/db-impl.h"
#include "db/column-family.h"
#include "mai/transaction-db.h"
#include <map>
#include <set>

namespace mai {
    
namespace txn {
    
PessimisticTransaction::PessimisticTransaction(const TransactionOptions &options,
                                               const WriteOptions &wr_opts,
                                               PessimisticTransactionDB *db)
    : TransactionBase(wr_opts, db) {
    Initialize(options);
}

/*virtual*/ PessimisticTransaction::~PessimisticTransaction() {
    owns()->UnLock(this, &tracked_keys());
    if (expiration_time_ > 0) {
        owns()->RemoveExpirableTransaction(txn_id_);
    }
    if (!name_.empty() && state() != COMMITED) {
        owns()->UnregisterTransaction(this);
    }
}

void PessimisticTransaction::Reinitialize(const TransactionOptions &options,
                                          const WriteOptions &wr_opts,
                                          TransactionDB *db) {
    if (!name_.empty() && state() != COMMITED) {
        owns()->UnregisterTransaction(this);
    }
    TransactionBase::Reinitialize(wr_opts, db);
    Initialize(options);
}
    
void PessimisticTransaction::Initialize(const TransactionOptions &options) {
    txn_id_ = owns()->GenerateTxnID();
    set_state(STARTED);
    
    deadlock_detect_ = options.deadlock_detect;
    deadlock_detect_depth_ = options.deadlock_detect_depth;
    lock_timeout_ = options.lock_timeout * 1000L;
    if (lock_timeout_ < 0) {
        lock_timeout_ = owns()->options().transaction_lock_timeout * 1000;
    }
    
    if (options.expiration >= 0) {
        expiration_time_ = start_time_ + options.expiration * 1000;
    } else {
        expiration_time_ = 0;
    }
    
    if (expiration_time_ > 0) {
        owns()->InsertExpirableTransaction(txn_id_, this);
    }
}
    
bool PessimisticTransaction::IsExpired() const {
    if (expiration_time_ > 0) {
        if (impl()->env()->CurrentTimeMicros() >= expiration_time_) {
            return true;
        }
    }
    return false;
}
    
/*virtual*/ void PessimisticTransaction::Clear() {
    owns()->UnLock(this, &tracked_keys());
    TransactionBase::Clear();
}
    
/*virtual*/ Error PessimisticTransaction::SetName(const std::string &name) {
    if (state() == STARTED) {
        if (!name_.empty()) {
            return MAI_CORRUPTION("Transaction has already been named.");
        } else if (owns()->GetTransactionByName(name) != nullptr) {
            return MAI_CORRUPTION("Duplicated transaction name.");
        } else if (name.size() < 1 || name.size() > 512) {
            return MAI_CORRUPTION("Transaction name length must be between 1 "
                                  "and 512 chars.");
        } else {
            name_ = name;
            owns()->RegisterTransaction(this);
        }
    } else {
        return MAI_CORRUPTION("Transaction is beyond state for naming.");
    }
    return Error::OK();
}

/*virtual*/ Error PessimisticTransaction::Rollback() {
    Error rs;
    switch (state()) {
        case STARTED:
            Clear();
            set_state(ROLLEDBACK);
            break;
        case COMMITED:
            rs = MAI_CORRUPTION("This transaction has already been commited.");
            break;
        default:
            rs = MAI_CORRUPTION("Bad transaction state.");
            break;
    }
    return rs;
}

/*virtual*/ Error PessimisticTransaction::Commit() {
    if (IsExpired()) {
        return MAI_CORRUPTION("This transaction has been expired.");
    }
    
    bool ready_for_commit = false;
    if (expiration_time_ > 0) {
        ready_for_commit = update_state(STARTED, AWAITING_COMMIT);
    } else if (state() == STARTED) {
        ready_for_commit = true;
    }
    
    Error rs;
    if (ready_for_commit) {
        set_state(AWAITING_COMMIT);
        rs = impl()->WriteImpl(write_opts(), mutable_write_batch(), nullptr);
        if (!name_.empty()) {
            owns()->UnregisterTransaction(this);
        }
        Clear();
        if (rs.ok()) {
            set_state(COMMITED);
        }
    } else if (state() == LOCKS_STOLEN) {
        return MAI_CORRUPTION("This transaction has been expired.");
    } else if (state() == COMMITED) {
        rs = MAI_CORRUPTION("Transaction has already been commited.");
    } else if (state() == ROLLEDBACK) {
        rs = MAI_CORRUPTION("Transaction has already been rollback.");
    } else {
        rs = MAI_CORRUPTION("Transaction is not in state for commit.");
    }
    return rs;
}

/*virtual*/ Error
PessimisticTransaction::TryLock(ColumnFamily* cf, std::string_view key,
                                bool read_only, bool exclusive,
                                const bool do_validate) {
    uint32_t cfid = cf->id();
    std::string hold_key(key);
    bool prev_locked;
    bool lock_upgrade = false;
    
    // lock this key if this transactions hasn't already locked it
    core::SequenceNumber tracked_at_seq = core::Tag::kMaxSequenceNumber;
    const auto &keys = tracked_keys();
    auto cf_iter = keys.find(cfid);
    if (cf_iter == keys.end()) {
        prev_locked = false;
    } else {
        auto iter = cf_iter->second.find(hold_key);
        if (iter == cf_iter->second.end()) {
            prev_locked = false;
        } else {
            if (!iter->second.exclusive && exclusive) {
                lock_upgrade = true;
            }
            prev_locked = true;
            tracked_at_seq = iter->second.seq;
        }
    }
    
    Error rs;
    if (!prev_locked || lock_upgrade) {
        rs = owns()->TryLock(this, cfid, hold_key, exclusive);
    }
    
    if (!do_validate) {
        if (tracked_at_seq == core::Tag::kMaxSequenceNumber) {
            tracked_at_seq = impl()->GetLatestSequenceNumber();
        }
    }
    
    if (rs.ok()) {
        TrackKey(cfid, hold_key, tracked_at_seq, read_only, exclusive);
    }
    return rs;
}
    
Error PessimisticTransaction::CommitBatch(WriteBatch *updates) {
    TxnKeyMaps keys_to_unlock;
    Error rs = LockBatch(updates, &keys_to_unlock);
    if (!rs) {
        return rs;
    }
    bool can_commit = false;

    if (IsExpired()) {
        rs = MAI_CORRUPTION("Expired.");
    } else if (expiration_time_ > 0) {
        can_commit = update_state(STARTED, AWAITING_COMMIT);
    } else if (state() == STARTED) {
        can_commit = true;
    }

    if (can_commit) {
        rs = impl()->WriteImpl(write_opts(), updates, nullptr);
        if (rs.ok()) {
            set_state(COMMITED);
        }
    } else if (state() == LOCKS_STOLEN) {
        rs = MAI_CORRUPTION("Expired.");
    } else {
        rs = MAI_CORRUPTION("Bad transaction state");
    }
    
    owns()->UnLock(this, &keys_to_unlock);
    return rs;
}
    
class RecordHandler : public WriteBatch::Stub {
public:
    RecordHandler() {}
    virtual ~RecordHandler() override {}

    virtual void Put(uint32_t cfid, std::string_view key,
                     std::string_view /*value*/) override {
        RecordKey(cfid, key);
    }
    
    virtual void Delete(uint32_t cfid, std::string_view key) override {
        RecordKey(cfid, key);
    }
    
    void RecordKey(uint32_t cfid, std::string_view key) {
        std::string hold_key(key);
        
        auto iter = keys_[cfid].find(hold_key);
        if (iter == keys_[cfid].end()) {
            keys_[cfid].insert(std::move(hold_key));
        }
    }
    
    std::map<uint32_t, std::set<std::string>> keys_;
}; // class RecordHandler
    
Error PessimisticTransaction::LockBatch(WriteBatch *updates,
                                        TxnKeyMaps *keys_to_unlock) {
    RecordHandler handler;
    updates->Iterate(&handler);
    
    Error rs;
    for (const auto &cf : handler.keys_) {
        uint32_t cfid = cf.first;
        
        for (const auto &key : cf.second) {
            rs = owns()->TryLock(this, cfid, key, true);
            if (!rs) {
                break;
            }
            TrackKey(keys_to_unlock, cfid, std::move(key),
                     core::Tag::kMaxSequenceNumber, false, true);
        }
        if (!rs) {
            break;
        }
    }
    
    if (!rs) {
        owns()->UnLock(this, keys_to_unlock);
    }
    return rs;
}

} // namespace txn
    
} // namespace mai
