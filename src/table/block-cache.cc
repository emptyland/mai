#include "table/block-cache.h"
#include "base/slice.h"
#include "base/hash.h"
#include "mai/env.h"

namespace mai {
    
namespace table {

static const size_t kKeySize = sizeof(uintptr_t) + sizeof(uint64_t);
    
using Args = std::tuple<RandomAccessFile *, uint64_t, uint64_t, bool>;
    
BlockCache::BlockCache(Allocator *ll_allocator, size_t capacity)
    : cache_(7, ll_allocator, capacity) {
}

BlockCache::~BlockCache() {
}

Error BlockCache::GetOrLoad(RandomAccessFile *file, uint64_t offset,
                            uint64_t size,
                            bool checksum_verify,
                            base::intrusive_ptr<core::LRUHandle> *result) {
    char key[kKeySize];
    ::memcpy(key, &file, sizeof(file));
    ::memcpy(key + sizeof(uintptr_t), &offset, sizeof(offset));
    
    auto idx = GetShardIdx(file);
    auto shard = cache_.GetShard(idx);
    auto args = std::make_tuple(file, offset, size, checksum_verify);
    return shard->GetOrLoad(std::string_view(key, kKeySize), result, &Deleter,
                            &Loader, this, &args);
}
    
/*static*/ void BlockCache::Deleter(std::string_view, void *) {
}

/*static*/ Error BlockCache::Loader(std::string_view key,
                                    core::LRUHandle **result, void *arg0,
                                    void *arg1) {
    RandomAccessFile *file;
    uint64_t offset, size;
    bool checksum_verify;
    
    std::tie(file, offset, size, checksum_verify) = *static_cast<Args *>(arg1);
    
    std::string_view buf;
    std::string scratch;
    Error rs = file->Read(offset, size, &buf, &scratch);
    if (!rs) {
        return rs;
    }
    
    if (checksum_verify) {
        auto checksum = base::Slice::SetU32(buf.substr(0, 4));
        if (checksum != base::Hash::Crc32(buf.data() + 4, buf.size() - 4)) {
            return MAI_IO_ERROR("Checksum fail!");
        }
    }
    
    // TODO: Uncompress
    
    auto handle = core::LRUHandle::New(key, buf.size() - 4);
    ::memcpy(handle->value, buf.data() + 4, buf.size() - 4);
    
    *result = handle;
    return Error::OK();
}
    
} // namespace table
    
} // namespace mai
