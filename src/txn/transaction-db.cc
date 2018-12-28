#include "txn/optimism-transaction-db.h"
#include "txn/optimism-transaction.h"
#include "mai/db.h"

namespace mai {
    
/*static*/ Error
TransactionDB::Open(const Options &opts,
                    const TransactionDBOptions &txn_db_opts,
                    const std::string name,
                    const std::vector<ColumnFamilyDescriptor> &descriptors,
                    std::vector<ColumnFamily *> *column_families,
                    TransactionDB **result) {
    DB *db = nullptr;
    Error rs = DB::Open(opts, name, descriptors, column_families, &db);
    if (!rs) {
        return rs;
    }

    if (txn_db_opts.optimism) {
        *result = new txn::OptimismTransactionDB(txn_db_opts, db);
    } else {
        delete db;
        return MAI_NOT_SUPPORTED("TODO:");
    }
    return Error::OK();
}
    
} // namespace mai
