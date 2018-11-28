#include "table/s1-table-reader.h"
#include "table/table.h"
#include "core/key-boundle.h"
#include "core/internal-key-comparator.h"
#include "base/io-utils.h"
#include "base/slice.h"
#include "base/hash.h"
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
    return nullptr;
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
    base::RandomAccessFileReader reader(file_);
    
    bool found = false;
    std::string saved_key;
    for (const auto &idx : index_[slot]) {
        uint64_t offset = idx.block_offset + idx.offset + 4;
        size_t len = 0;
        
        uint64_t shared_len, private_len;
        TRY_RUN0(shared_len = reader.ReadVarint64(offset, &len));
        offset += len;
        TRY_RUN0(private_len = reader.ReadVarint64(offset, &len));
        offset += len;
        
        std::string_view result = reader.Read(offset, private_len);
        offset += result.size();

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
            uint64_t value_len;
            TRY_RUN0(value_len = reader.ReadVarint64(offset, &len));
            offset += len;
            *value = reader.Read(offset, value_len, scratch);
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
                               std::string *scatch) {
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
    
} // namespace table
    
} // namespace mai
