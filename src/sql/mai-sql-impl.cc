#include "sql/mai-sql-impl.h"
#include "sql/mai-sql-connection-impl.h"
#include "sql/storage-engine-factory.h"
#include "sql/storage-engine.h"
#include "sql/form-schema.h"
#include "sql/types.h"
#include "sql/config.h"
#include "base/io-utils.h"
#include "glog/logging.h"

namespace mai {
    
const char kPrimaryDatabaseName[] = "primary";
const char kPrimaryDatabaseEngine[] = "ColumnMaiDB";
static const sql::StorageKind kPrimaryDatabaseKind = sql::SQL_COLUMN_STORE;
    
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
    engine_factory_.reset(new StorageEngineFactory(abs_meta_dir_, env_));
}

/*virtual*/ MaiSQLImpl::~MaiSQLImpl() {
    DCHECK(dummy_->next_ == dummy_ && dummy_->prev_ == dummy_);
    delete dummy_;
    
    for (const auto &pair : dbs_) {
        delete pair.second;
    }
}
    
Error MaiSQLImpl::Open(bool boot) {
    if (abs_meta_dir_.empty()) {
        return MAI_CORRUPTION("Invalid MaiSQLOptions::meta_dir.");
    }
    if (abs_data_dir_.empty()) {
        return MAI_CORRUPTION("Invalid MaiSQLOptions::data_dir.");
    }
    
    Error rs;
    if (boot) {
        rs = form_schema_->NewDatabase(kPrimaryDatabaseName,
                                       kPrimaryDatabaseEngine,
                                       kPrimaryDatabaseKind);
        if (!rs) {
            return rs;
        }
    } else {
        std::string file_name(Files::CurrentFileName(abs_meta_dir_));
        std::string meta_file_number;
        rs = base::FileReader::ReadAll(file_name, &meta_file_number, env_);
        if (!rs) {
            return rs;
        }
        rs = form_schema_->Recovery(atoll(meta_file_number.c_str()));
        if (!rs) {
            return rs;
        }
    }
    
    for (const auto &pair : form_schema_->GetDatabases()) {
        StorageEngine *engine = nullptr;
        rs = engine_factory_->NewEngine(abs_data_dir_, pair.first,
                                        pair.second.engine_name,
                                        pair.second.storage_kind,
                                        &engine);
        if (rs.ok()) {
            rs = engine->Prepare(boot);
        }
        if (!rs) {
            return rs;
        }
        dbs_.insert({pair.first, engine});
    }

    initialized_ = true;
    return Error::OK();
}
    
/*virtual*/ Error
MaiSQLImpl::GetConnection(const std::string &conn_str,
                          std::unique_ptr<MaiSQLConnection> *result,
                          base::Arena *arena,
                          MaiSQLConnection *old) {
    bool ownership = false;
    if (!arena) {
        arena = conn_zone_.NewArena(0);
        ownership = true;
    }

    Error rs;
    if (old) {
        auto conn = down_cast<MaiSQLConnectionImpl>(old);
        if (conn->owns_impl_ != this) {
            return MAI_CORRUPTION("Differect instance.");
        }
        rs = conn->Reinitialize(kPrimaryDatabaseName, arena, ownership);
        if (rs.ok()) {
            result->reset(old);
        }
    } else {
        auto conn = new MaiSQLConnectionImpl(conn_str,
                                             next_conn_id_.fetch_add(1),
                                             this, arena, ownership);
        rs = conn->SwitchDB(kPrimaryDatabaseName);
        if (rs.ok()) {
            result->reset(conn);
        }
        InsertConnection(conn);
    }
    return rs;
}
    
void MaiSQLImpl::RemoveConnection(MaiSQLConnectionImpl *conn) {
    std::lock_guard<std::mutex> lock(dummy_mutex_);
    conn->prev_->next_ = conn->next_;
    conn->next_->prev_ = conn->prev_;
#if defined(DEBUG) || defined(_DEBUG)
    conn->next_ = nullptr;
    conn->prev_ = nullptr;
#endif
}

void MaiSQLImpl::InsertConnection(MaiSQLConnectionImpl *conn) {
    std::lock_guard<std::mutex> lock(dummy_mutex_);
    conn->next_ = dummy_;
    auto *prev = dummy_->prev_;
    conn->prev_ = prev;
    prev->next_ = conn;
    dummy_->prev_ = conn;
}
    
} // namespace sql
    
} // namespace mai
