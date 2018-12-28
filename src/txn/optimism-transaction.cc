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
    : write_batch_(db->impl()->env()->GetLowLevelAllocator()) {
    Reinitialize(opts, db);
}

/*virtual*/ OptimismTransaction::~OptimismTransaction() {
    if (snapshot_) {
        db_->impl()->ReleaseSnapshot(snapshot_);
    }
}

void OptimismTransaction::Reinitialize(const WriteOptions &opts,
                                       OptimismTransactionDB *db) {
    options_  = opts;
    db_       = DCHECK_NOTNULL(db);
    snapshot_ = nullptr;
    write_batch_.Clear();
    txn_keys_.clear();
    state_.store(Transaction::STARTED, std::memory_order_release);
}

/*virtual*/ Error OptimismTransaction::Rollback() {
    Clear();
    state_.store(Transaction::ROLLEDBACK, std::memory_order_release);
    return Error::OK();
}

/*virtual*/ Error OptimismTransaction::Commit() {
    auto db = db_->impl();
    
    Callback callback(this);
    
    Error rs = db->WriteImpl(options_, &write_batch_, &callback);
    if (rs.ok()) {
        Clear();
        state_.store(Transaction::COMMITED, std::memory_order_release);
    }
    return rs;
}

/*virtual*/ Error
OptimismTransaction::Put(ColumnFamily *cf, std::string_view key,
                         std::string_view value) {
    Error rs = TryLock(cf, key, false /* read_only */, true /* exclusive */,
                       true);
    if (rs.ok()) {
        write_batch_.AddOrUpdate(cf, core::Tag::kFlagValue, key, value);
    }
    return rs;
}
    
/*virtual*/ Error
OptimismTransaction::Delete(ColumnFamily *cf, std::string_view key) {
    Error rs = TryLock(cf, key, false /* read_only */, true /* exclusive */,
                       true);
    if (rs.ok()) {
        write_batch_.AddOrUpdate(cf, core::Tag::kFlagDeletion, key, "");
    }
    return rs;
}
    
/*virtual*/ Error OptimismTransaction::Get(const ReadOptions &opts,
                                           ColumnFamily *cf,
                                           std::string_view key,
                                           std::string *value) {
    uint8_t flag;
    Error rs = write_batch_.RawGet(cf, key, &flag, value);
    if (rs.ok()) {
        if (flag == core::Tag::kFlagDeletion) {
            return MAI_NOT_FOUND("Deleted.");
        }
        return rs;
    }
    return db_->impl()->Get(opts, cf, key, value);
}
    
/*virtual*/ Error
OptimismTransaction::GetForUpdate(const ReadOptions &opts, ColumnFamily *cf,
                                  std::string_view key, std::string *value,
                                  bool exclusive, const bool do_validate) {
    if (!do_validate && opts.snapshot != nullptr) {
        return MAI_CORRUPTION("If do_validate is false then GetForUpdate with "
                              "snapshot is not defined.");
    }
    
    Error rs = TryLock(cf, key, true /* read_only */, exclusive, do_validate);
    if (rs.ok()) {
        rs = Get(opts, cf, key, value);
    }
    return rs;
}
    
/*virtual*/ Iterator *OptimismTransaction::GetIterator(const ReadOptions& opts,
                                                       ColumnFamily* cf)  {
    auto base = db_->NewIterator(opts, cf);
    if (base->error().fail()) {
        return base;
    }
    return write_batch_.NewIteratorWithBase(cf, base);
}
    
Error OptimismTransaction::TryLock(ColumnFamily* cf, std::string_view key,
                                   bool read_only, bool exclusive,
                                   const bool do_validate) {
    if (!do_validate) {
        return Error::OK();
    }
    
    SetSnapshotIfNeeded();
    
    core::SequenceNumber seq;
    if (snapshot_) {
        seq = db::SnapshotImpl::Cast(snapshot_)->sequence_number();
    } else {
        seq = db_->impl()->GetLatestSequenceNumber();
    }
    std::string hold_key(key);
    
    TrackKey(cf->id(), hold_key, seq, read_only, exclusive);
    return Error::OK();
}
    
void OptimismTransaction::TrackKey(uint32_t cfid, const std::string& key,
                                    core::SequenceNumber seq, bool read_only,
                                    bool exclusive) {
    auto &keys = txn_keys_[cfid];
    auto iter  = keys.find(key);
    if (iter == keys.end()) {
        std::tie(iter, std::ignore) = keys.insert({key, TxnKeyInfo(seq)});
    } else if (seq < iter->second.seq) {
        iter->second.seq = seq;
    }
    
    if (read_only) {
        iter->second.n_reads++;
    } else {
        iter->second.n_writes++;
    }
    iter->second.exclusive |= exclusive;
}
    
Error OptimismTransaction::CheckTransactionForConflicts(db::DBImpl *db) {
    Error rs;
    for (const auto &keys_iter : txn_keys_) {
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
    
Error OptimismTransaction::CheckKey(db::DBImpl *db, db::ColumnFamilyImpl *impl,
                                    core::SequenceNumber earliest_seq,
                                    core::SequenceNumber key_seq,
                                    std::string_view key, bool cache_only) {
    Error rs;
    bool need_to_read_sst = false;
    if (earliest_seq == core::Tag::kMaxSequenceNumber) {
        need_to_read_sst = true;
        
        if (cache_only) {
            auto m = base::Slice::Sprintf("Transaction only check memory-table "
                                          "at SequenceNumber: %" PRIu64,
                                          key_seq);
            rs = MAI_TRY_AGAIN(m);
        }
    } else if (key_seq < earliest_seq) {
        need_to_read_sst = true;
        
        if (cache_only) {
            auto m = base::Slice::Sprintf("Transaction only check memory-table "
                                          "at SequenceNumber: %" PRIu64,
                                          key_seq);
            rs = MAI_TRY_AGAIN(m);
        }
    }
    
    if (rs.ok()) {
        auto seq = core::Tag::kMaxSequenceNumber;
        
        rs = db->GetLatestSequenceForKey(impl, !need_to_read_sst, key, &seq);
        if (rs.IsNotFound()) {
            return Error::OK();
        } else if (rs.fail()) {
            return rs;
        } else {
            if (key_seq < seq) {
                rs = MAI_BUSY("Transaction write conflict");
            }
        }
    }
    return rs;
}
    
void OptimismTransaction::Clear() {
    snapshot_ = nullptr;
    write_batch_.Clear();
    txn_keys_.clear();
}
    
void OptimismTransaction::SetSnapshotIfNeeded() {
    // TODO:
}

} // namespace txn
    
} // namespace mai
