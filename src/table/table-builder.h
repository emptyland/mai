#ifndef MAI_TABLE_TABLE_BUILDER_H_
#define MAI_TABLE_TABLE_BUILDER_H_

#include "mai/error.h"
#include "base/base.h"
#include <string_view>

namespace mai {
    
namespace table {

class TableBuilder {
public:
    TableBuilder() {}
    virtual ~TableBuilder() {} // TODO:
    
    virtual void Add(std::string_view key, std::string_view value) = 0;
    
    virtual Error error() = 0;
    
    virtual Error Finish() = 0;
    
    virtual void Abandon() = 0;
    
    virtual uint64_t NumEntries() const = 0;
    
    virtual uint64_t FileSize() const = 0;
    
}; // class TableBuilder
    
} // namespace table
    
} // namespace mai


#endif // MAI_TABLE_TABLE_BUILDER_H_
