#ifndef MAI_DB_FACTORY_H_
#define MAI_DB_FACTORY_H_

#include "base/hash.h"
#include "base/base.h"
#include "mai/error.h"

namespace mai {
class RandomAccessFile;
class WritableFile;
class Allocator;
namespace core {
class MemoryTable;
class InternalKeyComparator;
} // namespace core
namespace table {
class TableReader;
class TableBuilder;
class BlockCache;
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
    NewMemoryTable(const core::InternalKeyComparator *ikcmp,
                   Allocator *allocator, bool unordered, size_t initial_slots) = 0;
    
    virtual Error
    NewTableReader(const std::string &name,
                   const core::InternalKeyComparator *ikcmp,
                   RandomAccessFile *file,
                   uint64_t file_number,
                   uint64_t file_size,
                   bool checksum_verify,
                   table::BlockCache *cache,
                   std::unique_ptr<table::TableReader> *result) = 0;
    
    virtual table::TableBuilder *
    NewTableBuilder(const std::string &name,
                    const core::InternalKeyComparator *ikcmp,
                    WritableFile *file, uint64_t block_size, int n_restart,
                    size_t max_hash_slots,
                    size_t approximated_n_entries) = 0;
    
    virtual Compaction *
    NewCompaction(const std::string &abs_db_path,
                  const core::InternalKeyComparator *ikcmp,
                  TableCache *table_cache, ColumnFamilyImpl *cfd) = 0;
    
    static Factory *NewDefault();
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Factory);
}; // class Factory


} // namespace db

} // namespace mai

#endif // MAI_DB_FACTORY_H_
