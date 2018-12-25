#include "mai/write-batch.h"
#include "core/key-boundle.h"
#include "base/slice.h"
#include "mai/db.h"

namespace mai {
    
using ::mai::base::Varint32;
using ::mai::base::Varint64;
using ::mai::base::Slice;
using ::mai::base::ScopedMemory;
using ::mai::base::BufferReader;
using ::mai::core::Tag;
using ::mai::core::KeyBoundle;
    
const size_t WriteBatch::kHeaderSize = sizeof(uint64_t) + sizeof(uint32_t);
    
WriteBatch::~WriteBatch() {}
    
void WriteBatch::Put(ColumnFamily *cf, std::string_view key, std::string_view value) {
    KeyBoundle::MakeRedo(key, value, cf->id(), Tag::kFlagValue, &redo_);
    ++n_entries_;
}

void WriteBatch::Delete(ColumnFamily *cf, std::string_view key) {
    KeyBoundle::MakeRedo(key, "", cf->id(), Tag::kFlagDeletion, &redo_);
    ++n_entries_;
}

/*static*/ Error WriteBatch::Iterate(const char *buf, size_t len, Stub *handler) {
    if (len == 0) {
        return Error::OK();
    }
//    const char *p = buf, *const end = p + len;
//    while (p < end) {
//        size_t varint_len;
//        uint32_t cfid = Varint32::Decode(p, &varint_len);
//        p += varint_len;
//
//        Tag::Flag flag = static_cast<Tag::Flag>(*p++);
//
//        size_t key_size = Varint64::Decode(p, &varint_len);
//        p += varint_len;
//        std::string_view key(p, key_size);
//        p += key_size;
//
//        size_t value_size = 0;
//        std::string_view value;
//        switch (flag) {
//            case Tag::kFlagValue:
//                value_size = Varint64::Decode(p, &varint_len);
//                p += varint_len;
//                value = std::string_view(p, value_size);
//                p += value_size;
//                handler->Put(cfid, key, value);
//                break;
//
//            case Tag::kFlagDeletion:
//                handler->Delete(cfid, key);
//                break;
//
//            default:
//                DLOG(FATAL) << "Noreached!";
//                break;
//        }
//    }
    
    BufferReader rd(std::string_view(buf, len));
    while (!rd.Eof()) {
        uint32_t cfid = rd.ReadVarint32();
        Tag::Flag flag = static_cast<Tag::Flag>(rd.ReadByte());
        std::string_view key = rd.ReadString();
        switch (flag) {
            case Tag::kFlagValue:
                handler->Put(cfid, key, rd.ReadString());
                break;

            case Tag::kFlagDeletion:
                handler->Delete(cfid, key);
                break;

            default:
                DLOG(FATAL) << "Noreached!";
                break;
        }
    }
    return Error::OK();
}
    
} // namespace mai
