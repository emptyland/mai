#include "table/block-cache.h"
#include "base/slice.h"
#include "base/hash.h"
#include "mai/env.h"
#include <thread>

namespace mai {
    
namespace table {

static const size_t kKeySize = sizeof(uint64_t) + sizeof(uint64_t);
    
using Args = std::tuple<RandomAccessFile *, uint64_t, uint64_t, bool>;
    
BlockCache::BlockCache(Allocator *ll_allocator, size_t capacity)
    : cache_(7, ll_allocator, capacity) {
}

BlockCache::~BlockCache() {
}

Error BlockCache::GetOrLoad(RandomAccessFile *file,
                            uint64_t file_number,
                            uint64_t offset,
                            uint64_t size,
                            bool checksum_verify,
                            base::intrusive_ptr<core::LRUHandle> *result) {
    char key[kKeySize];
    ::memcpy(key, &file_number, sizeof(file_number));
    ::memcpy(key + sizeof(file_number), &offset, sizeof(offset));
    
    auto shard = GetShard(file_number);
    auto args = std::make_tuple(file, offset, size, checksum_verify);
    return shard->GetOrLoad(std::string_view(key, kKeySize), result, nullptr,
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
    
size_t BlockCache::GetShardIdx(uint64_t file_number) {
    //std::this_thread::get_id()
    return cache_.HashNumber(file_number);
}
    
} // namespace table
    
} // namespace mai
