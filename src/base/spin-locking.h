#ifndef MAI_BASE_SPIN_LOCKING_H_
#define MAI_BASE_SPIN_LOCKING_H_

#include "base/base.h"
#include <atomic>

namespace mai {

namespace base {

#define SPIN_LOCK_INIT 0
#define RW_SPIN_LOCK_INIT (::mai::base::RWSpinLocking::kLockBais)

typedef std::atomic<int> SpinMutex;
typedef std::atomic<int> SpinRwMutex;

struct SpinLocking final {
    constexpr static const int kSpinCount = 2 * 1024;

    static void Lock(SpinMutex *mutex);

    static void Unlock(SpinMutex *mutex);

    DISALLOW_ALL_CONSTRUCTORS(SpinLocking)
}; // struct SpinLocking

class SpinLock final {
public:
    SpinLock(SpinMutex *mutex) : mutex_(mutex) { SpinLocking::Lock(mutex_); }
    ~SpinLock() { SpinLocking::Unlock(mutex_); }

private:
    SpinMutex * const mutex_;
};

struct RWSpinLocking final {
    enum {
        kSpinCount = 2 * 1024,
        kLockBais  = 0x010000000,
    };

    static void ReadLock(SpinMutex *mutex);

    static void WriteLock(SpinMutex *mutex);

    static void Unlock(SpinMutex *mutex);

    DISALLOW_ALL_CONSTRUCTORS(RWSpinLocking)
}; // struct RWSpinLocking


class ReaderSpinLock final {
public:
    ReaderSpinLock(SpinMutex *mutex)
        : mutex_(mutex) { RWSpinLocking::ReadLock(mutex_); }
    ~ReaderSpinLock() { RWSpinLocking::Unlock(mutex_); }

    DISALLOW_IMPLICIT_CONSTRUCTORS(ReaderSpinLock);
private:
    SpinMutex * const mutex_;
}; // class RWSpinLock


class WriterSpinLock final {
public:
    WriterSpinLock(SpinMutex *mutex)
        : mutex_(mutex) { RWSpinLocking::WriteLock(mutex_); }
    ~WriterSpinLock() { RWSpinLocking::Unlock(mutex_); }

    DISALLOW_IMPLICIT_CONSTRUCTORS(WriterSpinLock);
private:
    SpinMutex * const mutex_;
}; // class RWSpinLock

} // namespace base

} // namespace mai

#endif // MAI_BASE_SPIN_LOCKING_H_
