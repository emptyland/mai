#ifndef MAI_TABLE_S1_TABLE_READER_H_
#define MAI_TABLE_S1_TABLE_READER_H_

#include "table/table-reader.h"
#include <vector>

namespace mai {
class RandomAccessFile;
namespace table {
class BlockHandle;
    
class S1TableReader final : public TableReader {
public:
    S1TableReader(RandomAccessFile *file, uint64_t file_size,
                  bool checksum_verify);
    virtual ~S1TableReader();
    
    struct Index {
        uint64_t block_offset;
        uint32_t offset;
    };
    
    DEF_VAL_GETTER(std::vector<std::vector<Index>>, index);
    
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
    class IteratorImpl;
    class KeyFilterImpl;
    
    Error ReadBlock(const BlockHandle &bh, std::string_view *result,
                    std::string *scatch) const;
    Error ReadKey(uint64_t *offset, uint64_t *shared_len, uint64_t *private_len,
                  std::string_view *result, std::string *scatch) const;
    Error ReadValue(uint64_t *offset, std::string_view *result,
                    std::string *scatch) const;
    
    RandomAccessFile *const file_;
    const uint64_t file_size_;
    const bool checksum_verify_;
    
    std::vector<std::vector<Index>> index_;
    std::shared_ptr<TableProperties> table_props_;
    std::shared_ptr<core::KeyFilter> filter_;
}; // class S1TableReader
    
} // namespace table

} // namespace mai


#endif // MAI_TABLE_S1_TABLE_READER_H_
