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
                NOREACHED();
                break;
        }
    }
    return Error::OK();
}
    
} // namespace mai
