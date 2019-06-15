#include "sql/storage-engine.h"
#include "sql/form-schema.h"
#include "sql/heap-tuple.h"
#include "base/slice.h"
#include "mai/transaction.h"
#include "mai/write-batch.h"
#include "mai/iterator.h"

namespace mai {

namespace sql {
    
using ::mai::base::Slice;

static const char kSysCfName[] = "__system__";
static const char kSysTablesKey[] = "tables";
    
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
    , kind_(kind) {}

StorageEngine::~StorageEngine() {
    for (auto cf : column_families_) {
        trx_db_->ReleaseColumnFamily(cf);
    }
    delete trx_db_;
    trx_db_ = nullptr;
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
        // TODO:
        rs = trx->Commit();
        if (!rs) {
            return rs;
        }
    } else {
        auto iter = cf_names_.find(kSysCfName);
        if (iter == cf_names_.end()) {
            return MAI_CORRUPTION("\'__system__\' column family not found.");
        }
                
        std::unique_ptr<Iterator> tbit(trx_db_->NewIterator({}, sys_cf()));
        if (tbit->error().fail()) {
            return tbit->error();
        }
        std::string prefix(kSysTablesKey);
        prefix.append(".");
        for (tbit->Seek(prefix); tbit->Valid(); tbit->Next()) {
            if (tbit->key().find(prefix) != 0) {
                break;
            }
            auto name = tbit->key();
            name.remove_prefix(prefix.length());
            
            TableMetadata *tbmd = &item_owns_[std::string(name)];
            base::BufferReader rd(tbit->value());
            tbmd->auto_increment = rd.ReadVarint64();
            
            while (!rd.Eof()) {
                uint32_t id = rd.ReadVarint32();
                bool column = rd.ReadVarint32();
                std::string cf_name(rd.ReadString());
                
                auto cfit = cf_names_.find(cf_name);
                DCHECK(cfit != cf_names_.end());
                
                items_.insert({id, {column, column_families_[cfit->second]}});
                tbmd->items.insert(id);
            }
        }
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
        auto id = form->column(i)->cid;
        items_.insert({id, {true, cf}});
        item_owns_[form->table_name()].items.insert(id);
    }
    
    for (size_t i = 0; i < form->indices_size(); ++i) {
        auto id = form->index(i)->iid;
        items_.insert({id, {false, cf}});
        item_owns_[form->table_name()].items.insert(id);
    }

    return SyncMetadata(form->table_name());
}
    
Error StorageEngine::PutTuple(const HeapTuple *tuple) {
    // TODO:
    return MAI_NOT_SUPPORTED("TODO:");
}

StorageOperation *StorageEngine::NewWriter() {
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
    
Error StorageEngine::SyncMetadata(const std::string &table_name) {
    WriteOptions wr_opts;
    wr_opts.sync = true;
    std::unique_ptr<Transaction> trx(trx_db_->BeginTransaction(wr_opts));
    
    std::map<std::string, std::string> bufs;
    if (table_name.empty()) {
        for (const auto &owns : item_owns_) {
            auto buf = &bufs[owns.first];
            Slice::WriteVarint64(buf, owns.second.auto_increment.load());
            for (const auto id : owns.second.items) {
                auto it = items_.find(id);
                DCHECK(it != items_.end());

                Slice::WriteVarint32(buf, it->first);
                Slice::WriteVarint32(buf, it->second.column);
                Slice::WriteString(buf, it->second.owns_cf->name());
            }
        }
    } else {
        auto buf = &bufs[table_name];
        auto owns_it = item_owns_.find(table_name);
        DCHECK(owns_it != item_owns_.end());
        
        Slice::WriteVarint64(buf, owns_it->second.auto_increment.load());
        for (const auto id : owns_it->second.items) {
            auto it = items_.find(id);
            DCHECK(it != items_.end());

            Slice::WriteVarint32(buf, it->first);
            Slice::WriteVarint32(buf, it->second.column);
            Slice::WriteString(buf, it->second.owns_cf->name());
        }
    }
    
    for (const auto &buf : bufs) {
        std::string key(kSysTablesKey);
        key.append(".").append(buf.first);
        trx->Put(sys_cf(), key, buf.second);
    }

    return trx->Commit();
}
    
} // namespace sql
    
} // namespace mai
