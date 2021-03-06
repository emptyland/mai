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
    buf->append(Slice::GetU32(static_cast<uint32_t>(props.index_size),
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
    TRY_RUN(props->index_size      = reader.ReadFixed32());
    TRY_RUN(props->filter_position = reader.ReadFixed64());
    TRY_RUN(props->filter_size     = reader.ReadFixed32());
    TRY_RUN(props->last_version    = reader.ReadFixed64());
    TRY_RUN(props->smallest_key    = reader.ReadString());
            props->largest_key     = reader.ReadString();
    return Error::OK();
}
    
#undef TRY_RUN
    
/*static*/ Error Table::ReadProperties(RandomAccessFile *file, uint64_t position,
                                       uint64_t size, TableProperties *props) {
    std::string_view result;
    std::string scratch;
    
    Error rs = file->Read(position, size, &result, &scratch);
    if (!rs) {
        return rs;
    }
    return ReadProperties(result, props);
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
