#ifndef MAI_TABLE_TABLE_READER_H_
#define MAI_TABLE_TABLE_READER_H_

#include "reference-count.h"
#include "base/base.h"
#include "mai/error.h"
#include <string_view>
#include <memory>

namespace mai {
    
struct ReadOptions;
class Comparator;
class Iterator;

namespace core {
class Tag;
class InternalKeyComparator;
class KeyFilter;
} // namespace core
    
namespace table {
    
struct TableProperties;
class  TablePropsBoundle;

class TableReader {
public:
    TableReader() {}
    virtual ~TableReader() {}
    
    virtual Iterator *NewIterator(const ReadOptions &read_opts,
                                  const core::InternalKeyComparator *ikcmp) = 0;

    // Prepare for Get()
    virtual void Prepare(std::string_view /*target*/) {}
    
    virtual Error Get(const ReadOptions &read_opts,
                      const core::InternalKeyComparator *ikcmp,
                      std::string_view key,
                      core::Tag *tag,
                      std::string_view *value,
                      std::string *scratch) = 0;
    
    virtual size_t ApproximateMemoryUsage() const = 0;
    
    virtual base::intrusive_ptr<TablePropsBoundle> GetTableProperties() const = 0;
    
    virtual base::intrusive_ptr<core::KeyFilter> GetKeyFilter() const = 0;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(TableReader);
}; // class TableReader

} // namespace table
    
} // namespace mai

#endif //  MAI_TABLE_TABLE_READER_H_
