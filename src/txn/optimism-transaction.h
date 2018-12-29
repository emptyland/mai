#ifndef MAI_TXN_OPTIMISM_TRANSACTION_H_
#define MAI_TXN_OPTIMISM_TRANSACTION_H_

#include "txn/transaction-base.h"
#include "base/base.h"

namespace mai {
namespace db {
class DBImpl;
} // namespace db
namespace txn {

class OptimismTransactionDB;
    
class OptimismTransaction final : public TransactionBase {
public:
    OptimismTransaction(const WriteOptions &opts, OptimismTransactionDB *db);
    virtual ~OptimismTransaction() override;
    
    virtual Error Rollback() override;
    virtual Error Commit() override;
    virtual Error TryLock(ColumnFamily* cf, std::string_view key, bool read_only,
                          bool exclusive, const bool do_validate) override;

    DISALLOW_IMPLICIT_CONSTRUCTORS(OptimismTransaction);
private:
    class Callback;
    
    Error CheckTransactionForConflicts(db::DBImpl *db);
}; // class OptimismTransaction
    
} // namespace txn
    
} // namespace mai

#endif // MAI_TXN_OPTIMISM_TRANSACTION_H_
