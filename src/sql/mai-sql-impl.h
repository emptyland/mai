#ifndef MAI_SQL_MAI_SQL_IMPL_H_
#define MAI_SQL_MAI_SQL_IMPL_H_

#include "base/zone.h"
#include "mai-sql/mai-sql.h"
#include "mai-sql/options.h"
#include "mai/env.h"
#include <mutex>

namespace mai {
    
namespace sql {
    
class MaiSQLConnectionImpl;
class FormSchemaSet;
    
class MaiSQLImpl final : public MaiSQL {
public:
    using ConnectionImpl = MaiSQLConnectionImpl;
    
    MaiSQLImpl(const MaiSQLOptions &opts);
    virtual ~MaiSQLImpl() override;
    
    Error Open(bool init);
    
    base::Zone *zone() { return &conn_zone_; }
    
    virtual Error
    GetConnection(const std::string &conn_str,
                  std::unique_ptr<MaiSQLConnection> *result,
                  base::Arena *arena,
                  MaiSQLConnection *old) override;
    
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(MaiSQLImpl);
private:
    base::Zone conn_zone_;
    ConnectionImpl *dummy_;
    std::atomic<uint32_t> next_conn_id_;
    Env *env_;
    std::string abs_meta_dir_;
    std::string abs_data_dir_;
    
    std::unique_ptr<FormSchemaSet> form_schema_;
    
    std::mutex dummy_mutex_;
    bool initialized_ = false;
}; // class MaiSQLImpl
    
} // namespace sql
    
} // namespace mai


#endif // MAI_SQL_MAI_SQL_IMPL_H_
