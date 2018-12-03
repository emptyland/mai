#ifndef MAI_DB_COMPACTION_IMPL_H_
#define MAI_DB_COMPACTION_IMPL_H_

#include "base/reference-count.h"
#include "db/compaction.h"
#include <string_view>

namespace mai {
namespace core {
class InternalKeyComparator;
class MemoryTable;
} // namespace core
namespace db {
    
class TableCache;

class CompactionImpl : public Compaction {
public:
    CompactionImpl(const std::string abs_db_path,
                   const core::InternalKeyComparator *ikcmp,
                   TableCache *table_cache, ColumnFamilyImpl *cfd);
    virtual ~CompactionImpl();
    
    virtual Error Run(table::TableBuilder *builder,
                      CompactionResult *result) override;
    
private:
    using MemoryTable = ::mai::core::MemoryTable;
    
    Error IsBaseLevelForKey(int start_level, std::string_view key,
                            bool *may_exists);
    bool IsBaseMemoryForKey(std::string_view key) const;
    
    const std::string abs_db_path_;
    const core::InternalKeyComparator *const ikcmp_;
    TableCache *const table_cache_;

    std::vector<base::intrusive_ptr<MemoryTable>> in_mem_;
}; // class CompactionImpl
    
} // namespace db

} // namespace mai


#endif // MAI_DB_COMPACTION_IMPL_H_
