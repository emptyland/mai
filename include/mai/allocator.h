#ifndef MAI_ALLOCATOR_H_
#define MAI_ALLOCATOR_H_

#include "mai/error.h"
//#include <stdint.h>
#include <stddef.h>

namespace mai {
    
class Allocator {
public:
    enum Access : int {
        kWr = 1,
        kRd = 1 << 1,
        kEx = 1 << 2,
    };
    
    Allocator() {}
    virtual ~Allocator() {}
    
    virtual void *Allocate(size_t size, size_t alignment = sizeof(max_align_t)) = 0;
    
    virtual void Free(const void *chunk, size_t size = 0) = 0;
    
    virtual Error SetAccess(void *chunk, size_t size, int flags) { return Error::OK(); }
    
    //virtual uint32_t GetAccess() { return 0; }
    
    virtual size_t granularity() = 0;
}; // class Allocator
    
} // namespace mai

#endif // MAI_ALLOCATOR_H_
