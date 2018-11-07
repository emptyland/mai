#ifndef MAI_CORE_MEMORY_TABLE_H_
#define MAI_CORE_MEMORY_TABLE_H_

#include "core/key-boundle.h"
#include "base/reference-count.h"
#include "base/allocators.h"
#include "mai/error.h"
#include <string_view>

namespace mai {
class Iterator;
namespace core {
    
class MemoryTable : public base::ReferenceCountable {
public:
    MemoryTable() {}
    virtual ~MemoryTable() {}
    
    virtual void Put(std::string_view key, std::string_view value,
                     Version version, uint8_t flag) = 0;

    virtual Error Get(std::string_view key, Version version, Tag *tag,
                      std::string *value) const = 0;
    
    virtual Iterator *NewIterator() = 0;
    
    virtual size_t ApproximateMemoryUsage() const = 0;

    DISALLOW_IMPLICIT_CONSTRUCTORS(MemoryTable);
}; // class MemoryTable
    
} // namespace core
    
} // namespace mai


#endif // MAI_CORE_MEMORY_TABLE_H_
