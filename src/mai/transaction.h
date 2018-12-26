#ifndef MAI_TRANSACTION_H_
#define MAI_TRANSACTION_H_

#include "mai/options.h"
#include "mai/error.h"
#include <atomic>
#include <string>
#include <string_view>

namespace mai {

class ColumnFamily;
class Iterator;
    
using TxnID = uint64_t;
    
class Transaction {
public:
    enum State {
        STARTED,
        AWAITING_PREPARE,
        PREPARED,
        AWAITING_COMMIT,
        COMMITED,
        AWAITING_ROLLBACK,
        ROLLEDBACK,
        LOCKS_STOLEN,
    };

    virtual ~Transaction() {}
    
    std::string name() const { return name_; }
    
    void set_name(const std::string &name) { name_ = name; }
    
    TxnID id() const { return id_; }
    
    State state() const { return state_.load(); }
    
    virtual Error Rollback() = 0;
    
    virtual Error Commit() = 0;
    
    virtual Error Put(const WriteOptions &opts, ColumnFamily *cf,
                      std::string_view key, std::string_view value) = 0;
    
    virtual Error Delete(const WriteOptions &opts, ColumnFamily *cf,
                         std::string_view key) = 0;
    
    virtual Error Get(const ReadOptions &opts, ColumnFamily *cf,
                      std::string_view key, std::string *value) = 0;
    
    virtual Error GetForUpdate(const ReadOptions &opts, ColumnFamily *cf,
                               std::string_view key, std::string *value,
                               bool exclusive = true,
                               const bool do_validate = true) = 0;
    
    virtual Iterator* GetIterator(const ReadOptions& opts,
                                  ColumnFamily* cf) = 0;
    
    Transaction(const Transaction &) = delete;
    Transaction(Transaction &&) = delete;
    void operator = (const Transaction &) = delete;
protected:
    Transaction() : state_(STARTED) {}
    
    std::atomic<State> state_;
    std::string name_;
    TxnID id_;
}; // class Transaction
    
} // namespace mai


#endif // MAI_TRANSACTION_H_
