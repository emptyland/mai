#ifndef MAI_SQL_STORAGE_ENGINE_H_
#define MAI_SQL_STORAGE_ENGINE_H_

#include "sql/types.h"
#include "base/base.h"
#include "mai/transaction-db.h"
#include "mai/db.h"
#include "glog/logging.h"
#include <unordered_map>
#include <atomic>
#include <shared_mutex>

namespace mai {

namespace sql {
    
class Form;
class HeapTuple;
class ColumnDescriptor;
    
class StorageOperation {
public:
    StorageOperation() {}
    virtual ~StorageOperation() {}

    virtual Error Put(const HeapTuple *tuple, uint64_t key_hint = 0) = 0;
    
    virtual Error Finialize(bool commit = false) = 0;
    
    virtual Error Commit() = 0;
    
    virtual Error Rollback() = 0;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(StorageOperation);
}; // class Storage
    
class StorageEngine final {
public:
    StorageEngine(TransactionDB *trx_db, StorageKind kind);
    ~StorageEngine();
    
    Error Prepare(bool boot);
    
    Error NewTable(const std::string &db_name, const Form *form);
    
    Error PutTuple(const HeapTuple *tuple);
    
    StorageOperation *NewBatch();
    
    DEF_PTR_GETTER(TransactionDB, trx_db);

    ColumnFamily *sys_cf() const {
        DCHECK_LT(sys_cf_idx_, column_families_.size());
        return column_families_[sys_cf_idx_];
    }
    ColumnFamily *def_cf() const {
        DCHECK_LT(def_cf_idx_, column_families_.size());
        return column_families_[def_cf_idx_];
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(StorageEngine);
private:
    class ColumnStorageBatch;
    
    struct Column {
        ColumnFamily *owns_cf;
        uint32_t      col_id;
    };
    
    struct Index {
        ColumnFamily *owns_cf;
        uint32_t      idx_id;
    };
    
    enum Flag : uint32_t {
        kIds = 1,
        kColumns = 1u << 1,
        kIndices = 1u << 2,
    };
    
    Error SyncMetadata(const std::string &arg, uint32_t flags);
    
    ColumnFamily *GetTableCfOrNull(const std::string &name) const {
        auto it = cf_names_.find(name);
        return it == cf_names_.end() ? nullptr : column_families_[it->second];
    }
    
    void MakePrimaryIndex(const HeapTuple *tuple, const ColumnDescriptor *cd, std::string *buf);
    void MakeSecondaryIndex(const HeapTuple *tuple, const ColumnDescriptor *cd,
                            std::string_view pri_key, std::string *buf);
    void MakeValue(const HeapTuple *tuple, const ColumnDescriptor *cd, std::string *buf);
    
    TransactionDB *trx_db_;
    const StorageKind kind_;
    std::atomic<uint32_t> next_column_id_;
    std::atomic<uint32_t> next_index_id_;
    
    std::vector<ColumnFamily *> column_families_;
    std::unordered_map<std::string, size_t> cf_names_;
    size_t sys_cf_idx_ = 0; // __system__
    size_t def_cf_idx_ = 0; // default
    
    std::unordered_map<std::string, std::shared_ptr<Column>> columns_;
    std::unordered_map<std::string, std::shared_ptr<Index>> indices_;
    
    mutable std::shared_mutex meta_mutex_;

    static const char kSysCfName[];
    static const char kSysNextIndexIdKey[];
    static const char kSysNextColumnIdKey[];
    static const char kSysColumnsKey[];
    static const char kSysIndicesKey[];
}; // class StorageEngine

} // namespace sql
    
} // namespace mai

#endif // MAI_SQL_STORAGE_ENGINE_H_
