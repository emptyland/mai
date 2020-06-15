#ifndef MAI_BASE_ARENA_H_
#define MAI_BASE_ARENA_H_

#include "base/base.h"
#include "mai/allocator.h"

namespace mai {
    
namespace base {
    
class Arena : public Allocator {
public:
    static const uint32_t kInitZag;
    static const uint32_t kFreeZag;
    
    Arena() {}
    virtual ~Arena() override;
    
    virtual void Purge(bool reinit) = 0;
    
    virtual size_t granularity() override { return 4; }
    
    virtual size_t memory_usage() const = 0;
    
    template<class T> T *New() { return new (Allocate(sizeof(T))) T(); }
    
    template<class T> T *NewArray(size_t n) {
        return static_cast<T *>(Allocate(sizeof(T) * n));
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Arena);
private:
    virtual void Free(const void */*chunk*/, size_t /*size*/) override {}
}; // class Arena

class ArenaObject {
public:
    void *operator new (size_t n, Arena *arena) { return arena->Allocate(n); }
}; // class ArenaObject
    
} // namespace base
    
} // namespace mai

#endif // MAI_BASE_ARENA_H_
