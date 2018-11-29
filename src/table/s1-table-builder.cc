#include "table/s1-table-builder.h"
#include "table/plain-block-builder.h"
#include "table/filter-block-builder.h"
#include "core/key-boundle.h"
#include "core/internal-key-comparator.h"
#include "base/slice.h"
#include "base/hash.h"
#include "mai/env.h"
#include "glog/logging.h"

namespace mai {
    
namespace table {
    
/*static*/ const uint64_t S1TableBuilder::kInvalidOffset = -1;
    
S1TableBuilder::S1TableBuilder(const core::InternalKeyComparator *ikcmp,
                             WritableFile *file,
                             size_t max_hash_slots, uint32_t block_size)
    : ikcmp_(DCHECK_NOTNULL(ikcmp))
    , writer_(DCHECK_NOTNULL(file))
    , max_buckets_(max_hash_slots)
    , block_size_(block_size)
    , buckets_(new std::vector<Index>[max_hash_slots]) {
    DCHECK_GE(block_size_, 512);

    props_.block_size = static_cast<uint32_t>(block_size_);
    props_.unordered = true;
}

/*virtual*/ S1TableBuilder::~S1TableBuilder() {
}

/*virtual*/
void S1TableBuilder::Add(std::string_view key, std::string_view value) {
    using ::mai::core::ParsedTaggedKey;
    using ::mai::core::KeyBoundle;
    
    ParsedTaggedKey ikey;
    KeyBoundle::ParseTaggedKey(key, &ikey);
    
    if (!has_seen_first_key_) {
        if (ikey.tag.sequence_number() == 0) {
            props_.last_level = true;
        }

        props_.smallest_key = key;
        props_.largest_key  = key;
        has_seen_first_key_ = true;
    }
    
    if (!block_builder_) {
        block_builder_.reset(new PlainBlockBuilder(ikcmp_->ucmp(),
                                                   props_.last_level));
    }
    if (!filter_builder_) {
        filter_builder_.reset(new FilterBlockBuilder(block_size_ * 2 - 4, // 4 == ignore crc32
                                                     base::Hash::kBloomFilterHashs,
                                                     base::Hash::kNumberBloomFilterHashs));
    }
    
    uint32_t offset;
    if (props_.last_level) {
        offset = block_builder_->Add(ikey.user_key, value);
    } else {
        offset = block_builder_->Add(key, value);
    }
    filter_builder_->AddKey(ikey.user_key);
    
    size_t slot = ikcmp_->Hash(key) % max_buckets_;
    buckets_[slot].push_back({kInvalidOffset, offset});
    unbound_index_.insert(slot);
    
    if (block_builder_->CurrentSizeEstimate() >= block_size_) {
        FlushBlock();
        if (!error_) {
            return;
        }
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

/*virtual*/ Error S1TableBuilder::error() { return error_; }

/*virtual*/ Error S1TableBuilder::Finish() {
    if (!unbound_index_.empty()) {
        FlushBlock();
        if (!error_) {
            return error_;
        }
    }
    BlockHandle index = WriteIndex();
    props_.index_position = index.offset();
    props_.index_count    = index.size();

    DCHECK_NE(0, max_buckets_);
    float conflict_factor = static_cast<float>(props_.num_entries) /
                            static_cast<float>(max_buckets_);
    if (conflict_factor > 3) { // Too much conflict buckets, need bloom filter.
        BlockHandle filter = WriteFilter();
        props_.filter_position = filter.offset();
        props_.filter_size     = filter.size();
    }
    
    BlockHandle props = WriteProperties();
    if (error_.fail()) {
        return error_;
    }

    error_ = writer_.WriteFixed64(props.offset());
    if (error_.fail()) {
        return error_;
    }
    error_ = writer_.WriteFixed32(Table::kS1tMagicNumber);
    if (error_.fail()) {
        return error_;
    }
    return Error::OK();
}
    
/*virtual*/ void S1TableBuilder::Abandon() {
    block_builder_.reset();
    filter_builder_.reset();
    
    props_ = TableProperties{};
    props_.block_size = static_cast<uint32_t>(block_size_);
    props_.unordered = true;
    
    error_ = Error::OK();
    has_seen_first_key_ = false;
    is_last_level_ = false;
    buckets_.reset(new std::vector<Index>[max_buckets_]);
    unbound_index_.clear();

    writer_.Truncate(0);
}
    
/*virtual*/ uint64_t S1TableBuilder::NumEntries() const {
    return props_.num_entries;
}
    
/*virtual*/ uint64_t S1TableBuilder::FileSize() const {
    uint64_t size = 0;
    writer_.file()->GetFileSize(&size);
    return size;
}

BlockHandle S1TableBuilder::WriteIndex() {
    using ::mai::base::Slice;
    using ::mai::base::ScopedMemory;
    AlignmentToBlock();
    if (!error_) {
        return BlockHandle{};
    }
    
    ScopedMemory scope;
    std::string block;
    for (size_t i = 0; i < max_buckets_; ++i) {
        block.append(Slice::GetV64(buckets_[i].size(), &scope));
        
        for (const auto &index : buckets_[i]) {
            block.append(Slice::GetV64(index.block_offset, &scope));
            block.append(Slice::GetV32(index.offset, &scope));
        }
    }
    return WriteBlock(block);
}
    
BlockHandle S1TableBuilder::WriteFilter() {
    AlignmentToBlock();
    if (!filter_builder_) {
        return BlockHandle{};
    }
    
    std::string_view block = filter_builder_->Finish();
    return WriteBlock(block);
}
    
BlockHandle S1TableBuilder::WriteProperties() {
    AlignmentToBlock();
    if (!error_) {
        return BlockHandle{};
    }
    
    std::string block;
    Table::WriteProperties(props_, &block);
    return WriteBlock(block);
}
    
BlockHandle S1TableBuilder::FlushBlock() {
    DCHECK(!unbound_index_.empty());

    std::string_view block = block_builder_->Finish();
    BlockHandle bh = WriteBlock(block);
    if (error_.fail()) {
        return BlockHandle{};
    }
    
    for (size_t unbound : unbound_index_) {
        for (int i = 0; i < buckets_[unbound].size(); ++i) {
            if (buckets_[unbound][i].block_offset == kInvalidOffset) {
                buckets_[unbound][i].block_offset = bh.offset();
            }
        }
    }
    block_builder_->Reset();
    unbound_index_.clear();
    return bh;
}
    
BlockHandle S1TableBuilder::WriteBlock(std::string_view block) {
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
    
uint64_t S1TableBuilder::AlignmentToBlock() {
    uint64_t current = writer_.written_position();
    uint64_t want = RoundUp(current, block_size_);
    if (want - current > 0) {
        error_ = writer_.WritePad(want - current);
    }
    return want;
}
    
} // namespace table


} // namespace mai
