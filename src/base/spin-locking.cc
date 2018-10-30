#include "base/spin-locking.h"
#include <thread>

namespace mai {

namespace base {

/*static*/ void SpinLocking::Lock(SpinMutex *mutex) {
    for (;;) {
        int expected = 0;
        if (mutex->compare_exchange_strong(expected, -1)) {
            return;
        }

        for (int n = 1; n < kSpinCount; n <<= 1) {
            for (int i = 0; i < n; i++) {
                // spin
                __asm__ ("pause");
            }

            expected = 0;
            if (mutex->compare_exchange_strong(expected, -1)) {
                return;
            }
        }

        std::this_thread::yield();
    }
}

/*static*/ void SpinLocking::Unlock(SpinMutex *mutex) {
    for (;;) {
        int expected = -1;
        if (mutex->compare_exchange_strong(expected, 0)) {
            return;
        }
    }
}

/*static*/ void RWSpinLocking::ReadLock(SpinMutex *mutex) {
    for (;;) {
        int readers = mutex->load(std::memory_order_relaxed);
        int expected = readers;
        if (readers > 0 &&
            mutex->compare_exchange_strong(expected, readers - 1)) {
            return;
        }
        for (int n = 1; n < kSpinCount; n <<= 1) {
            for (int i = 0; i < n; ++i) {
                __asm__("pause");
            }
            int readers = mutex->load(std::memory_order_relaxed);
            int expected = readers;
            if (readers > 0 &&
                mutex->compare_exchange_strong(expected, readers - 1)) {
                return;
            }
        }
        std::this_thread::yield();
    }
}

/*static*/ void RWSpinLocking::WriteLock(SpinMutex *mutex) {
    for (;;) {
        int expected = kLockBais;
        if (mutex->load(std::memory_order_relaxed) == kLockBais &&
            mutex->compare_exchange_strong(expected, 0)) {
            return;
        }
        for (int n = 1; n < kSpinCount; n <<= 1) {
            for (int i = 0; i < n; ++i) {
                __asm__("pause");
            }
            int expected = kLockBais;
            if (mutex->load(std::memory_order_relaxed) == kLockBais &&
                mutex->compare_exchange_strong(expected, 0)) {
                return;
            }
        }
        std::this_thread::yield();
    }
}

/*static*/ void RWSpinLocking::Unlock(SpinMutex *mutex) {
    int readers = mutex->load(std::memory_order_relaxed);
    if (readers == 0) {
        mutex->store(kLockBais, std::memory_order_relaxed);
        return;
    }

    for (;;) {
        int expected = readers;
        if (mutex->compare_exchange_strong(expected, readers + 1)) {
            return;
        }
        readers = mutex->load(std::memory_order_relaxed);
    }
}

} // namespace base

} // namespace mai
