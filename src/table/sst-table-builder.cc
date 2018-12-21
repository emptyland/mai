#include "table/sst-table-builder.h"
#include "table/data-block-builder.h"
#include "table/filter-block-builder.h"
#include "core/key-boundle.h"
#include "core/internal-key-comparator.h"
#include "mai/env.h"
#include "glog/logging.h"

namespace mai {
    
namespace table {
    
SstTableBuilder::SstTableBuilder(const core::InternalKeyComparator *ikcmp,
                                 WritableFile *file, uint64_t block_size,
                                 int n_restart, size_t approximated_n_entries)
    : ikcmp_(DCHECK_NOTNULL(ikcmp))
    , writer_(DCHECK_NOTNULL(file))
    , block_size_(block_size)
    , n_restart_(n_restart)
    , approximated_n_entries_(approximated_n_entries) {
    DCHECK_GT(n_restart_, 1);
    DCHECK_GE(block_size_, 512);
    DCHECK_EQ(0, block_size_ % 4);
        
    props_.block_size = static_cast<uint32_t>(block_size_);
    props_.unordered  = false;
}

/*virtual*/ SstTableBuilder::~SstTableBuilder() {}

/*virtual*/
void SstTableBuilder::Add(std::string_view key, std::string_view value) {
    using ::mai::core::ParsedTaggedKey;
    using ::mai::core::KeyBoundle;

    
    if (!block_builder_) {
        block_builder_.reset(new DataBlockBuilder(n_restart_));
    }
    if (!index_builder_) {
        index_builder_.reset(new DataBlockBuilder(n_restart_));
    }
    if (!filter_builder_) {
        size_t bloom_filter_size =
            FilterBlockBuilder::ComputeBoomFilterSize(approximated_n_entries_,
                                                      block_size_,
                                                      base::Hash::kNumberBloomFilterHashs);
        filter_builder_.reset(
            new FilterBlockBuilder(bloom_filter_size - 4, // 4 == ignore crc32
                                   base::Hash::kBloomFilterHashs,
                                   base::Hash::kNumberBloomFilterHashs));
    }
    
    ParsedTaggedKey ikey;
    KeyBoundle::ParseTaggedKey(key, &ikey);
    
    if (!has_seen_first_key_) {
        if (ikey.tag.sequence_number() == 0) {
            is_last_level_ = true;
        }
        
        props_.smallest_key = key;
        props_.largest_key  = key;
        has_seen_first_key_ = true;
    }
    
    if (is_last_level_) {
        block_builder_->Add(ikey.user_key, value);
    } else {
        block_builder_->Add(key, value);
    }
    filter_builder_->AddKey(ikey.user_key);
    
    if (block_builder_->CurrentSizeEstimate() >= block_size_) {
        std::string_view block = block_builder_->Finish();
        BlockHandle handle = WriteBlock(block);
        if (error_.fail()) {
            return;
        }
        std::string buf;
        handle.Encode(&buf);
        index_builder_->Add(block_builder_->last_key(), buf);
        block_builder_->Reset();
    }

    if (ikcmp_->Compare(key, props_.smallest_key) < 0) {
        props_.smallest_key = key;
    }
    if (ikcmp_->Compare(key, props_.largest_key) > 0) {
        props_.largest_key = key;
    }
    if (ikey.tag.sequence_number() > props_.last_version) {
        props_.last_version = ikey.tag.sequence_number();
    }
    props_.num_entries++;
}

/*virtual*/ Error SstTableBuilder::error() { return error_; }
    
/*virtual*/ Error SstTableBuilder::Finish() {
    using ::mai::base::Slice;
    using ::mai::base::ScopedMemory;
    
    if (block_builder_) {
        std::string_view last_block = block_builder_->Finish();
        if (!last_block.empty()) {
            BlockHandle handle = WriteBlock(last_block);
            if (error_.fail()) {
                return error_;
            }
            std::string buf;
            handle.Encode(&buf);
            index_builder_->Add(block_builder_->last_key(), buf);
        }
    }
    
    BlockHandle filter = WriteFilter();
    if (error_.fail()) {
        return error_;
    }
    
    BlockHandle index = WriteIndexs();
    if (error_.fail()) {
        return error_;
    }
    
    BlockHandle props = WriteProperties(index, filter);
    if (error_.fail()) {
        return error_;
    }
    
    // Footer
    error_ = writer_.WriteFixed64(props.offset());
    if (error_.fail()) {
        return error_;
    }
    error_ = writer_.WriteFixed32(Table::kSstMagicNumber);
    if (error_.fail()) {
        return error_;
    }
    
    // Flush All
    error_ = writer_.Flush();
    if (error_.fail()) {
        return error_;
    }
    return Error::OK();
}

/*virtual*/ void SstTableBuilder::Abandon() {
    block_builder_.reset();
    filter_builder_.reset();
    index_builder_.reset();
    
    props_ = TableProperties{};
    props_.block_size = static_cast<uint32_t>(block_size_);
    props_.unordered = false;
    
    has_seen_first_key_ = false;
    is_last_level_ = false;
    error_ = writer_.file()->Truncate(0);
}
    
/*virtual*/ uint64_t SstTableBuilder::NumEntries() const {
    return props_.num_entries;
}

/*virtual*/ uint64_t SstTableBuilder::FileSize() const {
    uint64_t size;
    Error rs = writer_.file()->GetFileSize(&size);
    if (!rs) {
        return 0;
    }
    return size;
}
    
BlockHandle SstTableBuilder::WriteBlock(std::string_view block) {
    using ::mai::base::Slice;
    using ::mai::base::ScopedMemory;
    
    uint32_t checksum = ::crc32(0, block.data(), block.size());
    uint64_t offset = writer_.written_position();
    BlockHandle handle(offset, 4 + block.size());

    error_ = writer_.WriteFixed32(checksum);
    if (error_.fail()) {
        return BlockHandle{};
    }
    error_ = writer_.Write(block);
    if (error_.fail()) {
        return BlockHandle{};
    }
    return handle;
}
    
BlockHandle SstTableBuilder::WriteFilter() {
    std::string_view block = filter_builder_->Finish();
    return WriteBlock(block);
}
    
BlockHandle SstTableBuilder::WriteIndexs() {
    std::string_view block = index_builder_->Finish();
    return WriteBlock(block);
}

BlockHandle SstTableBuilder::WriteProperties(BlockHandle indexs, BlockHandle filter) {
    props_.index_position  = indexs.offset();
    props_.index_size      = indexs.size();

    props_.filter_position = filter.offset();
    props_.filter_size     = filter.size();

    std::string block;
    Table::WriteProperties(props_, &block);
    return WriteBlock(block);
}
    
} // namespace table
    
} // namespace mai
