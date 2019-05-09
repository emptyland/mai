#ifndef MAI_BASE_ALLOCATORS_H_
#define MAI_BASE_ALLOCATORS_H_

#include "mai/allocator.h"
#include "glog/logging.h"
#include <stdlib.h>
#include <memory>
#include <type_traits>

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
    
struct DelegatedAllocator {
    Allocator *allocator;
    void *Allocate(size_t size) const { return allocator->Allocate(size, 4); }
    void Free(void *chunk) const { allocator->Free(chunk, 0); }
}; // struct DelegatedAllocator

template<class T, int N>
class ScopedMemoryTemplate {
public:
    static const int kSize = N;
    
    ScopedMemoryTemplate() {}
    ~ScopedMemoryTemplate() { Purge(); }
    
    void *Allocate(size_t size) {
        Purge();
        if (size > kSize) {
            memory_ = static_cast<T *>(::malloc(size * sizeof(T)));
        }
        return memory_;
    }
    
    T *New(size_t size) {
        T *raw = static_cast<T *>(Allocate(size));
        if (std::is_constructible<T, void>::value) {
            for (size_t i = 0; i < size; ++i) {
                new (raw) T();
            }
        }
        return raw;
    }
    
    void Purge() {
        if (memory_ != static_) {
            ::free(memory_);
            memory_ = static_;
        }
    }
    
private:
    T  static_[kSize];
    T *memory_ = static_;
}; // class ScopedMemory
    
using ScopedMemory = ScopedMemoryTemplate<uint8_t, 512>;
    
struct ScopedAllocator {
    void *Allocate(size_t size) { return memory->Allocate(size); }
  
    ScopedMemory *memory;
}; // struct ScopedAllocator
    
} // namespace base
    
} // namespace mai

#endif // MAI_BASE_ALLOCATORS_H_
