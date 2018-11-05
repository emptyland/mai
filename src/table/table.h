#ifndef MAI_TABLE_TABLE_H_
#define MAI_TABLE_TABLE_H_

#include "mai/error.h"
#include <stdint.h>
#include <string>

namespace mai {
class WritableFile;
class RandomAccessFile;
namespace table {
    
struct TableProperties;
    
struct Table final {
    static const int kHmtMagicNumber;
    static const int kXmtMagicNumber;
    
    static Error WriteProperties(const TableProperties &prop, WritableFile *file);
    
    static Error ReadProperties(RandomAccessFile *file, uint64_t *position,
                                TableProperties *props);
    
    
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
    uint64_t    last_version;
    std::string smallest_key;
    std::string largest_key;
}; // struct FileProperties
    
} // namespace table
    
} // namespace mai


#endif // MAI_TABLE_TABLE_H_
