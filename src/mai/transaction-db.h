#ifndef MAI_TRANSACTION_DB_H_
#define MAI_TRANSACTION_DB_H_

#include "mai/proxied-db.h"

namespace mai {
    
class Transaction;
struct WriteOptions;
    
struct TransactionDBOptions final {
    
};
    
struct TransactionOptions final {
    
}; // struct TransactionOptions
    
class TransactionDB : public ProxiedDB {
public:
    virtual ~TransactionDB() override {}
    
    static Error Open(const Options &opts,
                      const TransactionDBOptions &txn_db_opts,
                      const std::string name,
                      const std::vector<ColumnFamilyDescriptor> &descriptors,
                      std::vector<ColumnFamily *> *column_families,
                      TransactionDB **result);
    
    virtual Transaction *
    BeginTransaction(const WriteOptions &wr_opts,
                     const TransactionOptions &txn_opts = TransactionOptions{},
                     Transaction *old_txn = nullptr) = 0;
    
    TransactionDB(const TransactionDB &) = delete;
    TransactionDB(TransactionDB &&) = delete;
    void operator = (const TransactionDB &) = delete;
protected:
    TransactionDB(DB *db) : ProxiedDB(db) {}
}; // class TransactionDB
    
} // namespace mai

#endif // MAI_TRANSACTION_DB_H_
