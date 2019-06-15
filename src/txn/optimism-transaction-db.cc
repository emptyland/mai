#include "txn/optimism-transaction-db.h"
#include "txn/optimism-transaction.h"
#include "db/db-impl.h"

namespace mai {
    
namespace txn {
    
OptimismTransactionDB::OptimismTransactionDB(const TransactionDBOptions &opts,
                                             DB *db)
    : TransactionDB(db)
    , options_(opts) {}
    
/*virtual*/ OptimismTransactionDB::~OptimismTransactionDB() {
}
    
/*virtual*/ Transaction *
OptimismTransactionDB::BeginTransaction(const WriteOptions &wr_opts,
                                        const TransactionOptions &txn_opts,
                                        Transaction *old_txn) {
    Transaction *txn = nullptr;
    if (old_txn) {
        down_cast<OptimismTransaction>(old_txn)->Reinitialize(wr_opts, this);
        txn = old_txn;
    } else {
        txn = new OptimismTransaction(wr_opts, this);
    }
    return txn;
}

db::DBImpl *OptimismTransactionDB::impl() const {
    return down_cast<db::DBImpl>(GetDB());
}
    
} // namespace txn
    
} // namespace mai
