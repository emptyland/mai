#include "table/hash-table-reader.h"
#include "table/table.h"
#include "core/key-boundle.h"
#include "core/internal-key-comparator.h"
#include "base/slice.h"
#include "mai/env.h"
#include "mai/iterator.h"
#include "mai/comparator.h"

namespace mai {
    
namespace table {

////////////////////////////////////////////////////////////////////////////////
/// class HashTableReader::Iterator
////////////////////////////////////////////////////////////////////////////////

class HashTableReader::IteratorImpl : public Iterator {
public:
    IteratorImpl(const core::InternalKeyComparator *ucmp, HashTableReader *reader)
        : ikcmp_(DCHECK_NOTNULL(ucmp))
        , reader_(DCHECK_NOTNULL(reader))
        , slot_(reader->index_.size()) {
        DCHECK_GT(reader_->index_.size(), 0);
    }
    virtual ~IteratorImpl() {}
    
    virtual bool Valid() const override {
        return slot_ < reader_->index_.size();
    }
    virtual void SeekToFirst() override {
        for (size_t i = 0; i < reader_->index_.size(); ++i) {
            if (reader_->index_[i].size != 0) {
                slot_ = i;
                offset_ = reader_->index_[i].offset;
                break;
            }
        }
        if (Valid()) {
            next_offset_ = SetUpKV(offset_);
        }
    }
    virtual void SeekToLast() override { DLOG(FATAL) << "Not Supported"; }
    
    virtual void Seek(std::string_view target) override {
        core::ParsedTaggedKey tk;
        core::KeyBoundle::ParseTaggedKey(target, &tk);
        
        hash_func_t hash = reader_->hash_func_;
        uint32_t hash_val = hash(tk.user_key.data(), tk.user_key.size())
            & 0x7fffffff;
        slot_ = hash_val % reader_->index_.size();
        Index index = reader_->index_[slot_];
        if (index.size == 0) {
            latest_error_ = MAI_NOT_FOUND("Key not seek.");
            slot_ = reader_->index_.size();
            return;
        }
        
        core::ParsedTaggedKey lk;
        do {
            offset_ = index.offset;
            next_offset_ = SetUpKV(offset_);
            
            core::KeyBoundle::ParseTaggedKey(key(), &lk);
            if (ikcmp_->ucmp()->Equals(tk.user_key, lk.user_key) &&
                tk.tag.version() >= lk.tag.version()) {
                break;
            }
            offset_ = next_offset_;
        } while (offset_ < index.offset + index.size);
    }
    
    virtual void Next() override {
        DCHECK(Valid());
        Index index = reader_->index_[slot_];
        if (next_offset_ >= index.offset + index.size) {
            while (++slot_ < reader_->index_.size()) {
                if (reader_->index_[slot_].size > 0) {
                    next_offset_ = reader_->index_[slot_].offset;
                    break;
                }
            }
            if (!Valid()) {
                return;
            }
        }
        offset_ = next_offset_;
        next_offset_ = SetUpKV(offset_);
    }
    virtual void Prev() override { DLOG(FATAL) << "Not Supported"; }
    
    virtual std::string_view key() const override {
        DCHECK(Valid()); return key_;
    }
    virtual std::string_view value() const override {
        DCHECK(Valid()); return value_;
    }
    
    virtual Error error() const override { return latest_error_; }
    
private:
    uint32_t SetUpKV(uint32_t initial) {
        DCHECK(Valid());
        std::string scratch;
        std::string_view key;
        
        uint32_t offset = initial;
        latest_error_ = reader_->ReadLengthItem(offset, &key, &scratch);
        if (!latest_error_) {
            return 0;
        }
        offset += 4 + key.size();
        if (!key.empty()) {
            key_ = key;
        }
        
        std::string_view value;
        latest_error_ = reader_->ReadLengthItem(offset, &value, &scratch);
        if (!latest_error_) {
            return 0;
        }
        offset += 4 + value.size();
        value_ = value;
        return offset;
    }
    
    const core::InternalKeyComparator *const ikcmp_;
    HashTableReader *reader_;
    size_t slot_;
    
    uint32_t offset_ = 0;
    uint32_t next_offset_ = 0;
    std::string key_;
    std::string value_;
    Error latest_error_;
}; // class HashTableReader::Iterator
    
////////////////////////////////////////////////////////////////////////////////
/// class HashTableReader
////////////////////////////////////////////////////////////////////////////////

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
    

/*virtual*/ Iterator *
HashTableReader::NewIterator(const ReadOptions &,
                             const core::InternalKeyComparator *ikcmp) {
    return new IteratorImpl(ikcmp, this);
}

/*virtual*/ Error
HashTableReader::Get(const ReadOptions &,
                     const core::InternalKeyComparator *ikcmp,
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
        auto rs = ReadLengthItem(slot.offset, &data, &buf);
        if (!rs) {
            return rs;
        }
        slot.offset += 4 + data.size();
        
        core::ParsedTaggedKey lk;
        if (data.size() > 0) {
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
        
        if (ikcmp->ucmp()->Equals(tk.user_key, lk.user_key) &&
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
    
Error HashTableReader::ReadLengthItem(uint64_t offset, std::string_view *item,
                                std::string *scratch) {
    auto rs = file_->Read(offset, 4, item, scratch);
    if (!rs) {
        return rs;
    }
    uint32_t length = base::Slice::SetU32(*item);
    if (length > 0) {
        return file_->Read(offset + 4, length, item, scratch);
    } else {
        *item = "";
    }
    return Error::OK();
}
    
} // namespace table
    
} // namespace mai
