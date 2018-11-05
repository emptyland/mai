#ifndef MAI_TABLE_TABLE_H_
#define MAI_TABLE_TABLE_H_

#include <stdint.h>
#include <string>

namespace mai {
    
namespace table {
    
struct Table final {
    static const int kHmtMagicNumber = 0x746d6800;
    static const int kXmtMagicNumber = 0x746d7800;
}; // struct Table
    
// ordered/unordered
// block-size
// num-entries
// index-position
// smallest-key
// largest-key
struct TableProperties final {
    bool        unordered;
    uint32_t    block_size;
    size_t      num_entries;
    uint64_t    index_position;
    std::string smallest_key;
    std::string largest_key;
}; // struct FileProperties
    
} // namespace table
    
} // namespace mai


#endif // MAI_TABLE_TABLE_H_
