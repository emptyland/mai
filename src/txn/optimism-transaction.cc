#include "txn/optimism-transaction.h"
#include "txn/optimism-transaction-db.h"
#include "txn/write-batch-with-index.h"
#include "db/db-impl.h"
#include "db/column-family.h"
#include "base/slice.h"
#include "mai/iterator.h"
#include "glog/logging.h"

namespace mai {
    
namespace txn {
    
class OptimismTransaction::Callback final : public db::WriteCallback {
public:
    Callback(OptimismTransaction *owns) : owns_(DCHECK_NOTNULL(owns)) {}
    
    // Thie callback has been acquired db mutex_ !
    virtual Error Prepare(db::DBImpl *db) override {
        return owns_->CheckTransactionForConflicts(db);
    }
private:
    OptimismTransaction *const owns_;
};

OptimismTransaction::OptimismTransaction(const WriteOptions &opts,
                                         OptimismTransactionDB *db)
    : TransactionBase(opts, db) {
}

/*virtual*/ OptimismTransaction::~OptimismTransaction() {
}

/*virtual*/ Error OptimismTransaction::Rollback() {
    Clear();
    set_state(ROLLEDBACK);
    return Error::OK();
}

/*virtual*/ Error OptimismTransaction::Commit() {
    Callback callback(this);
    
    Error rs = impl()->WriteImpl(write_opts(), mutable_write_batch(), &callback);
    if (rs.ok()) {
        Clear();
        set_state(COMMITED);
    }
    return rs;
}
    
Error OptimismTransaction::TryLock(ColumnFamily* cf, std::string_view key,
                                   bool read_only, bool exclusive,
                                   const bool do_validate) {
    if (!do_validate) {
        return Error::OK();
    }
    
    //SetSnapshotIfNeeded();
    
    core::SequenceNumber seq;
    if (snapshot_) {
        seq = db::SnapshotImpl::Cast(snapshot_)->sequence_number();
    } else {
        seq = impl()->GetLatestSequenceNumber();
    }
    std::string hold_key(key);
    
    TrackKey(cf->id(), hold_key, seq, read_only, exclusive);
    return Error::OK();
}
    
Error OptimismTransaction::CheckTransactionForConflicts(db::DBImpl *db) {
    Error rs;
    for (const auto &keys_iter : tracked_keys()) {
        uint32_t cfid = keys_iter.first;
        const TxnKeyMap &keys = keys_iter.second;
        
        base::intrusive_ptr<db::ColumnFamilyImpl> impl;
        rs = db->GetColumnFamilyImpl(cfid, &impl);
        if (!rs) {
            break;
        }
        
        // TODO: use memory-table earliest_seq
        core::SequenceNumber earliest_seq = core::Tag::kMaxSequenceNumber;
        for (const auto &key_iter : keys) {
            const std::string &key = key_iter.first;
            const core::SequenceNumber key_seq = key_iter.second.seq;
            
            rs = CheckKey(db, impl.get(), earliest_seq, key_seq, key, false);
            if (!rs) {
                break;
            }
        }
        if (!rs) {
            break;
        }
    }
    return rs;
}

} // namespace txn
    
} // namespace mai
