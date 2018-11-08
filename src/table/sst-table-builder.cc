#include "table/sst-table-builder.h"
#include "table/data-block-builder.h"
#include "table/filter-block-builder.h"
#include "core/key-boundle.h"
#include "core/internal-key-comparator.h"
#include "glog/logging.h"

namespace mai {
    
namespace table {
    
SstTableBuilder::SstTableBuilder(const core::InternalKeyComparator *ikcmp,
                                 WritableFile *file, uint64_t block_size,
                                 int n_restart)
    : ikcmp_(DCHECK_NOTNULL(ikcmp))
    , file_(DCHECK_NOTNULL(file))
    , block_size_(block_size)
    , n_restart_(n_restart) {
    DCHECK_GT(n_restart_, 1);
    DCHECK_GE(block_size_, 512);
}

/*virtual*/ SstTableBuilder::~SstTableBuilder() {}

/*virtual*/
void SstTableBuilder::Add(std::string_view key, std::string_view value) {
    using ::mai::core::ParsedTaggedKey;
    using ::mai::core::KeyBoundle;
    
    if (!block_builder_) {
        block_builder_.reset(new DataBlockBuilder(n_restart_));
    }
    if (!filter_builder_) {
        filter_builder_.reset(
            new FilterBlockBuilder(block_size_ * 2,
                                   base::Hash::kBloomFilterHashs,
                                   base::Hash::kNumberBloomFilterHashs));
    }
    
    ParsedTaggedKey ikey;
    KeyBoundle::ParseTaggedKey(key, &ikey);
    
    if (!has_seen_first_key_) {
        if (ikey.tag.version() == 0) {
            is_last_level_ = true;
        }
        
        smallest_key_.assign(key);
        largest_key_.assign(key);
        has_seen_first_key_ = true;
    }
    
    if (is_last_level_) {
        block_builder_->Add(ikey.user_key, value);
    } else {
        block_builder_->Add(key, value);
    }
    filter_builder_->AddKey(ikey.user_key);
    
    if (block_builder_->CurrentSizeEstimate() >= block_size_) {
        // TODO: Write block
        std::string_view block = block_builder_->Finish();
    }

    if (ikcmp_->Compare(key, smallest_key_) < 0) {
        smallest_key_ = key;
    }
    if (ikcmp_->Compare(key, largest_key_) > 0) {
        largest_key_ = key;
    }
    if (ikey.tag.version() > max_version_) {
        max_version_ = ikey.tag.version();
    }
    if (ikey.tag.version() > max_version_) {
        max_version_ = ikey.tag.version();
    }
}

/*virtual*/ Error SstTableBuilder::error() { return error_; }
    
/*virtual*/ Error SstTableBuilder::Finish() {
    // TODO:
    return MAI_NOT_SUPPORTED("TODO:");
}

/*virtual*/ void SstTableBuilder::Abandon() {
}
    
/*virtual*/ uint64_t SstTableBuilder::NumEntries() const {
    return n_entries_;
}

/*virtual*/ uint64_t SstTableBuilder::FileSize() const {
    // TODO:
    return 0;
}
    
} // namespace table
    
} // namespace mai
