#ifndef MAI_TABLE_S1_TABLE_BUILDER_H_
#define MAI_TABLE_S1_TABLE_BUILDER_H_

#include "table/table-builder.h"
#include "table/table.h"
#include "base/io-utils.h"
#include <memory>
#include <set>
#include <map>

namespace mai {
class WritableFile;
namespace core {
class InternalKeyComparator;
} // namespace core
namespace table {
    
class PlainBlockBuilder;
class FilterBlockBuilder;
    
class S1TableBuilder final : public TableBuilder {
public:
    S1TableBuilder(const core::InternalKeyComparator *ikcmp, WritableFile *file,
                  size_t max_hash_slots, uint32_t block_size,
                  size_t approximated_n_entries = 0);
    virtual ~S1TableBuilder() override;
    virtual void Add(std::string_view key, std::string_view value) override;
    virtual Error error() override;
    virtual Error Finish() override;
    virtual void Abandon() override;
    virtual uint64_t NumEntries() const override;
    virtual uint64_t FileSize() const override;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(S1TableBuilder);
private:
    static const uint64_t kInvalidIdx;
    struct Index {
        uint64_t block_idx = kInvalidIdx;
        uint32_t offset = 0;
    };
    
    BlockHandle WriteIndex();
    BlockHandle WriteFilter();
    BlockHandle WriteProperties();
    BlockHandle FlushBlock();
    BlockHandle WriteBlock(std::string_view block);

    const core::InternalKeyComparator *const ikcmp_;
    base::FileWriter writer_;
    const size_t max_buckets_;
    const uint64_t block_size_;
    const uint64_t approximated_n_entries_;
    
    Error error_;
    bool has_seen_first_key_ = false;
    bool is_last_level_ = false;
    std::unique_ptr<std::vector<Index>[]> buckets_;
    TableProperties props_;
    
    std::unique_ptr<PlainBlockBuilder> block_builder_;
    std::unique_ptr<FilterBlockBuilder> filter_builder_;
    std::set<uint64_t> unbound_index_;
    std::vector<BlockHandle> block_map_;
}; // class S1TableReader
    
} // namespace table

} // namespace mai


#endif // MAI_TABLE_S1_TABLE_BUILDER_H_
