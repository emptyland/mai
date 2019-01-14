#ifndef MAI_BASE_ZONE_H_
#define MAI_BASE_ZONE_H_

#include "base/base.h"
#include "mai/allocator.h"
#include <memory>

namespace mai {
    
namespace base {
    
class Arena;
    
class Zone final : public Allocator {
public:
    Zone(Allocator *ll_allocator, size_t max_cache_size);
    virtual ~Zone() override;
    
    DEF_VAL_GETTER(size_t, page_size);
    DEF_VAL_GETTER(size_t, max_cache_size);
    
    Arena *NewArena(size_t limit_size, Arena *old_arena = nullptr);
    Arena *default_arena() const;
    
    size_t GetCacheSize() const;
    size_t GetTotalMameoryUsage() const;
    
    virtual void *Allocate(size_t size,
                           size_t alignment = sizeof(max_align_t)) override;
    virtual size_t granularity() override;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Zone);
private:
    struct Page;
    struct ShadowPage;
    class ArenaImpl;
    class Core;
    
    virtual void Free(const void *chunk, size_t size) override;
    
    Allocator *const ll_allocator_;
    size_t const page_size_;
    size_t const max_cache_size_;
    
    std::unique_ptr<Core> core_;
}; // class Zone
    
} // namespace base
    
} // namespace mai


#endif // MAI_BASE_ZONE_H_
