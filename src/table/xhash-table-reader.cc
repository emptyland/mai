#include "table/xhash-table-reader.h"
#include "table/table.h"
#include "core/key-boundle.h"
#include "core/internal-key-comparator.h"
#include "base/slice.h"
#include "mai/env.h"
#include "mai/iterator.h"
#include "mai/comparator.h"

namespace mai {
    
namespace table {
    
class XhashTableReader::IteratorImpl : public Iterator {
public:
    IteratorImpl(const core::InternalKeyComparator *ikcmp,
                 XhashTableReader *reader)
        : ikcmp_(DCHECK_NOTNULL(ikcmp))
        , reader_(DCHECK_NOTNULL(reader))
        , slot_(reader->indexs_.size()) {}
    
    virtual ~IteratorImpl() {}
    
    virtual bool Valid() const override {
        return slot_ < reader_->indexs_.size();
    }
    
    virtual void SeekToFirst() override {
        for (size_t i = 0; i < reader_->indexs_.size(); ++i) {
            if (reader_->indexs_[i].size != 0) {
                slot_ = i;
                offset_ = reader_->indexs_[i].offset;
                break;
            }
        }
        if (Valid()) {
            key_.clear();
            next_offset_ = SetUpKV(offset_);
        }
    }
    
    virtual void SeekToLast() override { Noreached(); }
    
    virtual void Seek(std::string_view target) override {
        core::ParsedTaggedKey tk;
        core::KeyBoundle::ParseTaggedKey(target, &tk);
        
        hash_func_t hash = reader_->hash_func_;
        uint32_t hash_val = hash(tk.user_key.data(), tk.user_key.size());
        slot_ = hash_val % reader_->indexs_.size();
        Index index = reader_->indexs_[slot_];
        if (index.size == 0) {
            error_ = MAI_NOT_FOUND("Key not seek.");
            slot_ = reader_->indexs_.size();
            return;
        }
        
        key_.clear();
        do {
            offset_ = index.offset;
            next_offset_ = SetUpKV(offset_);

            if (reader_->table_props_->last_level) {
                if (ikcmp_->ucmp()->Compare(key_, tk.user_key) >= 0) {
                    break;
                }
            } else {
                if (ikcmp_->Compare(key_, target) >= 0) {
                    break;
                }
            }
            offset_ = next_offset_;
        } while (offset_ < index.offset + index.size);
    }
    
    virtual void Next() override {
        DCHECK(Valid());
        Index index = reader_->indexs_[slot_];
        if (next_offset_ >= index.offset + index.size) {
            while (++slot_ < reader_->indexs_.size()) {
                if (reader_->indexs_[slot_].size > 0) {
                    next_offset_ = reader_->indexs_[slot_].offset;
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
    
    virtual void Prev() override { Noreached(); }
    
    virtual std::string_view key() const override {
        DCHECK(Valid()); return key_;
    }
    virtual std::string_view value() const override {
        DCHECK(Valid()); return value_;
    }
    virtual Error error() const override { return error_; }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(IteratorImpl);
private:
    void Noreached() const {
        DLOG(FATAL) << "Noreached! " << error_.ToString();
    }
    
    uint64_t SetUpKV(uint64_t initial) {
        DCHECK(Valid());
        std::string scratch;
        std::string_view result;
        
        uint64_t offset = initial;
        error_ = reader_->ReadKey(&offset, &result, &scratch);
        if (!error_) {
            return 0;
        }
        if (reader_->table_props_->last_level) {
            key_ = result;
        } else {
            if (result.size() == core::KeyBoundle::kMinSize) {
                key_.assign(core::KeyBoundle::ExtractUserKey(key_));
                key_.append(result);
            } else {
                key_.assign(result);
            }
        }
        
        uint64_t value_len;
        error_ = reader_->ReadLength(&offset, &value_len);
        if (!error_) {
            return 0;
        }
        error_ = reader_->file_->Read(offset, value_len, &result, &scratch);
        if (!error_) {
            return 0;
        }
        value_ = result;
        offset += result.size();
        return offset;
    }
    
    const core::InternalKeyComparator *ikcmp_;
    XhashTableReader *reader_;
    size_t slot_;
    
    uint64_t offset_ = 0;
    uint64_t next_offset_ = 0;
    std::string key_;
    std::string value_;
    Error error_;
}; // class XhashTableReader::Iterator

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

/*virtual*/ Iterator *
XhashTableReader::NewIterator(const ReadOptions &,
                              const core::InternalKeyComparator *ikcmp) {
    Error err;
    if (!table_props_) {
        err = MAI_CORRUPTION("Table reader not prepared!");
    }
    if (err.ok() && indexs_.empty()) {
        err = MAI_CORRUPTION("Table reader not prepared! Can not find any index.");
    }
    if (err.fail()) {
        return Iterator::AsError(err);
    }
    return new IteratorImpl(ikcmp, this);
}
/*virtual*/ Error
XhashTableReader::Get(const ReadOptions &,
                      const core::InternalKeyComparator *ikcmp,
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
            key = result;
            last_key = result;
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
        
        bool found = false;
        if (table_props_->last_level) {
            if (ikcmp->ucmp()->Compare(key, tk.user_key) >= 0) {
                found = true;
            }
        } else {
            if (ikcmp->Compare(key, target) >= 0) {
                found = true;
            }
        }
        if (found) {
            rs = file_->Read(slot.offset, value_len, value, scratch);
            if (!rs) {
                return rs;
            }
            if (!table_props_->last_level && tag) {
                *tag = core::KeyBoundle::ExtractTag(key);
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
