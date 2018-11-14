#include "mai/write-batch.h"
#include "core/key-boundle.h"
#include "base/slice.h"
#include "mai/db.h"

namespace mai {
    
using ::mai::base::Varint32;
using ::mai::base::Varint64;
using ::mai::base::Slice;
using ::mai::base::ScopedMemory;
using ::mai::core::Tag;
using ::mai::core::KeyBoundle;
    
WriteBatch::~WriteBatch() {}
    
void WriteBatch::Put(ColumnFamily *cf, std::string_view key, std::string_view value) {
    ScopedMemory scope;
    
    redo_.append(1, Tag::kFlagValue);
    redo_.append(Slice::GetU32(cf->id(), &scope));
    
    redo_.append(Slice::GetU64(key.size(), &scope));
    redo_.append(key);
    
    redo_.append(Slice::GetU64(value.size(), &scope));
    redo_.append(value);
}

void WriteBatch::Delete(ColumnFamily *cf, std::string_view key) {
    ScopedMemory scope;
    
    redo_.append(1, Tag::kFlagDeletion);
    redo_.append(Slice::GetU32(cf->id(), &scope));
    
    redo_.append(Slice::GetU64(key.size(), &scope));
    redo_.append(key);
}

/*static*/ Error WriteBatch::Iterate(const char *buf, size_t len, Stub *handler) {
    if (len == 0) {
        return Error::OK();
    }
    const char *p = buf, *const end = p + len;
    while (p < end) {
        Tag::Flag flag = static_cast<Tag::Flag>(*p++);
        uint32_t  cfid = *reinterpret_cast<const uint32_t *>(p);
        p += 4;
        
        size_t varint_len;
        size_t key_size = Varint64::Decode(p, &varint_len);
        p += varint_len;
        std::string_view key(p, key_size);
        p += key_size;
        
        size_t value_size = 0;
        std::string_view value;
        switch (flag) {
            case Tag::kFlagValue:
                value_size = Varint64::Decode(p, &varint_len);
                p += varint_len;
                value = std::string_view(p, value_size);
                p += varint_len;
                handler->Put(cfid, key, value);
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
