#include "table/xhash-table-builder.h"
#include "table/table.h"
#include "core/internal-key-comparator.h"
#include "core/key-boundle.h"
#include "base/hash.h"
#include "base/slice.h"
#include "mai/env.h"

namespace mai {

namespace table {
    
static const char kZeroBytesStub[8] = {0, 0, 0, 0, 0, 0, 0, 0};

/*virtual*/ XhashTableBuilder::~XhashTableBuilder() {}

/*virtual*/
void XhashTableBuilder::Add(std::string_view key, std::string_view value) {
    core::ParsedTaggedKey ikey;
    core::KeyBoundle::ParseTaggedKey(key, &ikey);
    
    if (!has_seen_first_key_) {
        if (ikey.tag.version() == 0) {
            is_last_level_ = true;
        }
        has_seen_first_key_ = true;
    }
    
    base::ScopedMemory scope;
    
    uint32_t hash_val = hash_func_(ikey.user_key.data(), ikey.user_key.size());
    Bucket *bucket = &buckets_[hash_val % buckets_.size()];
    if (is_last_level_) {
        if (ikey.user_key == bucket->last_user_key) {
            bucket->kv.append(kZeroBytesStub, 1);
        } else {
            bucket->kv.append(base::Slice::GetV64(ikey.user_key.size(), &scope));
            bucket->last_user_key = ikey.user_key;
        }
    } else {
        if (ikey.user_key == bucket->last_user_key) {
            bucket->kv.append(kZeroBytesStub, 1);
        } else {
            bucket->kv.append(base::Slice::GetV64(key.size(), &scope));
            bucket->kv.append(key);
            bucket->last_user_key = ikey.user_key;
        }
    }
    if (smallest_key_.empty() || ikcmp_->Compare(key, smallest_key_) < 0) {
        smallest_key_ = key;
    }
    if (largest_key_.empty() || ikcmp_->Compare(key, largest_key_) > 0) {
        largest_key_ = key;
    }
    
    bucket->kv.append(base::Slice::GetV64(value.size(), &scope));
    bucket->kv.append(value);
    num_entries_++;
}
    
/*virtual*/ Error XhashTableBuilder::error() {
    return last_error_;
}

/*virtual*/ Error XhashTableBuilder::Finish() {
    std::vector<std::tuple<uint64_t, uint64_t>> indexs;
    
    base::ScopedMemory scope;
    for (const Bucket &bucket : buckets_) {
        if (bucket.kv.empty()) {
            indexs.push_back(std::make_tuple(0, 0));
            continue;
        }
        uint64_t position = 0;
        Error rs = file_->GetFileSize(&position);
        if (!rs) {
            return rs;
        }
        
        uint32_t checksum = ::crc32(0, bucket.kv.data(), bucket.kv.size());
        rs = file_->Append(base::Slice::GetU32(checksum, &scope));
        if (!rs) {
            return rs;
        }
        rs = file_->Append(bucket.kv);
        if (!rs) {
            return rs;
        }
        indexs.push_back(std::make_tuple(position, bucket.kv.size() + 4));
    }
    
    index_position_ = WriteIndexs(indexs);
    if (!last_error_) {
        return last_error_;
    }
    
    properties_position_ = WriteFileProperties();
    if (!last_error_) {
        return last_error_;
    }
    Error rs = file_->Append(base::Slice::GetU64(properties_position_, &scope));
    if (!rs) {
        return rs;
    }
    return file_->Append(base::Slice::GetU32(Table::kXmtMagicNumber, &scope));
}
    
/*virtual*/ void XhashTableBuilder::Abandon() {
    size_t old_size = buckets_.size();
    buckets_.clear();
    buckets_.resize(old_size);
    file_->Truncate(0);
    
    smallest_key_.clear();
    largest_key_.clear();
    index_position_ = 0;
    properties_position_ = 0;
    
    has_seen_first_key_ = false;
    is_last_level_ = false;
    last_error_ = Error::OK();
}

/*virtual*/ uint64_t XhashTableBuilder::NumEntries() const {
    return num_entries_;
}

/*virtual*/ uint64_t XhashTableBuilder::FileSize() const {
    uint64_t size = 0;
    last_error_ = file_->GetFileSize(&size);
    return last_error_;
}
    
uint64_t XhashTableBuilder::AlignmentToBlock() {
    uint64_t position = FileSize();
    if (!last_error_) {
        return position;
    }
    
    base::ScopedMemory scope;
    if (position % block_size_) {
        size_t pad_size = block_size_ - position % block_size_;
        last_error_ = file_->Append(base::Slice::GetPad(pad_size, &scope));
        if (!last_error_) {
            return position;
        }
        position = RoundUp(position, block_size_);
    }
    return position;
}

uint64_t
XhashTableBuilder::WriteIndexs(const std::vector<std::tuple<uint64_t,
                               uint64_t>> &indexs) {
    uint64_t begin_position = AlignmentToBlock();
    if (!last_error_) {
        return 0;
    }

    base::ScopedMemory scope;
    for (const auto &index : indexs) {
        uint64_t position, size;
        std::tie(position, size) = index;
        Error rs = file_->Append(base::Slice::GetV64(position, &scope));
        if (!rs) {
            return 0;
        }
        rs = file_->Append(base::Slice::GetV64(size, &scope));
        if (!rs) {
            return 0;
        }
    }
    return begin_position;
}

// ordered/unordered
// block-size
// num-entries
// index-position
// smallest-key
// largest-key
// others...
uint64_t XhashTableBuilder::WriteFileProperties() {
    using base::Slice;
    DCHECK(last_error_.ok());
    
    uint64_t begin_position = AlignmentToBlock();
    if (!last_error_) {
        return 0;
    }
    const char unordered = 1;
    last_error_ = file_->Append(std::string_view(&unordered, 1));
    if (!last_error_) {
        return 0;
    }
    
    base::ScopedMemory scope;
    last_error_ = file_->Append(Slice::GetU32(static_cast<uint32_t>(block_size_),
                                              &scope));
    if (!last_error_) {
        return 0;
    }
    last_error_ = file_->Append(Slice::GetU32(static_cast<uint32_t>(num_entries_),
                                              &scope));
    if (!last_error_) {
        return 0;
    }
    last_error_ = file_->Append(Slice::GetU64(index_position_, &scope));
    if (!last_error_) {
        return 0;
    }
    
    last_error_ = file_->Append(Slice::GetV64(smallest_key_.size(), &scope));
    if (!last_error_) {
        return 0;
    }
    last_error_ = file_->Append(smallest_key_);
    if (!last_error_) {
        return 0;
    }

    last_error_ = file_->Append(Slice::GetV64(largest_key_.size(), &scope));
    if (!last_error_) {
        return 0;
    }
    last_error_ = file_->Append(largest_key_);
    if (!last_error_) {
        return 0;
    }
    return begin_position;
}

} // namespace table
    
} // namespace mai
