#ifndef MAI_TABLE_SST_TABLE_READER_H_
#define MAI_TABLE_SST_TABLE_READER_H_

#include "table/table-reader.h"
#include "table/table.h"
#include <vector>

namespace mai {
    
namespace table {
    
class BlockIterator;
    
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
    virtual base::intrusive_ptr<TablePropsBoundle> GetTableProperties() const override;
    virtual base::intrusive_ptr<core::KeyFilter> GetKeyFilter() const override;
    
    Iterator *NewIndexIterator(const core::InternalKeyComparator *ikcmp);
    
    void TEST_PrintAll(const core::InternalKeyComparator *ikcmp);
private:
    class IteratorImpl;
    
    Error GetFirstKey(BlockHandle handle, std::string_view *result,
                      std::string *scratch);
    Error GetKey(std::string_view prev_key, uint64_t *offset,
                 std::string_view *result, std::string *scratch);
    Error ReadBlock(const BlockHandle &bh, std::string_view *result,
                    std::string *scatch) const;
    
    RandomAccessFile *file_;
    uint64_t file_size_;
    bool checksum_verify_;
    
    base::intrusive_ptr<TablePropsBoundle> table_props_boundle_;
    const TableProperties *table_props_ = nullptr;
    base::intrusive_ptr<core::KeyFilter> bloom_filter_;
}; // class SstTableReader
    
} // namespace table
    
} // namespace mai


#endif // MAI_TABLE_SST_TABLE_READER_H_
