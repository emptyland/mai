#ifndef MAI_DB_OPTIMISM_TRANSACTION_H_
#define MAI_DB_OPTIMISM_TRANSACTION_H_

#include "db/write-batch-with-index.h"
#include "base/base.h"
#include "mai/transaction.h"

namespace mai {
    
namespace db {

class OptimismTransactionDB;
    
class OptimismTransaction final : public Transaction {
public:
    OptimismTransaction(const WriteOptions &opts,
                        OptimismTransactionDB *db);
    virtual ~OptimismTransaction() override;
    
    void Reinitialize(const WriteOptions &opts, OptimismTransactionDB *db);
    
    virtual Error Rollback() override;
    virtual Error Commit() override;
    virtual Error Put(const WriteOptions &opts, ColumnFamily *cf,
                      std::string_view key, std::string_view value) override;
    virtual Error Delete(const WriteOptions &opts, ColumnFamily *cf,
                         std::string_view key) override;
    virtual Error Get(const ReadOptions &opts, ColumnFamily *cf,
                      std::string_view key, std::string *value) override;
    virtual Error GetForUpdate(const ReadOptions &opts, ColumnFamily *cf,
                               std::string_view key, std::string *value,
                               bool exclusive, const bool do_validate) override;
    virtual Iterator* GetIterator(const ReadOptions& opts,
                                  ColumnFamily* cf) override;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(OptimismTransaction);
private:
    WriteOptions options_;
    OptimismTransactionDB *db_;
}; // class OptimismTransaction
    
} // namespace db
    
} // namespace mai

//OptimismTransaction


#endif // MAI_DB_OPTIMISM_TRANSACTION_H_
