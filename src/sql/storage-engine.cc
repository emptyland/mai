#include "sql/storage-engine.h"
#include "sql/form-schema.h"
#include "sql/heap-tuple.h"
#include "base/slice.h"
#include "mai/transaction.h"
#include "mai/write-batch.h"
//#include <shared_mutex>

namespace mai {

namespace sql {
    
using ::mai::base::Slice;

/*static*/ const char StorageEngine::kSysCfName[] = "__system__";
/*static*/ const char StorageEngine::kSysNextIndexIdKey[] = "next_index_id";
/*static*/ const char StorageEngine::kSysNextColumnIdKey[] = "next_column_id";
/*static*/ const char StorageEngine::kSysColumnsKey[] = "columns";
/*static*/ const char StorageEngine::kSysIndicesKey[] = "indices";
    
class StorageEngine::ColumnStorageBatch : public StorageOperation {
public:
    ColumnStorageBatch(StorageEngine *owns)
        : owns_(owns) {
        snapshot_ = owns_->trx_db_->GetSnapshot();
    }
    
    virtual ~ColumnStorageBatch() override {
        owns_->trx_db_->ReleaseSnapshot(snapshot_);
    }
    
    
    /*
           +-----------+-------------+        +-------+
     key : | column-id | primary-key |  val : | value |
           +-----------+-------------+        +-------+
     
           +-----------+---------------+-------------+       +------+
     key : | column-id | secondary-key | primary-key | val : | info |
           +-----------+---------------+-------------+       +------+
     */
    virtual Error Put(const HeapTuple *tuple, uint64_t key_hint) override {
        std::shared_lock<std::shared_mutex> lock(owns_->meta_mutex_);
        const Form *frm = DCHECK_NOTNULL(tuple->schema()->origin_table());
        const Form::Column *pri_col = frm->GetPrimaryKey();
        std::string pri_key;
        if (!pri_col) {
            // TODO:
            //owns_->MakeAutoIncrementIndex(frm, &pri_key);
        } else {
            auto pri_cd = DCHECK_NOTNULL(tuple->schema()->FindOrNull(pri_col->name));
            if (pri_col->auto_increment) {
                // TODO:
                //owns_->MakeAutoIncrementIndex(frm, &pri_key);
            } else {
                MakeKey(tuple, pri_cd, &pri_key);
            }
        }
        auto cf = owns_->GetTableCfOrNull(frm->table_name());

        for (size_t i = 0; i < tuple->schema()->columns_size(); ++i) {
            auto cd = tuple->schema()->column(i);

            std::string key, val;
            if (cd->origin()->key == SQL_KEY) {
                MakeSecondaryIndex(tuple, cd, pri_key, &key);
                batch_.Put(cf, key, "[INFO]");
            } else if (cd->origin()->key == SQL_UNIQUE_KEY) {
                // TODO:
            } else if (cd->origin()->key == SQL_NOT_KEY ||
                       cd->origin()->key == SQL_PRIMARY_KEY) {
                MakeIndex(cd, pri_key, &key);
                MakeValue(tuple, cd, &val);
                batch_.Put(cf, key, val);
            }
        }
        
        return MAI_NOT_SUPPORTED("TODO:");
    }
    
    virtual Error Finialize(bool commit = false) override {
        WriteOptions wr_opts;
        Error rs = owns_->trx_db_->Write(wr_opts, &batch_);
        batch_.Clear();
        return rs;
    }
    
    void MakeKey(const HeapTuple *tuple, const ColumnDescriptor *cd, std::string *buf) {
        
    }
    
    void MakeValue(const HeapTuple *tuple, const ColumnDescriptor *cd, std::string *buf) {
        
    }
    
    void MakeIndex(const ColumnDescriptor *cd, std::string_view pri_key, std::string *buf) {
        
    }
    
    void MakeSecondaryIndex(const HeapTuple *tuple, const ColumnDescriptor *cd,
                            std::string_view pri_key, std::string *buf) {
        
    }

private:
    virtual Error Commit() override { return MAI_NOT_SUPPORTED("XXX:"); }
    virtual Error Rollback() override { return MAI_NOT_SUPPORTED("XXX:"); }
    
    
    
    StorageEngine *owns_;
    const Snapshot *snapshot_;
    WriteBatch batch_;
}; // class StorageEngine::StorageBatch
    
StorageEngine::StorageEngine(TransactionDB *trx_db, StorageKind kind)
    : trx_db_(DCHECK_NOTNULL(trx_db))
    , kind_(kind)
    , next_column_id_(0)
    , next_index_id_(0) {}

StorageEngine::~StorageEngine() {
    for (auto cf : column_families_) {
        trx_db_->ReleaseColumnFamily(cf);
    }
}

Error StorageEngine::Prepare(bool boot) {
    Error rs = trx_db_->GetAllColumnFamilies(&column_families_);
    if (!rs) {
        return rs;
    }
    sys_cf_idx_ = column_families_.size();
    for (size_t i = 0; i < column_families_.size(); ++i) {
        cf_names_.insert({column_families_[i]->name(), i});
        if (column_families_[i]->name() == kDefaultColumnFamilyName) {
            def_cf_idx_ = i;
        }
        if (column_families_[i]->name() == kSysCfName) {
            sys_cf_idx_ = i;
        }
    }

    if (boot) {
        auto iter = cf_names_.find(kSysCfName);
        if (iter != cf_names_.end()) {
            return MAI_CORRUPTION("\'__system__\' column family already exist.");
        }
        
        ColumnFamilyOptions cf_opts;
        ColumnFamily *cf;
        rs = trx_db_->NewColumnFamily(kSysCfName, cf_opts, &cf);
        if (!rs) {
            return rs;
        }
        cf_names_.insert({cf->name(), column_families_.size()});
        sys_cf_idx_ = column_families_.size();
        column_families_.push_back(cf);
        
        WriteOptions wr_opts;
        wr_opts.sync = true;
        
        std::unique_ptr<Transaction> trx(trx_db_->BeginTransaction(wr_opts));
        std::string buf;
        Slice::WriteFixed32(&buf, next_index_id_.load());
        trx->Put(sys_cf(), kSysNextIndexIdKey, buf);
        buf.clear();
        
        Slice::WriteFixed32(&buf, next_column_id_.load());
        trx->Put(sys_cf(), kSysNextColumnIdKey, buf);
        buf.clear();
        
        rs = trx->Commit();
        if (!rs) {
            return rs;
        }
    } else {
        auto iter = cf_names_.find(kSysCfName);
        if (iter == cf_names_.end()) {
            return MAI_CORRUPTION("\'__system__\' column family not found.");
        }
        
        ReadOptions rd_opts;
        std::string buf;
        rs = trx_db_->Get(rd_opts, sys_cf(), kSysNextIndexIdKey, &buf);
        if (!rs) {
            return rs;
        }
        next_index_id_.store(Slice::SetFixed32(buf));
        rs = trx_db_->Get(rd_opts, sys_cf(), kSysNextColumnIdKey, &buf);
        if (!rs) {
            return rs;
        }
        next_column_id_.store(Slice::SetFixed32(buf));
        
        // TODO load metadata
    }
    return Error::OK();
}
    
Error StorageEngine::NewTable(const std::string &/*db_name*/, const Form *form) {
    std::unique_lock<std::shared_mutex> lock(meta_mutex_);
    
    ColumnFamily *cf = nullptr;
    ColumnFamilyOptions cf_opts;
    Error rs = trx_db_->NewColumnFamily(form->table_name(), cf_opts, &cf);
    if (!rs) {
        return rs;
    }
    cf_names_.insert({cf->name(), column_families_.size()});
    column_families_.push_back(cf);
    
    for (size_t i = 0; i < form->columns_size(); ++i) {
        std::shared_ptr<Column> column(new Column);
        
        column->col_id = next_column_id_.fetch_add(1);
        column->owns_cf = cf;
        
        std::string name(form->table_name());
        name.append(".").append(form->column(i)->name);
        
        columns_.insert({name, column});
    }
    
    for (size_t i = 0; i < form->indices_size(); ++i) {
        std::shared_ptr<Index> index(new Index);
        
        index->idx_id = next_index_id_.fetch_add(1);
        index->owns_cf = cf;

        std::string name(form->table_name());
        name.append(".").append(form->index(i)->name);
        
        indices_.insert({name, index});
    }

    return SyncMetadata(form->table_name(), kIds|kColumns|kIndices);
}
    
Error StorageEngine::PutTuple(const HeapTuple *tuple) {
    // TODO:
    return MAI_NOT_SUPPORTED("TODO:");
}

StorageOperation *StorageEngine::NewBatch() {
    switch (kind_) {
        case SQL_COLUMN_STORE:
            return new ColumnStorageBatch(this);
            
        case SQL_ROW_STORE:
            // TODO:
            DLOG(FATAL) << "TODO:";
            return nullptr;
            
        default:
            DLOG(FATAL) << "Noreached";
            break;
    }
    return nullptr;
}
    
Error StorageEngine::SyncMetadata(const std::string &arg, uint32_t flags) {
    WriteOptions wr_opts;
    wr_opts.sync = true;
    std::unique_ptr<Transaction> trx(trx_db_->BeginTransaction(wr_opts));

    if (flags & kIds) {
        std::string buf;

        Slice::WriteFixed32(&buf, next_index_id_.load());
        trx->Put(sys_cf(), kSysNextIndexIdKey, buf);
        buf.clear();
        
        Slice::WriteFixed32(&buf, next_column_id_.load());
        trx->Put(sys_cf(), kSysNextColumnIdKey, buf);
        buf.clear();
    }
    
    if (flags & kColumns) {
        std::map<std::string, std::string> tables;
        for (const auto &pair : columns_) {
            size_t dot = pair.first.find(".");
            std::string table_name = pair.first.substr(0, dot);
            std::string column_name = pair.first.substr(dot + 1);
            
            if (arg.empty() || arg == table_name) {
                auto buf = &tables[table_name];
                
                Slice::WriteString(buf, column_name);
                Slice::WriteString(buf, pair.second->owns_cf->name());
                Slice::WriteVarint32(buf, pair.second->col_id);
            }
        }
        
        for (const auto &pair : tables) {
            std::string key(kSysColumnsKey);
            key.append(".").append(pair.first);
            trx->Put(sys_cf(), key, pair.second);
        }
    }
    
    if (flags & kIndices) {
        std::map<std::string, std::string> tables;
        for (const auto &pair : indices_) {
            size_t dot = pair.first.find(".");
            std::string table_name = pair.first.substr(0, dot);
            std::string index_name = pair.first.substr(dot + 1);
            
            if (arg.empty() || arg == table_name) {
                auto buf = &tables[table_name];
                
                Slice::WriteString(buf, index_name);
                Slice::WriteString(buf, pair.second->owns_cf->name());
                Slice::WriteVarint32(buf, pair.second->idx_id);
            }
        }

        for (const auto &pair : tables) {
            std::string key(kSysColumnsKey);
            key.append(".").append(pair.first);
            trx->Put(sys_cf(), key, pair.second);
        }
    }

    return trx->Commit();
}
    
} // namespace sql
    
} // namespace mai
