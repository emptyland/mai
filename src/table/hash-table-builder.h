#ifndef MAI_TABLE_HASH_TABLE_BUILDER_H_
#define MAI_TABLE_HASH_TABLE_BUILDER_H_

#include "table/table-builder.h"
#include <vector>

namespace mai {
    
class WritableFile;
    
namespace table {

class HashTableBuilder final : public TableBuilder {
public:
    typedef uint32_t (*hash_func_t)(const char *, size_t);
    
    HashTableBuilder(WritableFile *file,
                     size_t max_hash_slots,
                     hash_func_t hash_func,
                     uint32_t block_size);
    
    virtual ~HashTableBuilder();
    
    virtual void Add(std::string_view key, std::string_view value) override;
    virtual Error error() override;
    virtual Error Finish() override;
    virtual void Abandon() override;
    virtual uint64_t NumEntries() const override;
    virtual uint64_t FileSize() const override;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(HashTableBuilder);
private:
    // Hash-Map-Table bucket;
    struct IndexBucket {
        uint32_t offset = 0;
        uint32_t size   = 0;
    };
    
    WritableFile *file_;
    const size_t max_hash_slots_;
    hash_func_t hash_func_;
    uint32_t block_size_;
    Error latest_error_;
    uint64_t num_entries_ = 0;
    bool has_seen_first_key_ = false;
    bool is_latest_level_ = false;
    std::vector<IndexBucket> index_buckets_;
    size_t latest_slot_ = -1;
    std::string_view latest_key_;
};

} // namespace table
    
} // namespace mai

#endif // MAI_TABLE_HASH_TABLE_BUILDER_H_
