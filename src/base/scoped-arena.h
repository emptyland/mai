#ifndef MAI_BASE_SCOPED_ARENA_H_
#define MAI_BASE_SCOPED_ARENA_H_

#include "base/arena.h"
#include <vector>

namespace mai {
    
namespace base {
    
class ScopedArena : public Arena {
public:
    ScopedArena(size_t limit = -1) : limit_(limit) {}
    virtual ~ScopedArena() override { Purge(false); }
    
    virtual void Purge(bool /*reinit*/) override;
    
    virtual size_t granularity() override { return 4; }
    
    virtual size_t memory_usage() const override { return usage_; }
    
    virtual void *Allocate(size_t size, size_t alignment = 4) override;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ScopedArena);
    
    FRIEND_UNITTEST_CASE(ScopedArenaTest, LargeAllocation);
    FRIEND_UNITTEST_CASE(ScopedArenaTest, TotalBufAllocation);
private:
    uint8_t buf_[64];
    size_t  usage_ = 0;
    size_t const limit_;
    std::vector<void *> chunks_;
}; // class ScopedArena
    
} // namespace base
    
} // namespace mai



#endif // MAI_BASE_SCOPED_ARENA_H_
