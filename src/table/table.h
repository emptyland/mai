#ifndef MAI_TABLE_TABLE_H_
#define MAI_TABLE_TABLE_H_

#include "base/base.h"
#include "mai/error.h"
#include <stdint.h>
#include <string>

namespace mai {
class WritableFile;
class RandomAccessFile;
namespace table {
    
struct TableProperties;
    
struct Table final {
    static const uint32_t kHmtMagicNumber;
    static const uint32_t kXmtMagicNumber;
    static const uint32_t kSstMagicNumber;
    
    static Error WriteProperties(const TableProperties &prop, WritableFile *file);
    
    static Error ReadProperties(RandomAccessFile *file, uint64_t *position,
                                TableProperties *props);
    
    DISALLOW_ALL_CONSTRUCTORS(Table);
}; // struct Table
    
// unordered?
// last-level?
// block-size
// num-entries
// index-position
// index-count
// last-version
// smallest-key
// largest-key
struct TableProperties final {
    bool        unordered;
    bool        last_level;
    uint32_t    block_size;
    size_t      num_entries;
    uint64_t    index_position;
    size_t      index_count;
    uint64_t    filter_position;
    size_t      filter_size;
    uint64_t    last_version;
    std::string smallest_key;
    std::string largest_key;
}; // struct FileProperties
    

class BlockHandle {
public:
    BlockHandle() : BlockHandle(0, 0) {}
    
    BlockHandle(uint64_t offset, uint64_t size)
        : offset_(offset)
        , size_(size) {}
    
    DEF_VAL_GETTER(uint64_t, offset);
    DEF_VAL_GETTER(uint64_t, size);
    
    bool empty() const { return offset_ == 0 && size_ == 0; }
    
    void Encode(std::string *buf) const;
    
private:
    uint64_t offset_;
    uint64_t size_;
}; // class BlockHandle
    
} // namespace table
    
} // namespace mai


#endif // MAI_TABLE_TABLE_H_
