#include "table/s1-table-reader.h"
#include "table/table.h"
#include "core/key-boundle.h"
#include "core/internal-key-comparator.h"
#include "base/io-utils.h"
#include "base/slice.h"
#include "base/hash.h"
#include "mai/iterator.h"
#include "glog/logging.h"

namespace mai {
    
namespace table {

using ::mai::core::KeyBoundle;
using ::mai::core::ParsedTaggedKey;
using ::mai::base::Slice;

#define TRY_RUN0(expr) \
    (expr); \
    if (!reader.error()) { \
        return reader.error(); \
    } (void)0
    
#define TRY_RUN1(expr) \
    rs = (expr); \
    if (!rs) { \
        return rs; \
    } (void)0
    
class S1TableReader::IteratorImpl final : public Iterator {
public:
    IteratorImpl(const core::InternalKeyComparator *ikcmp, S1TableReader *owns)
    : owns_(DCHECK_NOTNULL(owns))
    , ikcmp_(DCHECK_NOTNULL(ikcmp)) {
        DCHECK(!owns_->index().empty());
    }
    
    virtual ~IteratorImpl() override {}
    
    virtual bool Valid() const override {
        if (error_.ok() && slot_ >= 0 && slot_ < owns_->index_.size()) {
            if (!owns_->index_[slot_].empty()) {
                return current_ >= 0 && current_ < owns_->index_[slot_].size();
            }
        }
        return false;
    }
    
    virtual void SeekToFirst() override {
        slot_ = -1;
        current_ = -1;
        for (int64_t i = 0; i < owns_->index_.size(); ++i) {
            if (!owns_->index_[i].empty()) {
                slot_ = i;
                current_ = 0;
                break;
            }
        }
        saved_key_.clear();
        if (Valid()) {
            UpdateKV();
        }
    }
    
    virtual void SeekToLast() override {
        slot_ = -1;
        current_ = -1;
        error_ = MAI_NOT_SUPPORTED("Can not reserve.");
    }
    
    virtual void Seek(std::string_view target) override {
        slot_ = -1;
        current_ = -1;
        ParsedTaggedKey ikey;
        if (!KeyBoundle::ParseTaggedKey(target, &ikey)) {
            error_ = MAI_CORRUPTION("Bad target key!");
            return;
        }
        
        size_t slot = ikcmp_->Hash(target) % owns_->index_.size();
        if (owns_->index_[slot].empty()) {
            error_ = MAI_NOT_FOUND("No bucket.");
            return;
        }
        slot_ = slot;
        
        std::string scatch;
        std::string_view result;
        bool found = false;
        for (int64_t i = 0; i < owns_->index_[slot_].size(); ++i) {
            Index idx = owns_->index_[slot_][i];
            uint64_t offset = idx.block_offset + idx.offset + 4;
            uint64_t shared_len, private_len;
            error_ = owns_->ReadKey(&offset, &shared_len, &private_len, &result,
                                    &scatch);
            if (error_.fail()) {
                return;
            }
            
            saved_key_ = saved_key_.substr(0, shared_len);
            saved_key_.append(result);
            
            if (owns_->table_props_->last_level) {
                if (ikcmp_->ucmp()->Compare(saved_key_, ikey.user_key) >= 0) {
                    found = true;
                }
            } else {
                if (ikcmp_->Compare(saved_key_, target) >= 0) {
                    found = true;
                }
            }
            if (found) {
                error_ = owns_->ReadValue(&offset, &value_, &saved_value_);
                if (error_.fail()) {
                    return;
                }
                current_ = i;
                break;
            }
        }
        if (!found) {
            error_ = MAI_NOT_FOUND("Seek()");
        }
    }
    
    virtual void Next() override {
        DCHECK(Valid());
        if (current_ == owns_->index_[slot_].size() - 1) { // The last one
            int64_t old = slot_;
            slot_ = -1;
            for (int64_t i = old + 1; i < owns_->index_.size(); ++i) {
                if (!owns_->index_[i].empty()) {
                    slot_ = i;
                    current_ = 0;
                    saved_key_.clear();
                    break;
                }
            }
        } else {
            current_++;
        }
        if (Valid()) { UpdateKV(); }
    }
    
    virtual void Prev() override {
        error_ = MAI_NOT_SUPPORTED("Can not reserve.");
    }
    
    virtual std::string_view key() const override {
        DCHECK(Valid());
        return saved_key_;
    }
    
    virtual std::string_view value() const override {
        DCHECK(Valid());
        return value_;
    }
    
    virtual Error error() const override { return error_; }
    
private:
    Error UpdateKV() {
        Error rs;
        std::string scatch;
        std::string_view result;

        Index idx = owns_->index_[slot_][current_];
        uint64_t offset = idx.block_offset + idx.offset + 4;
        uint64_t shared_len, private_len;
        TRY_RUN1(owns_->ReadKey(&offset, &shared_len, &private_len, &result,
                                &scatch));

        saved_key_ = saved_key_.substr(0, shared_len);
        saved_key_.append(result);
        
        TRY_RUN1(owns_->ReadValue(&offset, &value_, &saved_value_));
        return Error::OK();
    }
    
    const S1TableReader *const owns_;
    const core::InternalKeyComparator *const ikcmp_;
    int64_t slot_ = -1;
    int64_t current_ = -1;
    std::string saved_key_;
    std::string_view value_;
    std::string saved_value_;
    Error error_;
}; // class S1TableReader::IteratorImpl
    
S1TableReader::S1TableReader(RandomAccessFile *file, uint64_t file_size,
                             bool checksum_verify)
    : file_(DCHECK_NOTNULL(file))
    , file_size_(file_size)
    , checksum_verify_(checksum_verify) {}

/*virtual*/ S1TableReader::~S1TableReader() {}

Error S1TableReader::Prepare() {
    if (file_size_ < 12) {
        return MAI_CORRUPTION("Incorrect file type. File too small.");
    }
    base::RandomAccessFileReader reader(file_);
    
    uint32_t magic_number = 0;
    TRY_RUN0(magic_number = reader.ReadFixed32(file_size_ - 4));
    if (magic_number != Table::kS1tMagicNumber) {
        return MAI_CORRUPTION("Incorrect file type. Required: s1t.");
    }
    
    uint64_t position = 0;
    TRY_RUN0(position = reader.ReadFixed64(file_size_ - 12));
    if (position >= file_size_ - 12) {
        return MAI_CORRUPTION("Incorrect table properties position.");
    }
    std::string_view result;
    std::string scatch;
    Error rs;
    TRY_RUN1(ReadBlock({position, file_size_ - 12 - position}, &result, &scatch));
    
    table_props_.reset(new TableProperties);
    TRY_RUN1(Table::ReadProperties(result, table_props_.get()));
    
    TRY_RUN1(ReadBlock({table_props_->index_position, table_props_->index_count},
                       &result, &scatch));
    
    base::BufferReader rd(result);
    while (!rd.Eof()) {
        std::vector<Index> n;
        uint64_t size = rd.ReadVarint64();
        for (uint64_t i = 0; i < size; ++i) {
            Index idx;
            idx.block_offset = rd.ReadVarint64();
            idx.offset       = rd.ReadVarint32();
            n.push_back(idx);
        }
        index_.push_back(n);
    }
    return Error::OK();
}

/*virtual*/ Iterator *
S1TableReader::NewIterator(const ReadOptions &read_opts,
                           const core::InternalKeyComparator *ikcmp) {
    if (index_.empty()) {
        return Iterator::AsError(MAI_CORRUPTION("Not prepare yet."));
    }
    return new IteratorImpl(ikcmp, this);
}

/*virtual*/ Error S1TableReader::Get(const ReadOptions &read_opts,
                  const core::InternalKeyComparator *ikcmp,
                  std::string_view key,
                  core::Tag *tag,
                  std::string_view *value,
                  std::string *scratch) {
    ParsedTaggedKey ikey;
    KeyBoundle::ParseTaggedKey(key, &ikey);
    
    size_t slot = ikcmp->Hash(key) % index_.size();
    if (index_[slot].empty()) {
        return MAI_NOT_FOUND("Key not exists in bucket.");
    }
    
    Error rs;
    std::string_view result;
    std::string saved_key;
    bool found = false;
    for (const auto &idx : index_[slot]) {
        uint64_t offset = idx.block_offset + idx.offset + 4;
        uint64_t shared_len, private_len;
        
        TRY_RUN1(ReadKey(&offset, &shared_len, &private_len, &result, scratch));
        saved_key = saved_key.substr(0, shared_len);
        saved_key.append(result);
    
        
        if (table_props_->last_level) {
            if (ikcmp->ucmp()->Compare(saved_key, ikey.user_key) >= 0) {
                found = true;
            }
        } else {
            if (ikcmp->Compare(saved_key, key) >= 0) {
                found = true;
            }
        }
        if (found) {
            TRY_RUN1(ReadValue(&offset, value, scratch));
            break;
        }
    }
    if (!found) {
        return MAI_NOT_FOUND("Key not exists.");
    }
    if (table_props_->last_level) {
        if (!ikcmp->ucmp()->Equals(saved_key, ikey.user_key)) {
            return MAI_NOT_FOUND("Key not exists.");
        }
    } else {
        if (!ikcmp->ucmp()->Equals(KeyBoundle::ExtractUserKey(saved_key),
                                   ikey.user_key)) {
            return MAI_NOT_FOUND("Key not exists.");
        }
        if (tag) {
            *tag = KeyBoundle::ExtractTag(saved_key);
        }
    }
    return Error::OK();
}

/*virtual*/ size_t S1TableReader::ApproximateMemoryUsage() const {
    size_t usage = sizeof(*this);
    for (const auto &n : index_) {
        usage += (sizeof(index_[0]) + n.size() * sizeof(Index));
    }
    return usage;
}

/*virtual*/
std::shared_ptr<TableProperties> S1TableReader::GetTableProperties() const {
    return table_props_;
}

/*virtual*/
std::shared_ptr<core::KeyFilter> S1TableReader::GetKeyFilter() const {
    return filter_;
}
    
Error S1TableReader::ReadBlock(const BlockHandle &bh, std::string_view *result,
                               std::string *scatch) const {
    Error rs = file_->Read(bh.offset(), bh.size(), result, scatch);
    if (!rs) {
        return rs;
    }
    if (checksum_verify_) {
        uint32_t checksum = *reinterpret_cast<const uint32_t *>(result->data());
        
        if (checksum != ::crc32(0, result->data() + 4, result->size() - 4)) {
            return MAI_IO_ERROR("CRC32 checksum fail!");
        }
    }
    result->remove_prefix(4); // remove crc32 checksum.
    return Error::OK();
}
    
Error S1TableReader::ReadKey(uint64_t *offset, uint64_t *shared_len,
                             uint64_t *private_len, std::string_view *result,
                             std::string *scatch) const {
    base::RandomAccessFileReader reader(file_);
    size_t len = 0;
    
    TRY_RUN0(*shared_len = reader.ReadVarint64(*offset, &len));
    *offset += len;
    TRY_RUN0(*private_len = reader.ReadVarint64(*offset, &len));
    *offset += len;
    TRY_RUN0(*result = reader.Read(*offset, *private_len, scatch));
    *offset += result->size();
    return Error::OK();
}
    
Error S1TableReader::ReadValue(uint64_t *offset, std::string_view *result,
                               std::string *scatch) const {
    base::RandomAccessFileReader reader(file_);
    size_t len = 0;

    uint64_t value_len;
    TRY_RUN0(value_len = reader.ReadVarint64(*offset, &len));
    *offset += len;
    TRY_RUN0(*result = reader.Read(*offset, value_len, scatch));
    *offset += result->size();
    return Error::OK();
}
    
} // namespace table
    
} // namespace mai
