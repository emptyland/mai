#pragma once
#ifndef MAI_BASE_LAZY_INSTANCE_H_
#define MAI_BASE_LAZY_INSTANCE_H_

#include "base/base.h"
#include "mai/at-exit.h"
#include <atomic>
#include <thread>

namespace mai {

namespace base {

template<class T>
class DefaultLazyPolicy {
public:
    static T *New(void *chunk) { return new (chunk) T; }
    static void Delete(void *obj) { static_cast<T*>(obj)->~T(); }

    DISALLOW_IMPLICIT_CONSTRUCTORS(DefaultLazyPolicy);
}; // template<class T> class DefaultLazyPolicy


template<class T, class Policy = DefaultLazyPolicy<T>>
class LazyInstance {
    static const uintptr_t kPendingMask = 1;
    static const uintptr_t kCreatedMask = ~kPendingMask;

public:
    LazyInstance() : inst_(0) {}

    T *Get() {
        if (!(inst_.load(std::memory_order_acquire) & kCreatedMask) && NeedInit()) {
            Install();
        }
        return Instance();
    }

    T *operator -> () { return Get(); }

    DISALLOW_IMPLICIT_CONSTRUCTORS(LazyInstance);
private:
    bool NeedInit() {
        uintptr_t expect = 0;
        if (inst_.compare_exchange_strong(expect, kPendingMask)) {
            return true;
        }
        while (inst_.load(std::memory_order_acquire) == kPendingMask) {
            std::this_thread::yield();
        }
        return false;
    }

    void Install() {
        T *inst = Policy::New(body_);
        AtExit::This()->Register(&Policy::Delete, inst);
        inst_.store(reinterpret_cast<uintptr_t>(inst), std::memory_order_release);
    }

    T *Instance() { return reinterpret_cast<T*>(inst_.load(std::memory_order_relaxed)); }

    std::atomic<uintptr_t> inst_;
    char body_[sizeof(T)];
}; // class LazyInstance

} // namespace base

} // namespace mai

#endif // MAI_BASE_LAZY_INSTANCE_H_
