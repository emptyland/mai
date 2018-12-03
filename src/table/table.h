#ifndef MAI_TABLE_TABLE_H_
#define MAI_TABLE_TABLE_H_

#include "base/reference-count.h"
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
    static const uint32_t kS1tMagicNumber;
    
    static Error WriteProperties(const TableProperties &prop, WritableFile *file);
    
    static void WriteProperties(const TableProperties &prop, std::string *buf);
    
    static Error ReadProperties(RandomAccessFile *file, uint64_t position,
                                uint64_t size, TableProperties *props);
    
    static Error ReadProperties(std::string_view buf, TableProperties *props);
    
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
    bool        unordered       = false;
    bool        last_level      = false;
    uint32_t    block_size      = 0;
    size_t      num_entries     = 0;
    uint64_t    index_position  = 0;
    size_t      index_count     = 0;
    uint64_t    filter_position = 0;
    size_t      filter_size     = 0;
    uint64_t    last_version    = 0;
    std::string smallest_key;
    std::string largest_key;
}; // struct FileProperties
    

class TablePropsBoundle final
    : public base::ReferenceCounted<TablePropsBoundle> {
public:
    TablePropsBoundle() {}
        
    DEF_VAL_GETTER(TableProperties, data);
    DEF_VAL_MUTABLE_GETTER(TableProperties, data);

    DISALLOW_IMPLICIT_CONSTRUCTORS(TablePropsBoundle);
private:
    TableProperties data_;
}; // class TablePropsBoundle
    

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
    void Decode(std::string_view buf);
    
private:
    uint64_t offset_;
    uint64_t size_;
}; // class BlockHandle
    
} // namespace table
    
} // namespace mai


#endif // MAI_TABLE_TABLE_H_
