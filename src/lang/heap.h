#pragma once
#ifndef MAI_LANG_HEAP_H_
#define MAI_LANG_HEAP_H_

#include "lang/space.h"
#include "base/base.h"
#include <map>
#include <mutex>
#include <atomic>

namespace mai {

namespace lang {

class Heap final {
public:
    static constexpr uint32_t kPinned   = 0x1;
    static constexpr uint32_t kLarge    = 0x1;
    static constexpr uint32_t kOld      = 1u << 1;
    static constexpr uint32_t kMetadata = 1u << 2;

    // Create a Heap object
    Heap(Allocator *lla);
    // Destroy heap object
    ~Heap();
    
    // Initialize heap: test every space allocation
    Error Initialize(size_t new_space_initial_size);
    
    // Allocate heap object
    AllocationResult Allocate(size_t size, uint32_t flags);
    
    // Test object in new-geneation
    ALWAYS_INLINE bool InNewArea(Any *host) const {
        return new_space_->Contains(DCHECK_NOTNULL(reinterpret_cast<Address>(host)));
    }
    
    // Test object in old-geneation
    ALWAYS_INLINE bool InOldArea(Any *host) const {
        SpaceKind space = FromObject(host);
        return space == kOldSpace || space == kLargeSpace;
    }
    
    // Test object's generation
    ALWAYS_INLINE SpaceKind FromObject(Any *object) const {
        return FromAddress(reinterpret_cast<Address>(object));
    }

    // Test address's generation
    SpaceKind FromAddress(Address addr) const {
        if (new_space_->Contains(DCHECK_NOTNULL(addr))) {
            return kNewSpace;
        }
        return PageHeader::FromAddress(addr)->owner_space();
    }
    
    struct TotalMemoryUsage {
        size_t rss;
        size_t used;
        
        float GetUsageRate() const {
            return static_cast<float>(used)/static_cast<float>(rss);
        }
    };

    TotalMemoryUsage ApproximateMemoryUsage() const;

    NewSpace *new_space() const { return new_space_.get(); }
    
    OldSpace *old_space() const { return old_space_.get(); }
    
    LargeSpace *large_space() const { return large_space_.get(); }
    
    float GetNewSpaceUsedRate() const {
        return static_cast<float>(new_space_->GetUsedSize()) /
               static_cast<float>(new_space()->original_area()->size());
    }
    
    size_t GetOldSpaceUsedSize() const {
        return old_space_->used_size() + large_space_->used_size();
    }
    
    DEF_VAL_GETTER(HeapColor, initialize_color);
    DEF_VAL_GETTER(HeapColor, finalize_color);
    
    uint32_t initialize_color_tags() const { return static_cast<uint32_t>(initialize_color()); }
    
    uint32_t finalize_color_tags() const { return static_cast<uint32_t>(finalize_color()); }
    
    void SwapColors() { std::swap(initialize_color_, finalize_color_); }
    
    Any *MoveNewSpaceObject(Any *object, bool promote);
    Any *MoveOldSpaceObject(Page *original, Any *object, Page *dest);

    // TEST functions:
    void TEST_set_trap(AllocationResult::Result trap, int count) {
        test_trap_ = trap;
        test_trap_count_ = count;
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Heap);
private:
    std::unique_ptr<NewSpace> new_space_; // New generation space
    std::unique_ptr<OldSpace> old_space_; // Old generation space
    std::unique_ptr<LargeSpace> large_space_; // Large object area(Old-Generation)
    std::mutex mutex_; // mutex
    
    AllocationResult::Result test_trap_ = AllocationResult::OK; // Trap for test
    int test_trap_count_ = 0; // Trap counter for test
    
    HeapColor initialize_color_ = kColorWhite; // Initialize object color for GC
    HeapColor finalize_color_ = kColorBlack; // Finalize object color for GC
}; // class Heap

} // namespace lang

} // namespace mai


#endif // MAI_LANG_HEAP_H_
