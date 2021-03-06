#ifndef MAI_TXN_OPTIMISM_TRANSACTION_DB_H_
#define MAI_TXN_OPTIMISM_TRANSACTION_DB_H_

#include "base/base.h"
#include "mai/transaction-db.h"

namespace mai {
namespace db {
class DBImpl;
} // namespace db
namespace txn {
    
class OptimismTransactionDB final : public TransactionDB {
public:
    OptimismTransactionDB(const TransactionDBOptions &opts, DB *db);
    virtual ~OptimismTransactionDB() override;

    virtual Transaction *BeginTransaction(const WriteOptions &wr_opts,
                                          const TransactionOptions &txn_opts,
                                          Transaction *old_txn) override;
    
    db::DBImpl *impl() const;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(OptimismTransactionDB);
private:
    TransactionDBOptions const options_;
};
    
} // namespace txn
    
} // namespace mai


#endif // MAI_TXN_OPTIMISM_TRANSACTION_DB_H_
