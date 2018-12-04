#ifndef MAI_TABLE_SST_TABLE_BUILDER_H_
#define MAI_TABLE_SST_TABLE_BUILDER_H_

#include "table/table-builder.h"
#include "table/table.h"
#include "base/io-utils.h"
#include <vector>

namespace mai {
class WritableFile;
namespace core {
class InternalKeyComparator;
} // namespace core
namespace table {
class DataBlockBuilder;
class FilterBlockBuilder;
    
class SstTableBuilder final : public TableBuilder {
public:
    SstTableBuilder(const core::InternalKeyComparator *ikcmp, WritableFile *file,
                    uint64_t block_size, int n_restart,
                    size_t approximated_n_entries = 0);
    virtual ~SstTableBuilder() override;
    virtual void Add(std::string_view key, std::string_view value) override;
    virtual Error error() override;
    virtual Error Finish() override;
    virtual void Abandon() override;
    virtual uint64_t NumEntries() const override;
    virtual uint64_t FileSize() const override;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(SstTableBuilder);
private:
    BlockHandle WriteBlock(std::string_view block);
    BlockHandle WriteFilter();
    BlockHandle WriteIndexs();
    BlockHandle WriteProperties(BlockHandle indexs, BlockHandle filter);
    uint64_t AlignmentToBlock();
    
    const core::InternalKeyComparator *const ikcmp_;
    base::FileWriter writer_;
    const uint64_t block_size_;
    const int n_restart_;
    const size_t approximated_n_entries_;
    
    Error error_;
    bool has_seen_first_key_ = false;
    bool is_last_level_ = false;
    TableProperties props_;
    std::unique_ptr<DataBlockBuilder> block_builder_;
    std::unique_ptr<DataBlockBuilder> index_builder_;
    std::unique_ptr<FilterBlockBuilder> filter_builder_;
}; // class SSTTableBuilder
    
} // namespace table
    
} // namespace mai

#endif // MAI_TABLE_SST_TABLE_BUILDER_H_
