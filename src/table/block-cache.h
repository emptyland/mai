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
    
    Error GetOrLoad(RandomAccessFile *file, uint64_t offset, uint64_t size,
                    bool checksum_verify,
                    base::intrusive_ptr<core::LRUHandle> *result);
    
    void Purge(const RandomAccessFile *file) {
        cache_.Purge(GetShardIdx(file));
    }

    DISALLOW_IMPLICIT_CONSTRUCTORS(BlockCache);
private:
    size_t GetShardIdx(const RandomAccessFile *file) const {
        return cache_.HashNumber((reinterpret_cast<uintptr_t>(file)) >> 2 | 1);
    }
    
    static void Deleter(std::string_view, void *);
    static Error Loader(std::string_view, core::LRUHandle **, void *, void *);
    
    core::LRUCache cache_;
}; // class BlockCache

    
} // namespace table
    
} // namespace mai


#endif // MAI_TABLE_BLOCK_CACHE_H_
