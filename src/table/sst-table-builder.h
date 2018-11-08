#ifndef MAI_TABLE_SST_TABLE_BUILDER_H_
#define MAI_TABLE_SST_TABLE_BUILDER_H_

#include "table/table-builder.h"

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
                    uint64_t block_size, int n_restart);
    virtual ~SstTableBuilder();
    
    virtual void Add(std::string_view key, std::string_view value) override;
    virtual Error error() override;
    virtual Error Finish() override;
    virtual void Abandon() override;
    virtual uint64_t NumEntries() const override;
    virtual uint64_t FileSize() const override;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(SstTableBuilder);
private:
    const core::InternalKeyComparator *ikcmp_;
    WritableFile *file_;
    uint64_t block_size_;
    int n_restart_;
    
    Error error_;
    uint64_t n_entries_ = 0;
    bool has_seen_first_key_ = false;
    bool is_last_level_ = false;
    std::string smallest_key_;
    std::string largest_key_;
    uint64_t max_version_ = 0;
    std::unique_ptr<DataBlockBuilder> block_builder_;
    std::unique_ptr<FilterBlockBuilder> filter_builder_;
}; // class SSTTableBuilder
    
} // namespace table
    
} // namespace mai

#endif // MAI_TABLE_SST_TABLE_BUILDER_H_
