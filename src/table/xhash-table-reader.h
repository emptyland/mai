#ifndef MAI_TABLE_XHASH_TABLE_READER_H_
#define MAI_TABLE_XHASH_TABLE_READER_H_

#include "table/table-reader.h"
#include "glog/logging.h"

namespace mai {
class RandomAccessFile;
namespace table {
    
class XhashTableReader final : public TableReader {
public:
    typedef uint32_t (*hash_func_t)(const char *, size_t);
    
    XhashTableReader(RandomAccessFile *file, uint64_t file_size,
                    hash_func_t hash_func)
        : file_(DCHECK_NOTNULL(file))
        , file_size_(file_size)
        , hash_func_(DCHECK_NOTNULL(hash_func)) {}
    
    virtual ~XhashTableReader();
    
    Error Prepare();
    
    virtual core::InternalIterator *NewIterator(const ReadOptions &read_opts,
                                                const Comparator *cmp) override;
    virtual Error Get(const ReadOptions &read_opts,
                      const Comparator *cmp,
                      std::string_view key,
                      core::Tag *tag,
                      std::string_view *value,
                      std::string *scratch) override;
    virtual size_t ApproximateMemoryUsage() const override;
    virtual std::shared_ptr<TableProperties> GetTableProperties() const override;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(XhashTableReader);
private:
    struct Index {
        uint64_t offset;
        uint64_t size;
    };
    
    Error ReadKey(uint64_t *offset, std::string_view *result,
                  std::string *scratch);
    Error ReadLength(uint64_t *offset, uint64_t *len);
    
    RandomAccessFile *file_;
    uint64_t file_size_;
    hash_func_t hash_func_;
    
    std::shared_ptr<TableProperties> table_props_;
    std::vector<Index> indexs_;
}; // class XhashTableReader
    
} // namespace table

} // namespace mai


#endif // MAI_TABLE_XHASH_TABLE_READER_H_
