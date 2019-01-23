#include "sql/storage-engine-factory.h"
#include "sql/storage-engine.h"
#include "mai/transaction-db.h"
#include "mai/db.h"
#include "glog/logging.h"

namespace mai {
    
namespace sql {
    
const char *const StorageEngineFactory::kSupportedEngines[] = {
    "MaiDB",
};
    
StorageEngineFactory::StorageEngineFactory(const std::string &abs_meta_dir, Env *env)
    : abs_meta_dir_(abs_meta_dir)
    , env_(DCHECK_NOTNULL(env)) {
}
    
Error StorageEngineFactory::NewEngine(const std::string &abs_data_dir,
                                      const std::string &db_name,
                                      const std::string &engine_name,
                                      StorageKind kind,
                                      StorageEngine **result) {
    std::string test_name(abs_data_dir);
    test_name.append("/").append(db_name).append("/CURRENT");
    
    Options opts;
    TransactionDBOptions trx_opts;
    
    std::string name(abs_data_dir);
    name.append("/").append(db_name);
    TransactionDB *db = nullptr;
    
    opts.env = env_;
    Error rs = env_->FileExists(test_name);
    if (!rs) {
        // Not exist
        opts.create_if_missing = true;
        rs = TransactionDB::Open(opts, trx_opts, name, {}, nullptr, &db);
    } else {
        // Exist
        opts.create_if_missing = false;
        rs = TransactionDB::Open(opts, trx_opts, name, {}, nullptr, &db);
    }
    if (rs.ok()) {
        *result = new StorageEngine(db, kind);
    }
    return rs;
}
    
void StorageEngineFactory::GetSupportedEngines(std::vector<std::string> *names) const {
    for (size_t i = 0; i < arraysize(kSupportedEngines); ++i) {
        names->push_back(kSupportedEngines[i]);
    }
}
    
} // namespace sql
    
} // namespace mai
