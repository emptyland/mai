#ifndef MAI_TABLE_S1_TABLE_READER_H_
#define MAI_TABLE_S1_TABLE_READER_H_

#include "table/table-reader.h"
#include "core/lru-cache-v1.h"
#include <vector>

namespace mai {
class RandomAccessFile;
namespace table {
class BlockHandle;
class BlockCache;
    
class S1TableReader final : public TableReader {
public:
    S1TableReader(RandomAccessFile *file, uint64_t file_number,
                  uint64_t file_size, bool checksum_verify, BlockCache *cache);
    virtual ~S1TableReader();
    
    struct Index {
        uint64_t block_idx;
        uint32_t offset;
    };
    
    //DEF_VAL_GETTER(std::vector<std::vector<Index>>, index);
    
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
    
private:
    class IteratorImpl;
    class KeyFilterImpl;
    
    uint64_t ReadKey(std::string_view buf, uint64_t *shared_len,
                     uint64_t *private_len, std::string_view *result) const;
    
    uint64_t ReadValue(std::string_view buf, std::string_view *result,
                       std::string *scatch) const;
    
    Error PrepareRead(const Index &idx, const ReadOptions &read_opts,
                      std::string_view *buf,
                      base::intrusive_ptr<core::LRUHandle> *handle) const;
    
    Error ReadKey(uint64_t *offset, uint64_t *shared_len, uint64_t *private_len,
                  std::string_view *result, std::string *scatch) const;
    Error ReadValue(uint64_t *offset, std::string_view *result,
                    std::string *scatch) const;
    Error ReadBlock(const BlockHandle &bh, std::string_view *result,
                    std::string *scatch) const;
    
    RandomAccessFile *const file_;
    const uint64_t file_number_;
    const uint64_t file_size_;
    const bool checksum_verify_;
    BlockCache *const cache_;
    
    std::unique_ptr<std::vector<Index>[]> index_;
    size_t index_size_ = 0;
    std::unique_ptr<BlockHandle[]> block_map_;
    size_t block_map_size_ = 0;
    bool has_initialized_ = false;
    base::intrusive_ptr<TablePropsBoundle> table_props_boundle_;
    const TableProperties *table_props_ = nullptr;
    base::intrusive_ptr<core::KeyFilter> filter_;
}; // class S1TableReader
    
} // namespace table

} // namespace mai


#endif // MAI_TABLE_S1_TABLE_READER_H_
