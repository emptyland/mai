#ifndef MAI_DB_TABLE_CACHE_H_
#define MAI_DB_TABLE_CACHE_H_

#include "table/table-reader.h"
#include "core/lru-cache-v1.h"
#include "base/reference-count.h"
#include "mai/options.h"
#include "mai/error.h"
#include "glog/logging.h"
#include <string>
#include <memory>

namespace mai {
class Env;
class RandomAccessFile;
class Iterator;
class ReadOptions;
namespace table {
class TableReader;
struct TableProperties;
class TablePropsBoundle;
} // namespace table
namespace core {
class KeyFilter;
} // namespace core
namespace db {
class ColumnFamilyImpl;
class FileMetaData;
class Factory;
    
class TableCache final {
public:
    TableCache(const std::string &abs_db_path, const Options &opts,
               Factory *factory);
    ~TableCache();
    
    Iterator *NewIterator(const ReadOptions &read_opts,
                          const ColumnFamilyImpl *cfd, uint64_t file_number,
                          uint64_t file_size);
    
    Error Get(const ReadOptions &read_opts, const ColumnFamilyImpl *cfd,
              uint64_t file_number, std::string_view key, core::Tag *tag,
              std::string *value);
    
    Error GetTableProperties(const ColumnFamilyImpl *cfd, uint64_t file_number,
                             base::intrusive_ptr<table::TablePropsBoundle> *props);
    
    Error GetKeyFilter(const ColumnFamilyImpl *cfd, uint64_t file_number,
                       base::intrusive_ptr<core::KeyFilter> *filter);
    
    void Invalidate(uint64_t file_number) {
        cache_.Remove(GetKey(&file_number));
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(TableCache);
private:
    struct Entry final {
        uint32_t    cfid;
        std::string file_name;
        std::unique_ptr<RandomAccessFile> file;
        std::unique_ptr<table::TableReader> table;
    };
    
    Error GetOrLoadTable(const ColumnFamilyImpl *cfd,
                         uint64_t file_number, uint64_t file_size,
                         base::intrusive_ptr<core::LRUHandle> *result);
    
    Error LoadTable(const ColumnFamilyImpl *cfd,
                    uint64_t file_number, uint64_t file_size,
                    Entry *result);
    
    static std::string_view GetKey(const uint64_t *file_number) {
        return std::string_view(reinterpret_cast<const char *>(file_number),
                                sizeof(*file_number));
    }
    
    static Entry *GetEntry(core::LRUHandle *handle) {
        return static_cast<Entry *>(DCHECK_NOTNULL(handle->value));
    }
    
    static void EntryDeleter(std::string_view, void *value);
    static Error EntryLoader(std::string_view key, core::LRUHandle **result,
                             void *arg0, void *arg1);

    const std::string abs_db_path_;
    Env *const env_;
    Factory *const factory_;
    const bool allow_mmap_reads_;
    core::LRUCacheShard cache_;
}; // class TableCache
    
} // namespace db
    
} // namespace mai


#endif // MAI_DB_TABLE_CACHE_H_
