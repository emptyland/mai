#ifndef MAI_BW_MAPPING_TABLE_H_
#define MAI_BW_MAPPING_TABLE_H_

#include "bw/bw-nodes.h"
#include <memory>

namespace mai {
namespace core {
class InternalKeyComparator;
}; // namespace core
namespace bw {

class MappingTable final {
public:
    MappingTable(const core::InternalKeyComparator *ikcmp, size_t inital_max_page)
        : ikcmp_(DCHECK_NOTNULL(ikcmp))
        , max_pages_(inital_max_page)
        , next_pid_(0)
        , pages_(new PageSlot[max_pages_]) {
        for (size_t i = 0; i < max_pages_; ++i) {
            pages_[i].in_memory.store(nullptr, std::memory_order_relaxed);
            pages_[i].offset.store(-1, std::memory_order_relaxed);
        }
    }
    
    DeltaKeyNode *NewDeltaNode(const core::KeyBoundle *key, DeltaNode *base,
                               Pid slibing);
    
    Pid NextPid() { return next_pid_.fetch_add(1); }
private:
    struct PageSlot {
        std::atomic<Node *>   in_memory;
        std::atomic<uint64_t> offset;
    };
    
    const core::InternalKeyComparator *const ikcmp_;
    size_t max_pages_;
    std::atomic<Pid> next_pid_;
    std::unique_ptr<PageSlot[]> pages_;
}; // class MappingTable
    
    
} // namespace bw
    
} // namespace mai

#endif // MAI_BW_MAPPING_TABLE_H_
