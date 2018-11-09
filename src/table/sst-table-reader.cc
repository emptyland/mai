#include "table/sst-table-reader.h"
#include "table/key-bloom-filter.h"
#include "base/slice.h"
#include "mai/env.h"
#include "glog/logging.h"

namespace mai {

namespace table {
    
SstTableReader::SstTableReader(RandomAccessFile *file, uint64_t file_size,
                               bool checksum_verify)
    : file_(DCHECK_NOTNULL(file))
    , file_size_(file_size)
    , checksum_verify_(checksum_verify) {}
    
/*virtual*/ SstTableReader::~SstTableReader() {}

Error SstTableReader::Prepare() {
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
    if (Slice::SetU32(result) != Table::kSstMagicNumber) {
        return MAI_CORRUPTION("Incorrect file type, required: sst");
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
    
    // Indexs:
    rs = file_->Read(table_props_->index_position, 4, &result, &scratch);
    if (!rs) {
        return rs;
    }
    const uint32_t index_checksum = Slice::SetU32(result);
    
    uint32_t checksum = 0;
    int64_t count = table_props_->index_count;
    position = table_props_->index_position + 4;
    while (count--) {
        rs = file_->Read(position, Varint64::kMaxLen, &result, &scratch);
        if (!rs) {
            return rs;
        }
        size_t varint_len;
        uint64_t offset = Varint64::Decode(result.data(), &varint_len);
        position += varint_len;
        checksum = ::crc32(checksum, result.data(), varint_len);
        
        rs = file_->Read(position, Varint64::kMaxLen, &result, &scratch);
        if (!rs) {
            return rs;
        }
        uint64_t size = Varint64::Decode(result.data(), &varint_len);
        position += varint_len;
        checksum = ::crc32(checksum, result.data(), varint_len);
        
        indexs_.push_back(BlockHandle{offset, size});
    }
    
    if (checksum_verify_ && index_checksum != checksum) {
        return MAI_IO_ERROR("Incorrect index block checksum!");
    }
    
    // Filter:
    rs = file_->Read(table_props_->filter_position, 4, &result, &scratch);
    if (!rs) {
        return rs;
    }
    const uint32_t filter_checksum = Slice::SetU32(result);
    rs = file_->Read(table_props_->filter_position + 4,
                     table_props_->filter_size - 4, &result, &scratch);
    if (!rs) {
        return rs;
    }
    if (checksum_verify_) {
        checksum = ::crc32(0, result.data(), result.size());
        if (filter_checksum != checksum) {
            return MAI_IO_ERROR("Incorrect filter block checksum!");
        }
    }
    core::KeyFilter *filter
        = new KeyBloomFilter(reinterpret_cast<const uint32_t *>(result.data()),
                             result.size() / 4,
                             base::Hash::kBloomFilterHashs,
                             base::Hash::kNumberBloomFilterHashs);
    bloom_filter_.reset(filter);
    return Error::OK();
}

/*virtual*/ Iterator *
SstTableReader::NewIterator(const ReadOptions &read_opts,
            const core::InternalKeyComparator *ikcmp) {
    // TODO:
    return nullptr;
}

/*virtual*/ Error SstTableReader::Get(const ReadOptions &read_opts,
                  const core::InternalKeyComparator *ikcmp,
                  std::string_view key,
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
    
    // TODO:
    return MAI_NOT_SUPPORTED("TODO:");
}

/*virtual*/ size_t SstTableReader::ApproximateMemoryUsage() const {
    size_t usage = sizeof(*this);
    usage += (table_props_ ? sizeof(TableProperties) : 0);
    usage += (bloom_filter_ ? bloom_filter_->memory_usage() : 0);
    // TODO:
    return usage;
}

/*virtual*/ std::shared_ptr<TableProperties>
SstTableReader::GetTableProperties() const { return table_props_; }

/*virtual*/ std::shared_ptr<core::KeyFilter>
SstTableReader::GetKeyFilter() const { return bloom_filter_; }
    
} // namespace table

} // namespace mai
