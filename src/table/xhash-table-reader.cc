#include "table/xhash-table-reader.h"
#include "table/table.h"
#include "base/slice.h"
#include "mai/env.h"

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
/*virtual*/ Error XhashTableReader::Get(const ReadOptions &read_opts,
                  const Comparator *cmp,
                  std::string_view key,
                  core::Tag *tag,
                  std::string_view *value,
                  std::string *scratch) {
    // TODO:
    return MAI_NOT_SUPPORTED("TODO:");
}
    
/*virtual*/ size_t XhashTableReader::ApproximateMemoryUsage() const {
    // TODO:
    return 0;
}
    
/*virtual*/ std::shared_ptr<TableProperties>
XhashTableReader::GetTableProperties() const { return table_props_; }
    
} // namespace table
    
} // namespace mai
