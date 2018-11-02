#ifndef MAI_TABLE_TABLE_READER_H_
#define MAI_TABLE_TABLE_READER_H_

#include "base/base.h"
#include "mai/error.h"
#include <string_view>

namespace mai {
    
struct ReadOptions;
class Comparator;
    
namespace core {
class InternalIterator;
class Tag;
} // namespace core
    
namespace table {

class TableReader {
public:
    TableReader() {}
    virtual ~TableReader() {}
    
    virtual core::InternalIterator *NewIterator(const ReadOptions &read_opts,
                                                const Comparator *ucmp) = 0;
    
    // Prepare for Get()
    virtual void Prepare(std::string_view /*target*/) {}
    
    virtual Error Get(const ReadOptions &read_opts,
                      const Comparator *ucmp,
                      std::string_view key,
                      core::Tag *tag,
                      std::string_view *value,
                      std::string *scratch) = 0;
    
    virtual size_t ApproximateMemoryUsage() const = 0;
}; // class TableReader

} // namespace table
    
} // namespace mai

#endif //  MAI_TABLE_TABLE_READER_H_
