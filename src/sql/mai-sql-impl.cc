#include "sql/mai-sql-impl.h"
#include "sql/mai-sql-connection-impl.h"
#include "sql/form-schema.h"
#include "glog/logging.h"

namespace mai {
    
const char kPrimaryDatabaseName[] = "primary";
    
namespace sql {
    
MaiSQLImpl::MaiSQLImpl(const MaiSQLOptions &opts)
    : conn_zone_(opts.env->GetLowLevelAllocator(), opts.zone_max_cache_size)
    , dummy_(new MaiSQLConnectionImpl("", 0, this, nullptr, false))
    , next_conn_id_(1)
    , env_(opts.env) {
    if (!opts.data_dir.empty()) {
        abs_data_dir_ = env_->GetAbsolutePath(opts.data_dir);
    }
    if (!opts.meta_dir.empty()) {
        abs_meta_dir_ = env_->GetAbsolutePath(opts.meta_dir);
    }
    form_schema_.reset(new FormSchemaSet(abs_meta_dir_,
                                         abs_data_dir_, env_));
}

/*virtual*/ MaiSQLImpl::~MaiSQLImpl() {
    DCHECK(dummy_->next_ == dummy_ && dummy_->prev_ == dummy_);
    delete dummy_;
}
    
Error MaiSQLImpl::Open(bool init) {
    if (abs_meta_dir_.empty()) {
        return MAI_CORRUPTION("Invalid MaiSQLOptions::meta_dir.");
    }
    if (abs_data_dir_.empty()) {
        return MAI_CORRUPTION("Invalid MaiSQLOptions::data_dir.");
    }
    
    Error rs;
    if (init) {
        rs = form_schema_->NewDatabase(kPrimaryDatabaseName, "maidb.column");
        if (!rs) {
            return rs;
        }
    }
    // TODO:
    
    initialized_ = true;
    return Error::OK();
}
    
/*virtual*/ Error
MaiSQLImpl::GetConnection(const std::string &conn_str,
                          std::unique_ptr<MaiSQLConnection> *result,
                          base::Arena *arena,
                          MaiSQLConnection *old) {
    return MAI_NOT_SUPPORTED("TODO:");
}
    
} // namespace sql
    
} // namespace mai
