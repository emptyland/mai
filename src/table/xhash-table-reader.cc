#include "table/xhash-table-reader.h"
#include "table/table.h"
#include "core/key-boundle.h"
#include "base/slice.h"
#include "mai/env.h"
#include "mai/comparator.h"

namespace mai {
    
namespace table {

/*virtual*/ XhashTableReader::~XhashTableReader() {}

Error XhashTableReader::Prepare() {
    using base::Slice;
    using base::Varint32;
    using base::Varint64;
    
    if (file_size_ < 12) {
        return MAI_CORRUPTION("Incorrect file type. File too small.");
    }

    std::string_view result;
    std::string scratch;
    
    Error rs = file_->Read(file_size_ - 4, 4, &result, &scratch);
    if (!rs) {
        return rs;
    }
    if (Slice::SetU32(result) != Table::kXmtMagicNumber) {
        return MAI_CORRUPTION("Incorrect file type, required: xmt");
    }
    
    rs = file_->Read(file_size_ - 12, 8, &result, &scratch);
    if (!rs) {
        return rs;
    }
    uint64_t position = Slice::SetU64(result);
    if (position >= file_size_ - 12) {
        return MAI_CORRUPTION("Incorrect table properties position.");
    }
    
    table_props_.reset(new TableProperties{});
    rs = Table::ReadProperties(file_, &position, table_props_.get());
    if (!rs) {
        return rs;
    }
    
    position = table_props_->index_position;
    size_t count = table_props_->index_count;
    while (count--) {
        rs = file_->Read(position, Varint64::kMaxLen, &result, &scratch);
        if (!rs) {
            return rs;
        }
        
        size_t varint_len = 0;
        uint64_t offset = Varint64::Decode(result.data(), &varint_len);
        position += varint_len;
        
        rs = file_->Read(position, Varint64::kMaxLen, &result, &scratch);
        if (!rs) {
            return rs;
        }
        uint64_t size = Varint64::Decode(result.data(), &varint_len);
        position += varint_len;
        
        indexs_.push_back({offset, size});
    }

    return Error::OK();
}

/*virtual*/ core::InternalIterator *
XhashTableReader::NewIterator(const ReadOptions &read_opts,
                              const Comparator *cmp) {
    // TODO:
    return nullptr;
}
/*virtual*/ Error XhashTableReader::Get(const ReadOptions &,
                  const Comparator *ikcmp,
                  std::string_view target,
                  core::Tag *tag,
                  std::string_view *value,
                  std::string *scratch) {
    using base::Slice;
    using base::Varint32;
    using base::Varint64;
    
    if (!table_props_) {
        return MAI_CORRUPTION("Table reader not prepared!");
    }
    if (indexs_.empty()) {
        return MAI_CORRUPTION("Table reader not prepared! Can not find any index.");
    }
    
    core::ParsedTaggedKey tk;
    core::KeyBoundle::ParseTaggedKey(target, &tk);
    
    uint32_t hash_val = hash_func_(tk.user_key.data(), tk.user_key.size());
    Index slot = indexs_[hash_val % indexs_.size()];
    
    std::string_view result;
    std::string last_key;
    uint64_t end = slot.offset + slot.size;
    while(slot.offset < end) {
        auto rs = ReadKey(&slot.offset, &result, scratch);
        if (!rs) {
            return rs;
        }

        std::string key;
        if (table_props_->last_level) {
            if (result.size() == 0) {
                DCHECK(!last_key.empty());
                key = last_key;
            } else if (result != last_key) {
                key = result;
                last_key = result;
            }
        } else {
            if (result.size() == core::KeyBoundle::kMinSize) {
                DCHECK(!last_key.empty());
                key.append(core::KeyBoundle::ExtractUserKey(last_key));
                key.append(result);
            } else if (result != last_key) {
                key = result;
                last_key = result;
            }
        }
        
        uint64_t value_len;
        rs = ReadLength(&slot.offset, &value_len);
        if (!rs) {
            return rs;
        }
        if (ikcmp->Compare(key, target) >= 0) {
            rs = file_->Read(slot.offset, value_len, value, scratch);
            if (!rs) {
                return rs;
            }
            core::ParsedTaggedKey lk;
            core::KeyBoundle::ParseTaggedKey(key, &lk);
            
            if (tag) {
                *tag = lk.tag;
            }
            return Error::OK();
        }
        slot.offset += value_len;
    }
    
    return MAI_NOT_FOUND("Not Found");
}
    
/*virtual*/ size_t XhashTableReader::ApproximateMemoryUsage() const {
    const size_t kPageSize = 4 * base::kKB;

    size_t size = sizeof(*this);
    size += sizeof(Index) * indexs_.size();
    size += RoundUp(file_size_, kPageSize);
    return size;
}
    
/*virtual*/ std::shared_ptr<TableProperties>
XhashTableReader::GetTableProperties() const { return table_props_; }


Error XhashTableReader::ReadKey(uint64_t *offset, std::string_view *result,
                                std::string *scratch) {
    using base::Slice;
    using base::Varint32;
    using base::Varint64;
    
    uint64_t key_len;
    Error rs = ReadLength(offset, &key_len);
    if (!rs) {
        return rs;
    }
    
    if (table_props_->last_level) {
        if (key_len == 0) {
            *result = "";
        } else {
            rs = file_->Read(*offset, key_len, result, scratch);
            if (!rs) {
                return rs;
            }
            (*offset) += result->size();
        }
    } else {
        DCHECK_GE(key_len, core::KeyBoundle::kMinSize);
        rs = file_->Read(*offset, key_len, result, scratch);
        if (!rs) {
            return rs;
        }
        (*offset) += result->size();
    }
    return Error::OK();
}
    
Error XhashTableReader::ReadLength(uint64_t *offset, uint64_t *len) {
    using base::Slice;
    using base::Varint32;
    using base::Varint64;
    
    std::string_view result;
    std::string scratch;
    
    Error rs = file_->Read(*offset, Varint64::kMaxLen, &result, &scratch);
    if (!rs) {
        return rs;
    }
    size_t varint_len = 0;
    *len = Varint64::Decode(result.data(), &varint_len);
    (*offset) += varint_len;
    return Error::OK();
}
    
} // namespace table
    
} // namespace mai
