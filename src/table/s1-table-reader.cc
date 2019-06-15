#include "table/s1-table-reader.h"
#include "table/key-bloom-filter.h"
#include "table/block-cache.h"
#include "table/table.h"
#include "core/key-boundle.h"
#include "core/internal-key-comparator.h"
#include "core/key-filter.h"
#include "base/io-utils.h"
#include "base/slice.h"
#include "base/hash.h"
#include "mai/options.h"
#include "mai/iterator.h"
#include "glog/logging.h"

namespace mai {
    
namespace table {

using ::mai::core::Tag;
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
    
    
////////////////////////////////////////////////////////////////////////////////
/// class S1TableReader::IteratorImpl
////////////////////////////////////////////////////////////////////////////////
    
class S1TableReader::IteratorImpl final : public Iterator {
public:
    IteratorImpl(const core::InternalKeyComparator *ikcmp,
                 const ReadOptions &read_opts, S1TableReader *owns)
    : owns_(DCHECK_NOTNULL(owns))
    , ikcmp_(DCHECK_NOTNULL(ikcmp))
    , read_opts_(read_opts) {
        DCHECK(owns_->has_initialized_);
    }
    
    virtual ~IteratorImpl() override {}
    
    virtual bool Valid() const override {
        if (error_.ok() && slot_ >= 0 && slot_ < owns_->index_size_) {
            if (!owns_->index_[slot_].empty()) {
                return current_ >= 0 && current_ < owns_->index_[slot_].size();
            }
        }
        return false;
    }
    
    virtual void SeekToFirst() override {
        slot_ = -1;
        current_ = -1;
        for (int64_t i = 0; i < owns_->index_size_; ++i) {
            if (!owns_->index_[i].empty()) {
                slot_ = i;
                current_ = 0;
                break;
            }
        }
        saved_key_.clear();
        if (Valid()) {
            UpdateEntry();
        }
    }
    
    virtual void SeekToLast() override {
        slot_ = -1;
        current_ = -1;
        error_ = MAI_NOT_SUPPORTED("Can not reserve.");
    }
    
    virtual void Seek(std::string_view target) override;
    
    virtual void Next() override {
        DCHECK(Valid());
        if (current_ == owns_->index_[slot_].size() - 1) { // The last one
            int64_t old = slot_;
            slot_ = -1;
            for (int64_t i = old + 1; i < owns_->index_size_; ++i) {
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
        if (Valid()) { UpdateEntry(); }
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
    Error UpdateEntry();
    
    const S1TableReader *const owns_;
    const core::InternalKeyComparator *const ikcmp_;
    ReadOptions const read_opts_;
    int64_t slot_ = -1;
    int64_t current_ = -1;
    std::string saved_key_;
    std::string_view value_;
    std::string saved_value_;
    Error error_;
}; // class S1TableReader::IteratorImpl
    

void S1TableReader::IteratorImpl::Seek(std::string_view target) {
    slot_ = -1;
    current_ = -1;
    ParsedTaggedKey lookup;
    if (!KeyBoundle::ParseTaggedKey(target, &lookup)) {
        error_ = MAI_CORRUPTION("Bad target key!");
        return;
    }
    
    size_t slot = ikcmp_->Hash(target) % owns_->index_size_;
    if (owns_->index_[slot].empty()) {
        error_ = MAI_NOT_FOUND("No bucket.");
        return;
    }
    slot_ = slot;
    
    base::intrusive_ptr<core::LRUHandle> handle;
    std::string scatch;
    std::string_view result;
    bool found = false;
    for (int64_t i = 0; i < owns_->index_[slot_].size(); ++i) {
        const auto &idx = owns_->index_[slot_][i];
        std::string_view buf;
        error_ = owns_->PrepareRead(idx, read_opts_, &buf, &handle);
        if (error_.fail()) {
            return;
        }

        uint64_t shared_len, private_len;
        auto bytes = owns_->ReadKey(buf, &shared_len, &private_len, &result);
        buf.remove_prefix(bytes);
        
        saved_key_ = saved_key_.substr(0, shared_len);
        saved_key_.append(result);
        
        if (owns_->table_props_->last_level) {
            if (ikcmp_->ucmp()->Equals(saved_key_, lookup.user_key)) {
                found = true;
                // fill fake tag for last level key.
                uint64_t tag_stub = Tag(0, Tag::kFlagValue).Encode();
                saved_key_.append(reinterpret_cast<const char *>(&tag_stub),
                                  8);
            }
        } else {
            ParsedTaggedKey ikey;
            KeyBoundle::ParseTaggedKey(saved_key_, &ikey);
            if (ikcmp_->ucmp()->Equals(ikey.user_key, lookup.user_key) &&
                ikey.tag.sequence_number() <= lookup.tag.sequence_number()) {
                found = true;
            }
        }
        if (found) {
            owns_->ReadValue(buf, &value_, &saved_key_);
            current_ = i;
            break;
        }
    }
    if (!found) {
        error_ = MAI_NOT_FOUND("Seek()");
    }
}
    
Error S1TableReader::IteratorImpl::UpdateEntry() {
    Error rs;
    std::string scatch;
    std::string_view result;
    
    base::intrusive_ptr<core::LRUHandle> handle;
    std::string_view buf;
    rs = owns_->PrepareRead(owns_->index_[slot_][current_], read_opts_, &buf,
                            &handle);
    if (!rs) {
        return rs;
    }
    
    uint64_t shared_len, private_len;
    auto bytes = owns_->ReadKey(buf, &shared_len, &private_len, &result);
    buf.remove_prefix(bytes);
    
    saved_key_ = saved_key_.substr(0, shared_len);
    saved_key_.append(result);
    
    owns_->ReadValue(buf, &value_, &saved_value_);
    return Error::OK();
}
    
////////////////////////////////////////////////////////////////////////////////
/// class S1TableReader::KeyFilterImpl
////////////////////////////////////////////////////////////////////////////////
    
class S1TableReader::KeyFilterImpl final : public core::KeyFilter {
public:
    KeyFilterImpl(const S1TableReader *owns) : owns_(DCHECK_NOTNULL(owns)) {}

    virtual ~KeyFilterImpl() override {}
    
    virtual bool MayExists(std::string_view key) const override { return true; }

    virtual size_t ApproximateCount() const override {
        return owns_->table_props_->num_entries;
    }
    
    virtual size_t memory_usage() const override { return sizeof(*this); }
    
private:
    const S1TableReader *const owns_;
}; // class S1TableReader::KeyFilterImpl
    
////////////////////////////////////////////////////////////////////////////////
/// class S1TableReader
////////////////////////////////////////////////////////////////////////////////
    
S1TableReader::S1TableReader(RandomAccessFile *file, uint64_t file_number,
                             uint64_t file_size, bool checksum_verify,
                             BlockCache *cache)
    : file_(DCHECK_NOTNULL(file))
    , file_number_(file_number)
    , file_size_(file_size)
    , checksum_verify_(checksum_verify)
    , cache_(DCHECK_NOTNULL(cache)) {}

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
    
    table_props_boundle_.reset(new TablePropsBoundle);
    TRY_RUN1(Table::ReadProperties(result, table_props_boundle_->mutable_data()));
    if (!table_props_boundle_->data().unordered) {
        return MAI_CORRUPTION("S1 table can not be ordered.");
    }
    table_props_ = table_props_boundle_->mutable_data();

    TRY_RUN1(ReadBlock({table_props_->index_position, table_props_->index_size },
                       &result, &scatch));
    
    base::BufferReader rd(result);
    DCHECK(!rd.Eof());
    block_map_size_ = rd.ReadVarint64();
    block_map_.reset(new BlockHandle[block_map_size_]);
    for (uint64_t i = 0; i < block_map_size_; ++i) {
        auto offset = rd.ReadVarint64();
        auto size   = rd.ReadVarint64();
        block_map_[i] = BlockHandle(offset, size);
    }
    
    DCHECK(!rd.Eof());
    index_size_ = rd.ReadVarint64();
    index_.reset(new std::vector<Index>[index_size_]);
    for (uint64_t i = 0; i < index_size_; ++i) {
        uint64_t size = rd.ReadVarint64();
        for (uint64_t j = 0; j < size; ++j) {
            Index idx;
            idx.block_idx = rd.ReadVarint64();
            idx.offset       = rd.ReadVarint32();
            index_[i].push_back(idx);
        }
    }
    
    has_initialized_ = true;
    // Filter:
    if (table_props_->filter_size == 0) {
        filter_.reset(new KeyFilterImpl(this));
        return Error::OK();
    }
    TRY_RUN1(ReadBlock({table_props_->filter_position, table_props_->filter_size},
                       &result, &scatch));
    core::KeyFilter *filter
        = new KeyBloomFilter(reinterpret_cast<const uint32_t *>(result.data()),
                             result.size() / 4,
                             base::Hash::kBloomFilterHashs,
                             base::Hash::kNumberBloomFilterHashs);
    filter_.reset(filter);
    return Error::OK();
}

/*virtual*/ Iterator *
S1TableReader::NewIterator(const ReadOptions &read_opts,
                           const core::InternalKeyComparator *ikcmp) {
    if (!has_initialized_) {
        return Iterator::AsError(MAI_CORRUPTION("Not prepare yet."));
    }
    return new IteratorImpl(ikcmp, read_opts, this);
}

/*virtual*/ Error S1TableReader::Get(const ReadOptions &read_opts,
                  const core::InternalKeyComparator *ikcmp,
                  std::string_view key,
                  core::Tag *tag,
                  std::string_view *value,
                  std::string *scratch) {
    if (!has_initialized_) {
        return MAI_CORRUPTION("Not prepare yet.");
    }
    
    ParsedTaggedKey lookup;
    KeyBoundle::ParseTaggedKey(key, &lookup);
    
    size_t slot = ikcmp->Hash(key) % index_size_;
    if (index_[slot].empty()) {
        return MAI_NOT_FOUND("Key not exists in bucket.");
    }
    
    Error rs;
    base::intrusive_ptr<core::LRUHandle> handle;
    std::string_view result;
    std::string saved_key;
    ParsedTaggedKey ikey;
    bool found = false;
    for (const auto &idx : index_[slot]) {
        std::string_view buf;
        rs = PrepareRead(idx, read_opts, &buf, &handle);
        if (!rs) {
            return rs;
        }
        
        uint64_t shared_len, private_len;
        auto bytes = ReadKey(buf, &shared_len, &private_len, &result);
        buf.remove_prefix(bytes);
        
        saved_key = saved_key.substr(0, shared_len);
        saved_key.append(result);
        
        if (table_props_->last_level) {
            if (ikcmp->ucmp()->Equals(saved_key, lookup.user_key)) {
                found = true;
            }
        } else {
            KeyBoundle::ParseTaggedKey(saved_key, &ikey);
            if (ikcmp->ucmp()->Equals(ikey.user_key, lookup.user_key) &&
                lookup.tag.sequence_number() >= ikey.tag.sequence_number()) {
                found = true;
            }
        }
        if (found) {
            ReadValue(buf, value, scratch);
            break;
        }
    }
    if (!found) {
        return MAI_NOT_FOUND("Key not exists.");
    }
    if (!table_props_->last_level) {
        if (tag) {
            *tag = KeyBoundle::ExtractTag(saved_key);
        }
    } else {
        if (tag) {
            *tag = Tag(0, Tag::kFlagValue);
        }
    }
    return Error::OK();
}

/*virtual*/ size_t S1TableReader::ApproximateMemoryUsage() const {
    size_t usage = sizeof(*this);
//    for (const auto &n : index_) {
//        usage += (sizeof(index_[0]) + n.size() * sizeof(Index));
//    }
    // TODO:
    if (!filter_.is_null()) {
        usage += filter_->memory_usage();
    }
    if (!table_props_boundle_.is_null()) {
        usage += sizeof(*table_props_);
    }
    return usage;
}

/*virtual*/
base::intrusive_ptr<TablePropsBoundle>
S1TableReader::GetTableProperties() const { return table_props_boundle_; }

/*virtual*/
base::intrusive_ptr<core::KeyFilter>
S1TableReader::GetKeyFilter() const { return filter_; }
    
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
    
uint64_t S1TableReader::ReadKey(std::string_view buf, uint64_t *shared_len,
                                uint64_t *private_len,
                                std::string_view *result) const {
    base::BufferReader rd(buf);
    *shared_len  = rd.ReadVarint64();
    *private_len = rd.ReadVarint64();
    *result      = rd.ReadString(*private_len);
    return rd.position();
}
    
uint64_t S1TableReader::ReadValue(std::string_view buf, std::string_view *result,
                                  std::string *scatch) const {
    base::BufferReader rd(buf);
    uint64_t value_len = rd.ReadVarint64();
    *scatch = rd.ReadString(value_len);
    *result = *scatch;
    return rd.position();
}
    
Error S1TableReader::PrepareRead(const Index &idx, const ReadOptions &read_opts,
                                 std::string_view *buf,
                                 base::intrusive_ptr<core::LRUHandle> *handle) const {
    DCHECK_GE(idx.block_idx, 0);
    DCHECK_LT(idx.block_idx, block_map_size_);
    
    const auto &bh = block_map_[idx.block_idx];
    Error rs = cache_->GetOrLoad(file_, file_number_, bh.offset(), bh.size(),
                                 read_opts.verify_checksums, handle);
    if (!rs) {
        return rs;
    }

    // Ignore crc32 checksum
    *buf = std::string_view(static_cast<char *>(handle->get()->value) + idx.offset,
                            bh.size() - 4 - idx.offset);
    return Error::OK();
}
    
} // namespace table
    
} // namespace mai
