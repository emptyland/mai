#ifndef MAI_BASE_LOCK_GROUP_H_
#define MAI_BASE_LOCK_GROUP_H_

#include "base/base.h"
#include <string>
#include <string_view>
#include <atomic>
#include <memory>

namespace mai {
    
namespace base {

class LockGroup {
public:
    class Stripe {
    public:
        Stripe() {}
        virtual ~Stripe() {}
        virtual bool TryLock() = 0;
        virtual void Lock() = 0;
        virtual void Unlock() = 0;
        virtual void Wait() = 0;
        virtual bool WaitFor(uint64_t micro_seds) = 0;
        virtual void Notify(bool all = true) = 0;
        
        DISALLOW_IMPLICIT_CONSTRUCTORS(Stripe);
    }; // struct Stripe
    
    LockGroup(size_t n_stripes, bool lazy_init = false);
    ~LockGroup();
    
    size_t locks() const { return locks_.load(std::memory_order_acquire); }
    size_t waits() const { return waits_.load(std::memory_order_acquire); }
    DEF_VAL_GETTER(size_t, n_stripes);
    
    Stripe *Get(size_t idx);
    Stripe *GetByKey(std::string_view key);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(LockGroup);
private:
    class StripeImpl;
    
    const size_t n_stripes_;
    const bool lazy_init_;
    std::unique_ptr<std::atomic<Stripe *>[]> stripes_;
    std::atomic<size_t> locks_;
    std::atomic<size_t> waits_;
}; // class LockGroup

} // namespace base
    
} // namespace mai

#endif // MAI_BASE_LOCK_GROUP_H_
