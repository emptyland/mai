#ifndef MAI_SQL_MAI_SQL_CONNECTION_IMPL_H_
#define MAI_SQL_MAI_SQL_CONNECTION_IMPL_H_

#include "base/arena.h"
#include "base/base.h"
#include "mai-sql/mai-sql.h"

namespace mai {
    
namespace sql {
    
class MaiSQLImpl;
class StorageEngine;

class Statement;
class CreateTable;
    
class MaiSQLConnectionImpl final : public MaiSQLConnection {
public:
    MaiSQLConnectionImpl(const std::string &conn_str,
                         const uint32_t conn_id,
                         MaiSQLImpl *owns,
                         StorageEngine *engine,
                         base::Arena *arena,
                         bool arena_ownership);
    virtual ~MaiSQLConnectionImpl() override;
    
    //DEF_PTR_GETTER(MaiSQLImpl, owns_impl);
    
    Error Reinitialize(const std::string &init_db_name, base::Arena *arena,
                       bool arena_ownership);
    
    virtual uint32_t conn_id() const override;
    virtual base::Arena *arena() const override;
    virtual std::string GetDB() const override;
    virtual Error SwitchDB(const std::string &name) override;
    virtual Error Execute(const std::string &sql) override;
    virtual ResultSet *Query(const std::string &sql) override;
    virtual PreparedStatement *Prepare(const std::string &sql) override;

    friend class MaiSQLImpl;
    DISALLOW_IMPLICIT_CONSTRUCTORS(MaiSQLConnectionImpl);
private:
    ResultSet *ExecuteStatement(const Statement *stmt);
    Error ExecuteCreateTable(const CreateTable *ast);

    std::string db_name_;
    const uint32_t conn_id_;
    MaiSQLImpl *const owns_impl_;
    StorageEngine *engine_;
    base::Arena *arena_;
    bool arena_ownership_;
    
    

    MaiSQLConnectionImpl *next_ = this;
    MaiSQLConnectionImpl *prev_ = this;
}; // class MaiSQLConnectionImpl
    
} // namespace sql
    
} // namespace mai

#endif // MAI_SQL_MAI_SQL_CONNECTION_IMPL_H_
