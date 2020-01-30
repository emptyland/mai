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
    bool InNewArea(Any *host) const {
        return FromObject(host) == kNewSpace;
    }
    
    // Test object in old-geneation
    bool InOldArea(Any *host) const {
        SpaceKind space = FromObject(host);
        return space == kOldSpace || space == kLargeSpace;
    }
    
    // Test object's generation
    SpaceKind FromObject(Any *object) const {
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
    
    uint64_t NextRememberRecordSequanceNumber() { return remember_record_sequance_.fetch_add(1); }
    
    
    NewSpace *new_space() const { return new_space_.get(); }

    DISALLOW_IMPLICIT_CONSTRUCTORS(Heap);
private:
    std::unique_ptr<NewSpace> new_space_; // New generation space
    std::unique_ptr<OldSpace> old_space_; // Old generation space
    std::unique_ptr<LargeSpace> large_space_; // Large object area(Old-Generation)
    std::mutex mutex_; // mutex

    std::atomic<uint64_t> remember_record_sequance_ = 0;
    // Remember set record for old-generation -> new-generation
//    std::map<Address, RememberRecord> remember_set_;
//    std::mutex remember_set_mutex_; // For remember_set_
}; // class Heap

} // namespace lang

} // namespace mai


#endif // MAI_LANG_HEAP_H_
