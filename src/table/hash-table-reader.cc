#include "table/hash-table-reader.h"
#include "table/table.h"
#include "core/key-boundle.h"
#include "base/slice.h"
#include "mai/env.h"
#include "mai/comparator.h"

namespace mai {
    
namespace table {

/*virtual*/ HashTableReader::~HashTableReader() {}
    
Error HashTableReader::Prepare() {
    std::string scratch;
    std::string_view data;
    auto rs = file_->Read(file_size_ - 4, 4, &data, &scratch);
    if (!rs) {
        return rs;
    }
    if (data.size() != 4) {
        return MAI_CORRUPTION("Bad HMT file.");
    }
    if (base::Slice::SetU32(data) != Table::kHmtMagicNumber) {
        return MAI_CORRUPTION("Bad HMT file.");
    }
    
    rs = file_->Read(file_size_ - 8, 4, &data, &scratch);
    if (!rs) {
        return rs;
    }
    uint32_t index_position = base::Slice::SetU32(data);
    uint32_t index_size = static_cast<uint32_t>(file_size_) - 8 - index_position;
    rs = file_->Read(index_position, index_size, &data, &scratch);
    if (!rs) {
        return rs;
    }
    for (const char *p = data.data(); p != data.data() + data.size(); p += 8) {
        Index index;
        index.offset = *reinterpret_cast<const uint32_t *>(p);
        index.size   = *reinterpret_cast<const uint32_t *>(p + 4);
        index_.push_back(index);
    }
    return Error::OK();
}
    

/*virtual*/ core::InternalIterator *
HashTableReader::NewIterator(const ReadOptions &read_opts,
                             const Comparator *ucmp) {
    
    // TODO:
    return nullptr;
}

/*virtual*/ Error
HashTableReader::Get(const ReadOptions &, const Comparator *ucmp,
                     std::string_view key, core::Tag *tag,
                     std::string_view *value, std::string *scratch) {
    if (index_.empty()) {
        return MAI_CORRUPTION("Not initialize yet");
    }
    core::ParsedTaggedKey tk;
    core::KeyBoundle::ParseTaggedKey(key, &tk);
    
    uint32_t hash_val = hash_func_(tk.user_key.data(), tk.user_key.size())
        & 0x7fffffff;
    Index slot = index_[hash_val % index_.size()];
    
    std::string buf, latest_key;
    std::string_view data;
    uint32_t end = slot.offset + slot.size;
    while(slot.offset < end) {
        auto rs = file_->Read(slot.offset, 4, &data, &buf);
        if (!rs) {
            return rs;
        }
        uint32_t key_size = base::Slice::SetU32(data);
        slot.offset += data.size();
        
        core::ParsedTaggedKey lk;
        if (key_size > 0) {
            rs = file_->Read(slot.offset, key_size, &data, &buf);
            if (!rs) {
                return rs;
            }
            slot.offset += data.size();
            
            if (data != latest_key) {
                latest_key = data;
                core::KeyBoundle::ParseTaggedKey(data, &lk);
            }
        }

        rs = file_->Read(slot.offset, 4, &data, &buf);
        if (!rs) {
            return rs;
        }
        uint32_t val_size = base::Slice::SetU32(data);
        slot.offset += data.size();
        
        if (ucmp->Equals(tk.user_key, lk.user_key) &&
            tk.tag.version() >= lk.tag.version()) {
            rs = file_->Read(slot.offset, val_size, value, scratch);
            if (!rs) {
                return rs;
            }
            if (tag) {
                *tag = lk.tag;
            }
            return Error::OK();
        }
        
        slot.offset += val_size;
    }
    
    return MAI_NOT_FOUND("Not Found");
}

/*virtual*/ size_t HashTableReader::ApproximateMemoryUsage() const {
    // TODO:
    return 0;
}
    
} // namespace table
    
} // namespace mai
