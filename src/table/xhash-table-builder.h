#ifndef MAI_TABLE_XHASH_TABLE_BUILDER_H_
#define MAI_TABLE_XHASH_TABLE_BUILDER_H_

#include "table/table-builder.h"
#include "glog/logging.h"
#include <vector>

namespace mai {
class WritableFile;
namespace core {
class InternalKeyComparator;
} // namespace core
namespace table {

class XhashTableBuilder final : public TableBuilder {
public:
    XhashTableBuilder(const core::InternalKeyComparator *ikcmp,
                      WritableFile *file,
                      size_t max_hash_slots,
                      uint32_t block_size)
        : ikcmp_(DCHECK_NOTNULL(ikcmp))
        , file_(DCHECK_NOTNULL(file))
        , buckets_(max_hash_slots)
        , block_size_(block_size) {
        DCHECK_LE(7, max_hash_slots) << "Max slots too small!";
    }
    
    virtual ~XhashTableBuilder();
    
    virtual void Add(std::string_view key, std::string_view value) override;
    virtual Error error() override;
    virtual Error Finish() override;
    virtual void Abandon() override;
    virtual uint64_t NumEntries() const override;
    virtual uint64_t FileSize() const override;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(XhashTableBuilder);
private:
    struct Bucket {
        std::string kv;
        std::string_view last_user_key;
    };
    
    uint64_t AlignmentToBlock();
    uint64_t WriteIndexs(const std::vector<std::tuple<uint64_t, uint64_t>> &indexs);
    
    const core::InternalKeyComparator *const ikcmp_;
    WritableFile *file_;
    std::vector<Bucket> buckets_;
    uint64_t block_size_;
    
    bool has_seen_first_key_ = false;
    bool is_last_level_ = false;
    uint64_t num_entries_ = 0;
    mutable Error last_error_;
    std::string smallest_key_;
    std::string largest_key_;
    uint64_t max_version_ = 0;
    uint64_t index_position_ = 0;
    uint64_t properties_position_ = 0;
}; // public XhashTableBuilder

    
} // namespace table
    
} // namespace mai

#endif // MAI_TABLE_XHASH_TABLE_BUILDER_H_
