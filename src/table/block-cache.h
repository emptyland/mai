#ifndef MAI_TABLE_BLOCK_CACHE_H_
#define MAI_TABLE_BLOCK_CACHE_H_

#include "core/lru-cache-v1.h"
#include "base/reference-count.h"
#include "base/base.h"
#include <memory>

namespace mai {
class RandomAccessFile;
namespace table {

class BlockCache final {
public:
    BlockCache(Allocator *ll_allocator, size_t capacity);
    ~BlockCache();
    
    Error GetOrLoad(RandomAccessFile *file,
                    uint64_t file_number,
                    uint64_t offset,
                    uint64_t size,
                    bool checksum_verify,
                    base::intrusive_ptr<core::LRUHandle> *result);
    
    void Purge(uint64_t file_number) {
        cache_.Purge(GetShardIdx(file_number));
    }
    
    core::LRUCacheShard *GetShard(uint64_t file_number) {
        return cache_.GetShard(GetShardIdx(file_number));
    }
    
    size_t GetShardIdx(uint64_t file_number);

    DISALLOW_IMPLICIT_CONSTRUCTORS(BlockCache);
private:
    static void Deleter(std::string_view, void *);
    static Error Loader(std::string_view, core::LRUHandle **, void *, void *);
    
    core::LRUCache cache_;
}; // class BlockCache

    
} // namespace table
    
} // namespace mai


#endif // MAI_TABLE_BLOCK_CACHE_H_
