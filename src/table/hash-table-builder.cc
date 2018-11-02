#include "table/hash-table-builder.h"
#include "table/table.h"
#include "core/key-boundle.h"
#include "base/slice.h"
#include "mai/env.h"
#include "glog/logging.h"

namespace mai {
    
namespace table {
    
HashTableBuilder::HashTableBuilder(WritableFile *file,
                                   size_t max_hash_slots,
                                   hash_func_t hash_func,
                                   uint32_t block_size)
    : file_(DCHECK_NOTNULL(file))
    , max_hash_slots_(max_hash_slots)
    , hash_func_(DCHECK_NOTNULL(hash_func))
    , block_size_(block_size) {
    DCHECK_GE(block_size_, 512) << "block_size too small.";
}

/*virtual*/ HashTableBuilder::~HashTableBuilder() {}

/*virtual*/
void HashTableBuilder::Add(std::string_view key, std::string_view value) {
    core::ParsedTaggedKey tk;
    core::KeyBoundle::ParseTaggedKey(key, &tk);
    
    if (tk.tag.flags() != core::Tag::kFlagValue &&
        tk.tag.flags() != core::Tag::kFlagDeletion) {
        latest_error_ = MAI_NOT_SUPPORTED("Key flag not supported!");
        return;
    }
    
    if (!has_seen_first_key_) {
        if (tk.tag.version() == 0) {
            is_latest_level_ = true;
        }
        has_seen_first_key_ = true;
        
        index_buckets_.resize(max_hash_slots_);
    }
    uint64_t start_position = 0;
    latest_error_ = file_->GetFileSize(&start_position);
    if (!latest_error_.ok()) {
        return;
    }
    
    base::ScopedMemory scope;
    
    uint32_t hash_val = hash_func_(tk.user_key.data(), tk.user_key.size());
    size_t slot = (hash_val & 0x7fffffff) % max_hash_slots_;
    if (slot == latest_slot_) {
        uint32_t key_size;
        if (is_latest_level_) {
            if (tk.user_key == latest_key_) {
                key_size = 0;
            } else {
                key_size = static_cast<uint32_t>(tk.user_key.size());
            }
        } else {
            if (key == latest_key_) {
                key_size = 0;
            } else {
                key_size = static_cast<uint32_t>(key.size());
            }
        }
        latest_error_ = file_->Append(base::Slice::GetU32(key_size, &scope));
        if (!latest_error_.ok()) {
            return;
        }
        if (is_latest_level_) {
            latest_error_ = file_->Append(tk.user_key);
        } else {
            latest_error_ = file_->Append(key);
        }
        if (!latest_error_.ok()) {
            return;
        }
        
        uint32_t val_size = static_cast<uint32_t>(value.size());
        latest_error_ = file_->Append(base::Slice::GetU32(val_size, &scope));
        if (!latest_error_.ok()) {
            return;
        }
        
        latest_error_ = file_->Append(value);
        if (!latest_error_.ok()) {
            return;
        }
    } else {
        uint32_t key_size = is_latest_level_ ?
            static_cast<uint32_t>(tk.user_key.size()) :
            static_cast<uint32_t>(key.size());
        latest_error_ = file_->Append(base::Slice::GetU32(key_size, &scope));
        if (!latest_error_.ok()) {
            return;
        }
        
        if (is_latest_level_) {
            latest_error_ = file_->Append(tk.user_key);
        } else {
            latest_error_ = file_->Append(key);
        }
        if (!latest_error_.ok()) {
            return;
        }
        
        uint32_t val_size = static_cast<uint32_t>(value.size());
        latest_error_ = file_->Append(base::Slice::GetU32(val_size, &scope));
        if (!latest_error_.ok()) {
            return;
        }
        
        latest_error_ = file_->Append(value);
        if (!latest_error_.ok()) {
            return;
        }
        
        index_buckets_[slot].offset = static_cast<uint32_t>(start_position);
        uint64_t current_position = 0;
        latest_error_ = file_->GetFileSize(&current_position);
        if (!latest_error_.ok()) {
            return;
        }
        if (latest_slot_ != -1) {
            index_buckets_[latest_slot_].size =
                static_cast<uint32_t>(current_position)
                - index_buckets_[latest_slot_].offset;
        }
        latest_slot_ = slot;
    }
    num_entries_++;
}

/*virtual*/ Error HashTableBuilder::error() { return latest_error_; }
    
/*virtual*/ Error HashTableBuilder::Finish() {
    uint64_t index_position = 0;
    Error rv = file_->GetFileSize(&index_position);
    if (!rv.ok()) {
        return rv;
    }
    
    base::ScopedMemory scope;
    if (index_position % block_size_) {
        size_t pad_size = block_size_ - index_position % block_size_;
        rv = file_->Append(base::Slice::GetPad(pad_size, &scope));
        if (!rv.ok()) {
            return rv;
        }
        index_position = RoundUp(index_position, block_size_);
    }
    
    
    for (IndexBucket bucket : index_buckets_) {
        rv = file_->Append(base::Slice::GetBlock(&bucket));
        if (!rv.ok()) {
            return rv;
        }
    }
    
    // Footer
    rv = file_->Append(base::Slice::GetU32(static_cast<uint32_t>(index_position),
                                           &scope));
    if (!rv.ok()) {
        return rv;
    }
    // hmt
    rv = file_->Append(base::Slice::GetU32(Table::kHmtMagicNumber, &scope));
    if (!rv.ok()) {
        return rv;
    }
    return Error::OK();
}
    
/*virtual*/ void HashTableBuilder::Abandon() {
    // TODO:
    
    latest_error_ = Error::OK();
}

/*virtual*/ uint64_t HashTableBuilder::NumEntries() const {
    return num_entries_;
}

/*virtual*/ uint64_t HashTableBuilder::FileSize() const {
    uint64_t size = 0;
    file_->GetFileSize(&size);
    return size;
}

} // namespace table
    
} // namespace mai
