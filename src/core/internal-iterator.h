#ifndef MAI_CORE_INTERNAL_ITERATOR_H_
#define MAI_CORE_INTERNAL_ITERATOR_H_

#include "base/base.h"
#include "mai/error.h"
#include <string_view>

namespace mai {
    
namespace core {
    
class InternalIterator {
public:
    InternalIterator() {}
    virtual ~InternalIterator();
    
    virtual bool Valid() const = 0;
    virtual void SeekToFirst() = 0;
    virtual void SeekToLast() = 0;
    virtual void Seek(std::string_view target) = 0;
    
    virtual void Next() = 0;
    virtual void Prev() = 0;
    
    virtual std::string_view key() const = 0;
    virtual std::string_view value() const = 0;
    
    virtual Error error() const = 0;
    
    static InternalIterator *AsError(Error error);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(InternalIterator);
}; // class InternalIterator
    
} // namespace core
    
} // namespace mai


#endif // MAI_CORE_INTERNAL_ITERATOR_H_
