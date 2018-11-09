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
            new FilterBlockBuilder(block_size_ * 2 - 4, // 4 == ignore crc32
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
        std::string_view block = block_builder_->Finish();
        BlockHandle handle = WriteBlock(block);
        if (error_.fail()) {
            return;
        }
        indexs_.push_back(handle);
        block_builder_->Reset();
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
    n_entries_++;
}

/*virtual*/ Error SstTableBuilder::error() { return error_; }
    
/*virtual*/ Error SstTableBuilder::Finish() {
    using ::mai::base::Slice;
    using ::mai::base::ScopedMemory;
    
    std::string_view last_block = block_builder_->Finish();
    if (!last_block.empty()) {
        BlockHandle handle = WriteBlock(last_block);
        if (error_.fail()) {
            return error_;
        }
        indexs_.push_back(handle);
    }
    
    BlockHandle filter = WriteFilter();
    if (error_.fail()) {
        return error_;
    }
    
    BlockHandle index = WriteIndexs();
    if (error_.fail()) {
        return error_;
    }
    
    BlockHandle props = WriteProps(index, filter);
    if (error_.fail()) {
        return error_;
    }
    
    ScopedMemory scope;
    error_ = file_->Append(Slice::GetU64(props.offset(), &scope));
    if (error_.fail()) {
        return error_;
    }
    error_ = file_->Append(Slice::GetU32(Table::kSstMagicNumber, &scope));
    if (error_.fail()) {
        return error_;
    }
    return Error::OK();
}

/*virtual*/ void SstTableBuilder::Abandon() {
    if (block_builder_) {
        block_builder_->Reset();
    }
    if (filter_builder_) {
        filter_builder_->Reset();
    }
    indexs_.clear();
    smallest_key_.clear();
    largest_key_.clear();
    has_seen_first_key_ = false;
    is_last_level_ = false;
    n_entries_ = 0;
    
    error_ = file_->Truncate(0);
}
    
/*virtual*/ uint64_t SstTableBuilder::NumEntries() const { return n_entries_; }

/*virtual*/ uint64_t SstTableBuilder::FileSize() const {
    uint64_t size;
    Error rs = file_->GetFileSize(&size);
    if (!rs) {
        return 0;
    }
    return size;
}
    
BlockHandle SstTableBuilder::WriteBlock(std::string_view block) {
    using ::mai::base::Slice;
    using ::mai::base::ScopedMemory;
    
    uint32_t checksum = ::crc32(0, block.data(), block.size());
    
    uint64_t offset;
    error_ = file_->GetFileSize(&offset);
    if (error_.fail()) {
        return BlockHandle{};
    }
    BlockHandle handle(offset, 4 + block.size());
    
    ScopedMemory scope;
    error_ = file_->Append(Slice::GetU32(checksum, &scope));
    if (error_.fail()) {
        return BlockHandle{};
    }
    error_ = file_->Append(block);
    if (error_.fail()) {
        return BlockHandle{};
    }
    return handle;
}
    
BlockHandle SstTableBuilder::WriteFilter() {
    std::string_view block = filter_builder_->Finish();
    AlignmentToBlock();
    if (error_.fail()) {
        return BlockHandle{};
    }
    return WriteBlock(block);
}
    
BlockHandle SstTableBuilder::WriteIndexs() {
    using ::mai::base::Slice;
    using ::mai::base::ScopedMemory;
    
    std::string block;
    
    ScopedMemory scope;
    for (BlockHandle idx : indexs_) {
        idx.Encode(&block);
    }

    AlignmentToBlock();
    if (error_.fail()) {
        return BlockHandle{};
    }
    return WriteBlock(block);
}

BlockHandle SstTableBuilder::WriteProps(BlockHandle indexs, BlockHandle filter) {
    TableProperties props;
    
    props.unordered       = false;
    props.last_level      = is_last_level_;
    props.last_version    = max_version_;
    props.num_entries     = n_entries_;
    props.block_size      = static_cast<uint32_t>(block_size_);
    props.index_position  = indexs.offset();
    props.index_count     = indexs.size();
    props.filter_position = filter.offset();
    props.filter_size     = filter.size();
    props.smallest_key    = smallest_key_;
    props.largest_key     = largest_key_;
    uint64_t offset = AlignmentToBlock();
    if (error_.fail()) {
        return BlockHandle{};
    }
    error_ = Table::WriteProperties(props, file_);
    if (error_.fail()) {
        return BlockHandle{};
    }
    return BlockHandle{offset, FileSize()};
}
    
uint64_t SstTableBuilder::AlignmentToBlock() {
    uint64_t position = FileSize();
    if (error_.fail()) {
        return position;
    }
    
    base::ScopedMemory scope;
    if (position % block_size_) {
        size_t pad_size = block_size_ - position % block_size_;
        error_ = file_->Append(base::Slice::GetPad(pad_size, &scope));
        if (error_.fail()) {
            return position;
        }
        position = RoundUp(position, block_size_);
    }
    return position;
}
    
} // namespace table
    
} // namespace mai
