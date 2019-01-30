#ifndef MAI_SQL_STORAGE_ENGINE_H_
#define MAI_SQL_STORAGE_ENGINE_H_

#include "sql/types.h"
#include "base/base.h"
#include "mai/transaction-db.h"
#include "mai/db.h"
#include "glog/logging.h"
#include <unordered_map>
#include <atomic>

namespace mai {

namespace sql {
    
class Form;
    
class StorageEngine {
public:
    StorageEngine(TransactionDB *trx_db, StorageKind kind);
    ~StorageEngine();
    
    Error Prepare(bool boot);
    
    Error NewTable(const std::string &db_name, const Form *form);
    
    DEF_PTR_GETTER(TransactionDB, trx_db);

    ColumnFamily *sys_cf() const {
        DCHECK_LT(sys_cf_idx_, column_families_.size());
        return column_families_[sys_cf_idx_];
    }
    ColumnFamily *def_cf() const {
        DCHECK_LT(def_cf_idx_, column_families_.size());
        return column_families_[def_cf_idx_];
    }
    
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
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(StorageEngine);
private:
    Error SyncMetadata(const std::string &arg, uint32_t flags);
    
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

    static const char kSysCfName[];
    static const char kSysNextIndexIdKey[];
    static const char kSysNextColumnIdKey[];
    static const char kSysColumnsKey[];
    static const char kSysIndicesKey[];
}; // class StorageEngine

} // namespace sql
    
} // namespace mai

#endif // MAI_SQL_STORAGE_ENGINE_H_