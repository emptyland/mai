#include "db/optimism-transaction-db.h"
#include "db/db-impl.h"

namespace mai {
    
namespace db {
    
OptimismTransactionDB::OptimismTransactionDB(const TransactionDBOptions &opts,
                                             DB *db)
    : TransactionDB(db)
    , options_(opts) {}
    
/*virtual*/ OptimismTransactionDB::~OptimismTransactionDB() {
}

/*virtual*/ Error
OptimismTransactionDB::Put(const WriteOptions &opts, ColumnFamily *cf,
                           std::string_view key, std::string_view value) {
    WriteBatch batch;
    batch.Put(cf, key, value);
    return Write(opts, &batch);
}
    
/*virtual*/ Error
OptimismTransactionDB::Delete(const WriteOptions &opts, ColumnFamily *cf,
                              std::string_view key) {
    WriteBatch batch;
    batch.Delete(cf, key);
    return Write(opts, &batch);
}
    
/*virtual*/ Error
OptimismTransactionDB::Write(const WriteOptions& opts, WriteBatch* updates) {
    return MAI_NOT_SUPPORTED("");
}
    
/*virtual*/ Transaction *
OptimismTransactionDB::BeginTransaction(const WriteOptions &wr_opts,
                                        const TransactionOptions &txn_opts,
                                        Transaction *old_txn) {
    return nullptr;
}

DBImpl *OptimismTransactionDB::impl() const {
    return down_cast<DBImpl>(GetDB());
}
    
} // namespace db
    
} // namespace mai
