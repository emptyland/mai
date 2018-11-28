#include "table/table.h"
#include "base/slice.h"
#include "base/varint-encoding.h"
#include "mai/env.h"

namespace mai {
    
namespace table {
    
/*static*/ const uint32_t Table::kHmtMagicNumber = 0x746d6800;
/*static*/ const uint32_t Table::kXmtMagicNumber = 0x746d7800;
/*static*/ const uint32_t Table::kSstMagicNumber = 0x74737300;
/*static*/ const uint32_t Table::kS1tMagicNumber = 0x74317300;
    
/*static*/
Error Table::WriteProperties(const TableProperties &prop, WritableFile *file) {
    std::string buf;
    WriteProperties(prop, &buf);
    return file->Append(buf);
}
    
/*static*/
void Table::WriteProperties(const TableProperties &props, std::string *buf) {
    using base::Slice;

    const char unordered = props.unordered ? 1 : 0;
    buf->append(std::string_view(&unordered, 1));
    const char last_level = props.last_level ? 1 : 0;
    buf->append(std::string_view(&last_level, 1));
    
    base::ScopedMemory scope;
    buf->append(Slice::GetU32(props.block_size, &scope));
    buf->append(Slice::GetU32(static_cast<uint32_t>(props.num_entries),
                                    &scope));
    buf->append(Slice::GetU64(props.index_position, &scope));
    buf->append(Slice::GetU32(static_cast<uint32_t>(props.index_count),
                                    &scope));
    buf->append(Slice::GetU64(props.filter_position, &scope));
    buf->append(Slice::GetU32(static_cast<uint32_t>(props.filter_size),
                                    &scope));
    buf->append(Slice::GetU64(props.last_version, &scope));
    buf->append(Slice::GetV64(props.smallest_key.size(), &scope));
    buf->append(props.smallest_key);
    buf->append(Slice::GetV64(props.largest_key.size(), &scope));
    buf->append(props.largest_key);
}

#define TRY_RUN(expr) \
    (expr); \
    if (reader.Eof()) { \
        return MAI_IO_ERROR("Incomplete properties data."); \
    } (void)0

/*static*/ Error Table::ReadProperties(std::string_view buf,
                                       TableProperties *props) {
    base::BufferReader reader(buf);
    TRY_RUN(props->unordered       = reader.ReadByte() ? true : false);
    TRY_RUN(props->last_level      = reader.ReadByte() ? true : false);
    TRY_RUN(props->block_size      = reader.ReadFixed32());
    TRY_RUN(props->num_entries     = reader.ReadFixed32());
    TRY_RUN(props->index_position  = reader.ReadFixed64());
    TRY_RUN(props->index_count     = reader.ReadFixed32());
    TRY_RUN(props->filter_position = reader.ReadFixed64());
    TRY_RUN(props->filter_size     = reader.ReadFixed32());
    TRY_RUN(props->last_version    = reader.ReadFixed64());
    TRY_RUN(props->smallest_key    = reader.ReadString());
            props->largest_key     = reader.ReadString();
    return Error::OK();
}
    
#undef TRY_RUN
    
/*static*/ Error Table::ReadProperties(RandomAccessFile *file, uint64_t *position,
                                       TableProperties *props) {
    using base::Slice;
    using base::Varint32;
    using base::Varint64;
    
    std::string_view result;
    std::string scratch;
    
    Error rs = file->Read(*position, 1, &result, &scratch);
    if (!rs) {
        return rs;
    }
    (*position) += result.size();
    props->unordered = result[0];
    
    rs = file->Read(*position, 1, &result, &scratch);
    if (!rs) {
        return rs;
    }
    (*position) += result.size();
    props->last_level = result[0];
    
    rs = file->Read(*position, 4, &result, &scratch);
    if (!rs) {
        return rs;
    }
    (*position) += result.size();
    props->block_size = Slice::SetU32(result);
    
    rs = file->Read(*position, 4, &result, &scratch);
    if (!rs) {
        return rs;
    }
    (*position) += result.size();
    props->num_entries = Slice::SetU32(result);
    
    rs = file->Read(*position, 8, &result, &scratch);
    if (!rs) {
        return rs;
    }
    (*position) += result.size();
    props->index_position = Slice::SetU64(result);
    
    rs = file->Read(*position, 4, &result, &scratch);
    if (!rs) {
        return rs;
    }
    (*position) += result.size();
    props->index_count = Slice::SetU32(result);
    
    rs = file->Read(*position, 8, &result, &scratch);
    if (!rs) {
        return rs;
    }
    (*position) += result.size();
    props->filter_position = Slice::SetU64(result);
    
    rs = file->Read(*position, 4, &result, &scratch);
    if (!rs) {
        return rs;
    }
    (*position) += result.size();
    props->filter_size = Slice::SetU32(result);
    
    rs = file->Read(*position, 8, &result, &scratch);
    if (!rs) {
        return rs;
    }
    (*position) += result.size();
    props->last_version = Slice::SetU64(result);
    
    rs = file->Read(*position, Varint64::kMaxLen, &result, &scratch);
    if (!rs) {
        return rs;
    }
    size_t varint_len = 0;
    uint64_t key_size = Varint64::Decode(result.data(), &varint_len);
    (*position) += varint_len;
    rs = file->Read(*position, key_size, &result, &scratch);
    if (!rs) {
        return rs;
    }
    (*position) += result.size();
    props->smallest_key = result;
    
    rs = file->Read(*position, Varint64::kMaxLen, &result, &scratch);
    if (!rs) {
        return rs;
    }
    varint_len = 0;
    key_size = Varint64::Decode(result.data(), &varint_len);
    (*position) += varint_len;
    rs = file->Read(*position, key_size, &result, &scratch);
    if (!rs) {
        return rs;
    }
    (*position) += result.size();
    props->largest_key = result;
    
    return Error::OK();
}
    
void BlockHandle::Encode(std::string *buf) const {
    using ::mai::base::Slice;
    using ::mai::base::ScopedMemory;
    
    ScopedMemory scope;
    buf->append(Slice::GetV64(offset_, &scope));
    buf->append(Slice::GetV64(size_, &scope));
}
    
void BlockHandle::Decode(std::string_view buf) {
    using ::mai::base::Slice;
    using ::mai::base::Varint32;
    using ::mai::base::Varint64;
    
    size_t varint_len;
    offset_ = Varint64::Decode(buf.data(), &varint_len);
    buf.remove_prefix(varint_len);
    size_   = Varint64::Decode(buf.data(), &varint_len);
}
    
} // namespace table
    
} // namespace mai
