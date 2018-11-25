#ifndef MAI_DB_FACTORY_H_
#define MAI_DB_FACTORY_H_

#include "base/hash.h"
#include "base/base.h"
#include "mai/error.h"

namespace mai {
class RandomAccessFile;
class WritableFile;
namespace core {
class MemoryTable;
class InternalKeyComparator;
} // namespace core
namespace table {
class TableReader;
class TableBuilder;
} // namespace table
namespace db {
class Compaction;
class TableCache;
class ColumnFamilyImpl;

class Factory {
public:
    Factory() {}
    virtual ~Factory() {}
    
    virtual core::MemoryTable *
    NewMemoryTable(const core::InternalKeyComparator *ikcmp, bool unordered,
                   size_t initial_slots) = 0;
    
    virtual Error
    NewTableReader(const core::InternalKeyComparator *ikcmp, bool unordered,
                   RandomAccessFile *file, uint64_t file_size,
                   bool checksum_verify,
                   std::unique_ptr<table::TableReader> *result) = 0;
    
    virtual table::TableBuilder *
    NewTableBuilder(const core::InternalKeyComparator *ikcmp, bool unordered,
                    WritableFile *file, uint64_t block_size, int n_restart,
                    size_t max_hash_slots) = 0;
    
    virtual Compaction *
    NewCompaction(const std::string abs_db_path,
                  const core::InternalKeyComparator *ikcmp,
                  TableCache *table_cache, ColumnFamilyImpl *cfd) = 0;
    
    static Factory *NewDefault();
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Factory);
}; // class Factory


} // namespace db

} // namespace mai

#endif // MAI_DB_FACTORY_H_
