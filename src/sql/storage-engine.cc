#include "sql/storage-engine.h"
#include "base/slice.h"
#include "mai/transaction.h"

namespace mai {

namespace sql {
    
using ::mai::base::Slice;

/*static*/ const char StorageEngine::kSysCfName[] = "__system__";
/*static*/ const char StorageEngine::kSysNextIndexIdKey[] = "next_index_id";
/*static*/ const char StorageEngine::kSysNextColumnIdKey[] = "next_column_id";
    
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
    
Error StorageEngine::NewTable(const std::string &db_name, const Form *form) {
    
    return MAI_NOT_SUPPORTED("TODO:");
}
    
} // namespace sql
    
} // namespace mai
