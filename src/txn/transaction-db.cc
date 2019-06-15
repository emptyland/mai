#include "txn/optimism-transaction-db.h"
#include "txn/optimism-transaction.h"
#include "txn/pessimistic-transaction-db.h"
#include "txn/pessimistic-transaction.h"
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
        auto txn_db = new txn::PessimisticTransactionDB(txn_db_opts, db);
        if (column_families && !column_families->empty()) {
            for (auto cf : *column_families) {
                txn_db->AddColumnFamily(cf->id());
            }
        } else {
            std::vector<ColumnFamily *> cfs;
            rs = db->GetAllColumnFamilies(&cfs);
            if (!rs) {
                delete txn_db;
                delete db;
                return rs;
            }
            for (auto cf : cfs) {
                txn_db->AddColumnFamily(cf->id());
                db->ReleaseColumnFamily(cf);
            }
        }
        *result =txn_db;
    }
    return Error::OK();
}
    
} // namespace mai
