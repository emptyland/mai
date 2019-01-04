#ifndef MAI_TXN_TRANSACTION_BASE_H_
#define MAI_TXN_TRANSACTION_BASE_H_

#include "txn/write-batch-with-index.h"
#include "core/key-boundle.h"
#include "base/base.h"
#include "mai/transaction.h"
#include <unordered_map>

namespace mai {
class TransactionDB;
namespace db {
class DBImpl;
class ColumnFamilyImpl;
} // namespace db
namespace txn {

struct TxnKeyInfo {
    core::SequenceNumber seq;
    
    uint32_t n_writes = 0;
    uint32_t n_reads = 0;
    bool exclusive = false;
    
    TxnKeyInfo(core::SequenceNumber aseq) : seq(aseq) {}
};

using TxnKeyMap  = std::unordered_map<std::string, TxnKeyInfo>;
using TxnKeyMaps = std::unordered_map<uint32_t, TxnKeyMap>;

class TransactionBase : public Transaction {
public:
    TransactionBase(const WriteOptions &opts, TransactionDB *db);
    virtual ~TransactionBase() override;
    
    db::DBImpl *impl() const;
    DEF_VAL_GETTER(WriteOptions, write_opts);
    DEF_PTR_GETTER_NOTNULL(TransactionDB, db);
    DEF_VAL_MUTABLE_GETTER(WriteBatchWithIndex, write_batch);

    const TxnKeyMaps &tracked_keys() const {
        return txn_keys_;
    }
    
    void set_state(Transaction::State state) {
        state_.store(state, std::memory_order_release);
    }
    
    bool update_state(Transaction::State expected, Transaction::State new_val) {
        return state_.compare_exchange_strong(expected, new_val);
    }
    
    void Reinitialize(const WriteOptions &opts, TransactionDB *db);
    virtual void Clear();
    
    virtual std::string name() override { return ""; }
    virtual TxnID id() const override { return 0; }
    virtual Error SetName(const std::string &name) override {
        return Error::OK();
    }

    virtual Error Put(ColumnFamily *cf, std::string_view key,
                      std::string_view value) override;
    virtual Error Delete(ColumnFamily *cf, std::string_view key) override;
    virtual Error Get(const ReadOptions &opts, ColumnFamily *cf,
                      std::string_view key, std::string *value) override;
    virtual Error GetForUpdate(const ReadOptions &opts, ColumnFamily *cf,
                               std::string_view key, std::string *value,
                               bool exclusive, const bool do_validate) override;
    virtual Iterator* GetIterator(const ReadOptions& opts,
                                  ColumnFamily* cf) override;
    
    virtual Error TryLock(ColumnFamily* cf, std::string_view key, bool read_only,
                          bool exclusive, const bool do_validate) = 0;
    
    Error PutUntracked(ColumnFamily *cf, std::string_view key,
                       std::string_view value);
    Error DeleteUntracked(ColumnFamily *cf, std::string_view key);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(TransactionBase);
protected:
    void TrackKey(uint32_t cfid, const std::string& key,
                  core::SequenceNumber seq, bool read_only, bool exclusive) {
        TrackKey(&txn_keys_, cfid, key, seq, read_only, exclusive);
    }
    
    void TrackKey(TxnKeyMaps *key_map, uint32_t cfid, const std::string& key,
                  core::SequenceNumber seq, bool read_only, bool exclusive);
    Error CheckTransactionForConflicts(db::DBImpl *db);
    Error CheckKey(db::DBImpl *db, db::ColumnFamilyImpl *impl,
                   core::SequenceNumber earliest_seq,
                   core::SequenceNumber key_seq,
                   std::string_view key, bool cache_only);
    
    uint64_t start_time_ = 0;
    const Snapshot *snapshot_ = nullptr;

private:
    WriteOptions write_opts_;
    TransactionDB *db_;
    WriteBatchWithIndex write_batch_;
    std::unordered_map<uint32_t, TxnKeyMap> txn_keys_;
}; // class OptimismTransaction

} // namespace txn
    
} // namespace mai


#endif // MAI_TXN_TRANSACTION_BASE_H_
