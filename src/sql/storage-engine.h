#ifndef MAI_SQL_STORAGE_ENGINE_H_
#define MAI_SQL_STORAGE_ENGINE_H_

#include "sql/types.h"
#include "base/base.h"
#include "mai/transaction-db.h"
#include "mai/db.h"
#include "glog/logging.h"
#include <unordered_map>
#include <map>
#include <set>
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
    
    StorageOperation *NewWriter();
    
    DEF_PTR_GETTER(TransactionDB, trx_db);

    ColumnFamily *sys_cf() const {
        DCHECK_LT(sys_cf_idx_, column_families_.size());
        return column_families_[sys_cf_idx_];
    }
    ColumnFamily *def_cf() const {
        DCHECK_LT(def_cf_idx_, column_families_.size());
        return column_families_[def_cf_idx_];
    }
    
    FRIEND_UNITTEST_CASE(StorageEngineTest, NewTable);
    FRIEND_UNITTEST_CASE(StorageEngineTest, Recovery);
    DISALLOW_IMPLICIT_CONSTRUCTORS(StorageEngine);
private:
    class ColumnStorageBatch;
    
    struct ItemMetadata {
        bool column = false;
        ColumnFamily *owns_cf = nullptr;
    };
    
    struct TableMetadata {
        TableMetadata() : auto_increment(0) {}
        std::atomic<uint64_t> auto_increment;
        std::set<uint32_t> items;
    };
    
    Error SyncMetadata(const std::string &table_name);
    
    ColumnFamily *GetTableCfOrNull(const std::string &name) const {
        auto it = cf_names_.find(name);
        return it == cf_names_.end() ? nullptr : column_families_[it->second];
    }
    
    TransactionDB *trx_db_;
    const StorageKind kind_;
    
    std::vector<ColumnFamily *> column_families_;
    std::unordered_map<std::string, size_t> cf_names_;
    size_t sys_cf_idx_ = 0; // __system__
    size_t def_cf_idx_ = 0; // default
    
    std::unordered_map<uint32_t, ItemMetadata> items_;
    std::map<std::string, TableMetadata> item_owns_;

    mutable std::shared_mutex meta_mutex_;
}; // class StorageEngine

} // namespace sql
    
} // namespace mai

#endif // MAI_SQL_STORAGE_ENGINE_H_
