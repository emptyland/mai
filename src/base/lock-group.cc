#include "base/lock-group.h"
#include "base/hash.h"
#include <mutex>

namespace mai {
    
namespace base {
    
class LockGroup::StripeImpl final : public LockGroup::Stripe {
public:
    StripeImpl(LockGroup *owns)
        : owns_(owns) {
    }
    
    virtual ~StripeImpl() {}
    
    virtual bool TryLock() override {
        bool rv = mutex_.try_lock();
        if (rv) {
            owns_->locks_.fetch_add(1);
        }
        return rv;
    }
    
    virtual void Lock() override {
        mutex_.lock();
        owns_->locks_.fetch_add(1);
    }
    
    virtual void Unlock() override {
        mutex_.unlock();
        owns_->locks_.fetch_sub(1);
    }
    
    virtual void Wait() override {
        std::unique_lock<std::mutex> lock(mutex_, std::adopt_lock);
        owns_->waits_.fetch_add(1);
        cv_.wait(lock);
        owns_->waits_.fetch_sub(1);
        lock.release();
    }
    
    virtual bool WaitFor(uint64_t micro_seds) override {
        std::unique_lock<std::mutex> lock(mutex_, std::adopt_lock);
        owns_->waits_.fetch_add(1);
        auto rv = cv_.wait_for(lock, std::chrono::microseconds(micro_seds));
        owns_->waits_.fetch_sub(1);
        lock.release();
        return rv == std::cv_status::no_timeout;
    }
    
    virtual void Notify(bool all) override {
        if (all) {
            cv_.notify_all();
        } else {
            cv_.notify_one();
        }
    }
    
private:
    LockGroup *const owns_;
    std::mutex mutex_;
    std::condition_variable cv_;
}; // class StripeImpl

LockGroup::LockGroup(size_t n_stripes, bool lazy_init)
    : n_stripes_(n_stripes)
    , lazy_init_(lazy_init)
    , stripes_(new std::atomic<Stripe *>[n_stripes])
    , locks_(0)
    , waits_(0) {
    if (!lazy_init_) {
        for (size_t i = 0; i < n_stripes_; ++i) {
            stripes_[i].store(new StripeImpl(this), std::memory_order_relaxed);
        }
    } else {
        for (size_t i = 0; i < n_stripes_; ++i) {
            stripes_[i].store(nullptr, std::memory_order_relaxed);
        }
    }
}
    
LockGroup::~LockGroup() {
    for (size_t i = 0; i < n_stripes_; ++i) {
        auto stripe = stripes_[i].load(std::memory_order_relaxed);
        delete stripe;
    }
}

LockGroup::Stripe *LockGroup::Get(size_t idx) {
    if (!lazy_init_) {
        return stripes_[idx].load(std::memory_order_relaxed);
    }
    
    return nullptr;
}

LockGroup::Stripe *LockGroup::GetByKey(std::string_view key) {
    size_t idx = Hash::Js(key.data(), key.size()) % n_stripes_;
    return Get(idx);
}
    
} // namespace base
    
} // namespace mai
