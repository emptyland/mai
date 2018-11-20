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

class Factory {
public:
    Factory() {}
    virtual ~Factory() {}
    
    virtual core::MemoryTable *
    NewMemoryTable(const core::InternalKeyComparator *ikcmp, bool unordered,
                   size_t initial_slots) = 0;
    
    virtual Error
    NewTableReader(RandomAccessFile *file, uint64_t file_size,
                   bool checksum_verify, base::hash_func_t hash_func,
                   std::unique_ptr<table::TableReader> *result) = 0;
    
    virtual table::TableBuilder *
    NewTableBuilder(const core::InternalKeyComparator *ikcmp, bool unordered,
                    WritableFile *file, uint64_t block_size, int n_restart,
                    size_t max_hash_slots, base::hash_func_t hash_func) = 0;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Factory);
}; // class Factory


} // namespace db

} // namespace mai

#endif // MAI_DB_FACTORY_H_
