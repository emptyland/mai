#ifndef MAI_DB_TABLE_CACHE_H_
#define MAI_DB_TABLE_CACHE_H_

#include "table/table-reader.h"
#include "base/base.h"
#include "base/reference-count.h"
#include "mai/options.h"
#include "mai/error.h"
#include "glog/logging.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>

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
               Factory *factory)
        : abs_db_path_(abs_db_path)
        , env_(DCHECK_NOTNULL(opts.env))
        , factory_(DCHECK_NOTNULL(factory))
        , allow_mmap_reads_(opts.allow_mmap_reads) {}
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
        std::unique_lock<std::mutex> lock(mutex_);
        cached_.erase(file_number);
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(TableCache);
private:
    struct Entry : public base::ReferenceCounted<Entry> {
        uint32_t    cfid;
        std::string file_name;
        std::unique_ptr<RandomAccessFile> file;
        std::unique_ptr<table::TableReader> table;
    };
    
    Error EnsureTableCached(const ColumnFamilyImpl *cf, uint64_t file_number,
                            uint64_t file_size, base::intrusive_ptr<Entry> *result);
    
    static void EntryCleanup(void *arg1, void *arg2);
    
    const std::string abs_db_path_;
    Env *const env_;
    Factory *const factory_;
    const bool allow_mmap_reads_;
    
    std::unordered_map<uint64_t, base::intrusive_ptr<Entry>> cached_;
    std::mutex mutex_;
}; // class TableCache
    
} // namespace db
    
} // namespace mai


#endif // MAI_DB_TABLE_CACHE_H_
