#ifndef MAI_LANG_HEAP_H_
#define MAI_LANG_HEAP_H_

#include "lang/space.h"
#include "base/base.h"
#include <mutex>

namespace mai {

namespace lang {

class Heap final {
public:
    Heap(Allocator *lla);
    ~Heap();
    
    AllocationResult Allocate(size_t size, uint32_t tags);
 
    //AllocationResult NewString(const char *z, size_t n, uint32_t flags);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Heap);
private:
    std::unique_ptr<NewSpace> new_space_;
    std::unique_ptr<OldSpace> old_space_;
    std::unique_ptr<LargeSpace> large_space_;
    std::mutex mutex_;
}; // class Heap

} // namespace lang

} // namespace mai


#endif // MAI_LANG_HEAP_H_
