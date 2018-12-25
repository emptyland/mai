#include "db/optimism-transaction.h"
#include "db/optimism-transaction-db.h"
#include "db/db-impl.h"
#include "db/write-batch-with-index.h"
#include "mai/iterator.h"
#include "glog/logging.h"

namespace mai {
    
namespace db {

OptimismTransaction::OptimismTransaction(const WriteOptions &opts,
                                         OptimismTransactionDB *db) {
    Reinitialize(opts, db);
}

/*virtual*/ OptimismTransaction::~OptimismTransaction() {
}

void OptimismTransaction::Reinitialize(const WriteOptions &opts,
                                       OptimismTransactionDB *db) {
    options_ = opts;
    db_      = DCHECK_NOTNULL(db);
}

/*virtual*/ Error OptimismTransaction::Rollback() {
    return MAI_NOT_SUPPORTED("TODO:");
}

/*virtual*/ Error OptimismTransaction::Commit() {
    return MAI_NOT_SUPPORTED("TODO:");
}

/*virtual*/ Error
OptimismTransaction::Put(const WriteOptions &opts, ColumnFamily *cf,
                         std::string_view key, std::string_view value) {
    return MAI_NOT_SUPPORTED("TODO:");
}
    
/*virtual*/ Error
OptimismTransaction::Delete(const WriteOptions &opts, ColumnFamily *cf,
                            std::string_view key) {
    return MAI_NOT_SUPPORTED("TODO:");
}
    
/*virtual*/ Error OptimismTransaction::Get(const ReadOptions &opts,
                                           ColumnFamily *cf,
                                           std::string_view key,
                                           std::string *value) {
    return MAI_NOT_SUPPORTED("TODO:");
}
    
/*virtual*/ Error
OptimismTransaction::GetForUpdate(const ReadOptions &opts, ColumnFamily *cf,
                                  std::string_view key, std::string *value,
                                  bool exclusive, const bool do_validate) {
    return MAI_NOT_SUPPORTED("TODO:");
}
    
/*virtual*/ Iterator *OptimismTransaction::GetIterator(const ReadOptions& opts,
                                                       ColumnFamily* cf)  {
    return Iterator::AsError(MAI_NOT_SUPPORTED("TODO:"));
}

} // namespace db
    
} // namespace mai
