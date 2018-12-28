#ifndef MAI_TXN_OPTIMISM_TRANSACTION_H_
#define MAI_TXN_OPTIMISM_TRANSACTION_H_

#include "txn/write-batch-with-index.h"
#include "core/key-boundle.h"
#include "base/base.h"
#include "mai/transaction.h"
#include <unordered_map>

namespace mai {
namespace db {
class DBImpl;
class ColumnFamilyImpl;
} // namespace db
namespace txn {

class OptimismTransactionDB;
    
struct TxnKeyInfo {
    core::SequenceNumber seq;
    
    uint32_t n_writes = 0;
    uint32_t n_reads = 0;
    bool exclusive = false;
    
    TxnKeyInfo(core::SequenceNumber aseq) : seq(aseq) {}
};
    
using TxnKeyMap = std::unordered_map<std::string, TxnKeyInfo>;
    
class OptimismTransaction final : public Transaction {
public:
    OptimismTransaction(const WriteOptions &opts,
                        OptimismTransactionDB *db);
    virtual ~OptimismTransaction() override;
    
    void Reinitialize(const WriteOptions &opts, OptimismTransactionDB *db);
    
    virtual Error Rollback() override;
    virtual Error Commit() override;
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
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(OptimismTransaction);
private:
    class Callback;
    
    // OptimismTransaction never real lock
    Error TryLock(ColumnFamily* cf, std::string_view key, bool read_only,
                  bool exclusive, const bool do_validate);
    void TrackKey(uint32_t cfid, const std::string& key,
                  core::SequenceNumber seq, bool read_only, bool exclusive);
    Error CheckTransactionForConflicts(db::DBImpl *db);
    Error CheckKey(db::DBImpl *db, db::ColumnFamilyImpl *impl,
                   core::SequenceNumber earliest_seq,
                   core::SequenceNumber key_seq,
                   std::string_view key, bool cache_only);
    void Clear();
    void SetSnapshotIfNeeded();
    
    WriteOptions options_;
    OptimismTransactionDB *db_;
    WriteBatchWithIndex write_batch_;
    const Snapshot *snapshot_ = nullptr;
    std::unordered_map<uint32_t, TxnKeyMap> txn_keys_;
}; // class OptimismTransaction
    
} // namespace txn
    
} // namespace mai

#endif // MAI_TXN_OPTIMISM_TRANSACTION_H_
