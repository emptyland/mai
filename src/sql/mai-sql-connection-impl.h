#ifndef MAI_SQL_MAI_SQL_CONNECTION_IMPL_H_
#define MAI_SQL_MAI_SQL_CONNECTION_IMPL_H_

#include "base/arena.h"
#include "base/base.h"
#include "mai-sql/mai-sql.h"

namespace mai {
    
namespace sql {
    
class MaiSQLImpl;
    
class MaiSQLConnectionImpl final : public MaiSQLConnection {
public:
    MaiSQLConnectionImpl(const std::string &conn_str,
                         const uint32_t conn_id,
                         MaiSQLImpl *owns,
                         base::Arena *arena,
                         bool arena_ownership);
    virtual ~MaiSQLConnectionImpl() override;
    
    void Reinitialize(base::Arena *arena);
    
    virtual uint32_t conn_id() const override;
    virtual base::Arena *arena() const override;
    virtual std::string GetDB() const override;
    virtual Error SwitchDB(const std::string &name) override;
    virtual Error Execute(const std::string &sql) override;

    friend class MaiSQLImpl;
private:
    const uint32_t conn_id_;
    MaiSQLImpl *const owns_impl_;
    base::Arena *const arena_;
    bool const arena_ownership_;

    MaiSQLConnectionImpl *next_ = this;
    MaiSQLConnectionImpl *prev_ = this;
}; // class MaiSQLConnectionImpl
    
} // namespace sql
    
} // namespace mai

#endif // MAI_SQL_MAI_SQL_CONNECTION_IMPL_H_
