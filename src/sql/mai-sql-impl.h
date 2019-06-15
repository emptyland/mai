#ifndef MAI_SQL_MAI_SQL_IMPL_H_
#define MAI_SQL_MAI_SQL_IMPL_H_

#include "base/zone.h"
#include "mai-sql/mai-sql.h"
#include "mai-sql/options.h"
#include "mai/env.h"
#include <mutex>
#include <unordered_map>

namespace mai {
    
namespace sql {
    
class MaiSQLConnectionImpl;
class FormSchemaSet;
class StorageEngineFactory;
class StorageEngine;
    
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
    
    friend class MaiSQLConnectionImpl;
    DISALLOW_IMPLICIT_CONSTRUCTORS(MaiSQLImpl);
private:
    StorageEngine *GetDatabaseEngineOrNull(const std::string &db_name) const {
        auto iter = dbs_.find(db_name);
        return iter == dbs_.end() ? nullptr : iter->second;
    }
    
    void RemoveConnection(MaiSQLConnectionImpl *conn);
    void InsertConnection(MaiSQLConnectionImpl *conn);
    
    base::Zone conn_zone_;
    ConnectionImpl *dummy_;
    std::mutex dummy_mutex_;
    std::atomic<uint32_t> next_conn_id_;
    Env *env_;
    std::string abs_meta_dir_;
    std::string abs_data_dir_;
    
    std::unique_ptr<FormSchemaSet> form_schema_;
    std::mutex form_schema_mutex_;
    
    std::unique_ptr<StorageEngineFactory> engine_factory_;
    std::unordered_map<std::string, StorageEngine *> dbs_;

    bool initialized_ = false;
}; // class MaiSQLImpl
    
} // namespace sql
    
} // namespace mai


#endif // MAI_SQL_MAI_SQL_IMPL_H_
