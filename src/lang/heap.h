#ifndef MAI_LANG_HEAP_H_
#define MAI_LANG_HEAP_H_

#include "lang/space.h"
#include "base/base.h"
#include <mutex>

namespace mai {

namespace lang {

class Heap final {
public:
    static constexpr uint32_t kPinned   = 0x1;
    static constexpr uint32_t kLarge    = 0x1;
    static constexpr uint32_t kOld      = 1u << 1;
    static constexpr uint32_t kMetadata = 1u << 2;

    Heap(Allocator *lla);
    ~Heap();
    
    Error Initialize(size_t new_space_initial_size);
    
    AllocationResult Allocate(size_t size, uint32_t flags);
    
    struct TotalMemoryUsage {
        size_t rss;
        size_t used;
        
        float GetUsageRate() const {
            return static_cast<float>(used)/static_cast<float>(rss);
        }
    };
    
    TotalMemoryUsage ApproximateMemoryUsage() const;
    
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
