#ifndef MAI_TABLE_SST_TABLE_READER_H_
#define MAI_TABLE_SST_TABLE_READER_H_

#include "table/table-reader.h"
#include "table/table.h"
#include <vector>

namespace mai {
    
namespace table {
    
class SstTableReader final : public TableReader {
public:
    SstTableReader(RandomAccessFile *file, uint64_t file_size,
                   bool checksum_verify);
    virtual ~SstTableReader();
    
    Error Prepare();
    
    virtual Iterator *
    NewIterator(const ReadOptions &read_opts,
                const core::InternalKeyComparator *ikcmp) override;
    virtual Error Get(const ReadOptions &read_opts,
                      const core::InternalKeyComparator *ikcmp,
                      std::string_view key,
                      core::Tag *tag,
                      std::string_view *value,
                      std::string *scratch) override;
    virtual size_t ApproximateMemoryUsage() const override;
    virtual std::shared_ptr<TableProperties> GetTableProperties() const override;
    virtual std::shared_ptr<core::KeyFilter> GetKeyFilter() const override;
    
private:
    RandomAccessFile *file_;
    uint64_t file_size_;
    bool checksum_verify_;
    
    std::vector<BlockHandle> indexs_;
    std::shared_ptr<TableProperties> table_props_;
    std::shared_ptr<core::KeyFilter> bloom_filter_;
}; // class SstTableReader
    
} // namespace table
    
} // namespace mai


#endif // MAI_TABLE_SST_TABLE_READER_H_
