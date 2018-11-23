#ifndef MAI_DB_COMPACTION_IMPL_H_
#define MAI_DB_COMPACTION_IMPL_H_

#include "db/compaction.h"
#include <string_view>

namespace mai {
namespace core {
class InternalKeyComparator;
} // namespace core
namespace db {
    
class TableCache;

class CompactionImpl : public Compaction {
public:
    CompactionImpl(const std::string abs_db_path,
                   const core::InternalKeyComparator *ikcmp,
                   TableCache *table_cache, ColumnFamilyImpl *cfd);
    virtual ~CompactionImpl();
    
    virtual Error Run(table::TableBuilder *builder) override;
    
private:
    bool EnsureKeyNotExists(int start_level, std::string_view key);
    
    const std::string abs_db_path_;
    const core::InternalKeyComparator *const ikcmp_;
    TableCache *const table_cache_;
}; // class CompactionImpl
    
} // namespace db

} // namespace mai


#endif // MAI_DB_COMPACTION_IMPL_H_
