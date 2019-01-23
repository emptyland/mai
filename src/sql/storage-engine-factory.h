#ifndef MAI_SQL_STORAGE_ENGINE_FACTORY_H_
#define MAI_SQL_STORAGE_ENGINE_FACTORY_H_

#include "sql/types.h"
#include "base/base.h"
#include "mai/error.h"
#include <string>
#include <vector>

namespace mai {
class Env;
class DB;
class TransactionDB;
namespace sql {
    
class StorageEngine;

class StorageEngineFactory final {
public:
    StorageEngineFactory(const std::string &abs_meta_dir, Env *env);

    Error NewEngine(const std::string &abs_data_dir, const std::string &db_name,
                    const std::string &engine_name, StorageKind kind,
                    StorageEngine **result);
    
    void GetSupportedEngines(std::vector<std::string> *names) const;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(StorageEngineFactory);
private:
    std::string const abs_meta_dir_;
    Env *const env_;
    
    static const char *const kSupportedEngines[];
}; // class EngineFactory

} // namespace sql
    
} // namespace mai

#endif // MAI_SQL_STORAGE_ENGINE_FACTORY_H_
