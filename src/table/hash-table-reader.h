#ifndef MAI_TABLE_HASH_TABLE_READER_H_
#define MAI_TABLE_HASH_TABLE_READER_H_

#include "table/table-reader.h"
#include "glog/logging.h"
#include <vector>

namespace mai {
    
class RandomAccessFile;
    
namespace table {
    
class HashTableReader final : public TableReader {
public:
    typedef uint32_t (*hash_func_t)(const char *, size_t);
    
    HashTableReader(RandomAccessFile *file, uint64_t file_size,
                    hash_func_t hash_func)
        : file_(DCHECK_NOTNULL(file))
        , file_size_(file_size)
        , hash_func_(DCHECK_NOTNULL(hash_func)) {}
    
    virtual ~HashTableReader();
    
    Error Prepare();
    
    virtual Iterator *NewIterator(const ReadOptions &read_opts,
                                  const core::InternalKeyComparator *ikcmp) override;
    virtual Error Get(const ReadOptions &read_opts,
                      const core::InternalKeyComparator *ikcmp,
                      std::string_view key,
                      core::Tag *tag,
                      std::string_view *value,
                      std::string *scratch) override;
    virtual size_t ApproximateMemoryUsage() const override;

    virtual std::shared_ptr<TableProperties> GetTableProperties() const override {
        return std::shared_ptr<TableProperties>(nullptr);
    }
private:
    struct Index {
        uint32_t offset;
        uint32_t size;
    };
    
    class IteratorImpl;
    
    Error ReadLengthItem(uint64_t offset, std::string_view *item,
                         std::string *scratch);
    
    RandomAccessFile *file_;
    uint64_t file_size_;
    hash_func_t hash_func_;
    std::vector<Index> index_;
}; // class HashTableReader
    
} // namespace table
    
} // namespace mai


#endif // MAI_TABLE_HASH_TABLE_READER_H_
