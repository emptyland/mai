#ifndef MAI_ALLOCATOR_H_
#define MAI_ALLOCATOR_H_

#include <stddef.h>

namespace mai {
    
class Allocator {
public:
    Allocator() {}
    virtual ~Allocator() {}
    
    virtual void *Allocate(size_t size) = 0;
    
    virtual void Free(const void *chunk) = 0;
} // class Allocator
    
} // namespace mai

#endif // MAI_ALLOCATOR_H_
