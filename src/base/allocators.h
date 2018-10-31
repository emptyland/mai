#ifndef MAI_BASE_ALLOCATORS_H_
#define MAI_BASE_ALLOCATORS_H_

#include "glog/logging.h"
#include <stdlib.h>
#include <memory>

namespace mai {
    
namespace base {
    
struct MallocAllocator {
    void *Allocate(size_t size) const { return ::malloc(size); }
    void Free(void *chunk) const { ::free(chunk); }
}; // struct MallocAllocator
    
struct NewAllocator {
    void *Allocate(size_t size) const { return new char[size]; }
    void Free(void *chunk) const { delete[] static_cast<char *>(chunk); }
}; // struct NewAllocator

class ScopedMemory {
public:
    static const int kSize = 512;
    
    ScopedMemory() {}
    ~ScopedMemory() { Purge(); }
    
    void *New(size_t size) {
        Purge();
        if (size > kSize) {
            memory_ = ::malloc(size);
        }
        return memory_;
    }
    
    void Purge() {
        if (memory_ != init_memory_) {
            ::free(memory_);
            memory_ = init_memory_;
        }
    }
    
private:
    char init_memory_[kSize];
    void *memory_ = init_memory_;
}; // class ScopedMemory
    
struct ScopedAllocator {
    void *Allocate(size_t size) { return memory->New(size); }
  
    ScopedMemory *memory;
}; // struct ScopedAllocator
    
} // namespace base
    
} // namespace mai

#endif // MAI_BASE_ALLOCATORS_H_
