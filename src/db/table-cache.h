#ifndef MAI_DB_TABLE_CACHE_H_
#define MAI_DB_TABLE_CACHE_H_

#include "table/table-reader.h"
#include "base/base.h"
#include "base/reference-count.h"
#include "mai/error.h"
#include "glog/logging.h"
#include <string>
#include <memory>
#include <unordered_map>

namespace mai {
class Env;
class RandomAccessFile;
class Iterator;
class ReadOptions;
namespace table {
class TableReader;
class TableProperties;
} // namespace table
namespace core {
class KeyFilter;
} // namespace core
namespace db {
class ColumnFamilyImpl;
class FileMetadata;
class Factory;
    
class TableCache final {
public:
    TableCache(const std::string &db_name, Env *env, Factory *factory,
               bool allow_mmap_reads)
        : db_name_(db_name)
        , env_(DCHECK_NOTNULL(env))
        , factory_(DCHECK_NOTNULL(factory))
        , allow_mmap_reads_(allow_mmap_reads) {}
    ~TableCache();
    
    Iterator *NewIterator(const ReadOptions &read_opts,
                          const ColumnFamilyImpl *cf, uint64_t file_number,
                          uint64_t file_size);
    
    Error GetTableProperties(const ColumnFamilyImpl *cf, uint64_t file_number,
                             std::shared_ptr<table::TableProperties> *props);
    
    Error GetKeyFilter(const ColumnFamilyImpl *cf, uint64_t file_number,
                       std::shared_ptr<core::KeyFilter> *filter);
    
    Error GetFileMetadata(const ColumnFamilyImpl *cf, uint64_t file_number,
                          FileMetadata *fmd);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(TableCache);
private:
    struct Entry : public base::ReferenceCounted<Entry> {
        uint32_t    cfid;
        std::string file_name;
        std::unique_ptr<RandomAccessFile> file;
        std::unique_ptr<table::TableReader> table;
    };
    
    Error EnsureTableCached(const ColumnFamilyImpl *cf, uint64_t file_number,
                            uint64_t file_size, base::Handle<Entry> *result);
    
    static void EntryCleanup(void *arg1, void *arg2);
    
    const std::string db_name_;
    Env *const env_;
    Factory *const factory_;
    const bool allow_mmap_reads_;
    
    std::unordered_map<uint64_t, base::Handle<Entry>> cached_;
}; // class TableCache
    
} // namespace db
    
} // namespace mai


#endif // MAI_DB_TABLE_CACHE_H_
