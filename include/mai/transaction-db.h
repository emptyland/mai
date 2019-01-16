#ifndef MAI_TRANSACTION_DB_H_
#define MAI_TRANSACTION_DB_H_

#include "mai/proxied-db.h"

namespace mai {
    
class Transaction;
struct WriteOptions;
    
struct TransactionDBOptions final {
    
    bool optimism = true;
    
    int64_t max_num_locks = -1;
    
    int64_t transaction_lock_timeout = 1000;  // 1 second
    
    int64_t default_lock_timeout = 1000;  // 1 second
    
    size_t num_stripes = 16;
    
    uint32_t max_num_deadlocks = 50;
    
}; // struct TransactionDBOptions
    
struct TransactionOptions final {
    
    bool deadlock_detect = false;
    
    int64_t lock_timeout = 1000;
    
    int64_t expiration = -1;
    
    int64_t deadlock_detect_depth = 50;
    
    size_t max_write_batch_size = 0;
    
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
