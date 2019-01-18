#ifndef MAI_MAI_SQL_H_
#define MAI_MAI_SQL_H_

#include "mai/error.h"
#include <memory>

namespace mai {
namespace base {
class Arena;
} // namespace base

class MaiSQLConnection;
    
extern const char kPrimaryDatabaseName[];

class MaiSQL {
public:
    MaiSQL() {}
    virtual ~MaiSQL() {}
    
    virtual Error GetConnection(const std::string &conn_str,
                                std::unique_ptr<MaiSQLConnection> *result,
                                base::Arena *arena = nullptr,
                                MaiSQLConnection *old = nullptr) = 0;

    MaiSQL(const MaiSQL &) = delete;
    void operator = (const MaiSQL &) = delete;
}; // class MaiSql


class MaiSQLConnection {
public:
    virtual ~MaiSQLConnection() {}
    
    MaiSQL *owns() const { return owns_; }
    
    std::string conn_str() const { return conn_str_; }
    
    uint32_t thread_id() const { return thread_id_; }
    
    void set_thread_id(uint32_t id) { thread_id_ = id; }
    
    virtual uint32_t conn_id() const = 0;
    
    // Internal method for server.
    virtual base::Arena *arena() const = 0;
    
    virtual std::string GetDB() const = 0;
    
    virtual Error SwitchDB(const std::string &name) = 0;

    virtual Error Execute(const std::string &sql) = 0;

    MaiSQLConnection(const MaiSQLConnection &) = delete;
    void operator = (const MaiSQLConnection &) = delete;
protected:
    MaiSQLConnection(const std::string &conn_str, MaiSQL *owns)
        : owns_(owns)
        , conn_str_(conn_str) {}
    
    MaiSQL *const owns_;
    std::string conn_str_;
    uint32_t thread_id_ = 0;
};

} // namespace mai


#endif // MAI_MAI_SQL_H_
