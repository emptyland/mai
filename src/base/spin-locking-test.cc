#include "base/spin-locking.h"
#include "gtest/gtest.h"
#include <thread>
#include <future>

namespace mai {

namespace base {

TEST(SpinLockingTest, Sanity) {
    std::atomic<int> mutex(0);

    SpinLocking::Lock(&mutex);
    ASSERT_EQ(-1, mutex.load());

    SpinLocking::Unlock(&mutex);
    ASSERT_EQ(0, mutex.load());
}

TEST(SpinLockingTest, UnlockOnly) {
    std::atomic<int> mutex(-1);

    SpinLocking::Unlock(&mutex);
    ASSERT_EQ(0, mutex.load());
}

TEST(SpinLockingTest, Sync) {
    SpinMutex mutex(SPIN_LOCK_INIT);
    int p = 0;

    auto h1 = std::async(std::launch::async, [&mutex](int *p) {
        for (int i = 0; i < 10000; ++i) {
            SpinLocking::Lock(&mutex);
            (*p)++;
            SpinLocking::Unlock(&mutex);
        }
    }, &p);
    auto h2 = std::async(std::launch::async, [&mutex](int *p) {
        for (int i = 0; i < 10000; ++i) {
            SpinLocking::Lock(&mutex);
            (*p)--;
            SpinLocking::Unlock(&mutex);
        }
    }, &p);
    h1.get();
    h2.get();
    ASSERT_EQ(0, p);
}

TEST(SpinLockingTest, RWLockingRead) {
    SpinMutex mutex(RW_SPIN_LOCK_INIT);

    RWSpinLocking::ReadLock(&mutex);
    ASSERT_EQ(RW_SPIN_LOCK_INIT - 1, mutex.load());
    RWSpinLocking::ReadLock(&mutex);
    ASSERT_EQ(RW_SPIN_LOCK_INIT - 2, mutex.load());

    RWSpinLocking::Unlock(&mutex);
    ASSERT_EQ(RW_SPIN_LOCK_INIT - 1, mutex.load());
    RWSpinLocking::Unlock(&mutex);
    ASSERT_EQ(RW_SPIN_LOCK_INIT, mutex.load());
}

TEST(SpinLockingTest, RWLockingWrite) {
    SpinMutex mutex(RW_SPIN_LOCK_INIT);

    RWSpinLocking::WriteLock(&mutex);
    ASSERT_EQ(0, mutex.load());

    RWSpinLocking::Unlock(&mutex);
    ASSERT_EQ(RW_SPIN_LOCK_INIT, mutex.load());
}

TEST(SpinLockingTest, RWLockWWSync) {
    SpinMutex mutex(RW_SPIN_LOCK_INIT);
    int p = 0;

    auto h1 = std::async(std::launch::async, [&mutex](int *p) {
        for (int i = 0; i < 10000; ++i) {
            RWSpinLocking::WriteLock(&mutex);
            (*p)++;
            RWSpinLocking::Unlock(&mutex);
        }
    }, &p);
    auto h2 = std::async(std::launch::async, [&mutex](int *p) {
        for (int i = 0; i < 10000; ++i) {
            RWSpinLocking::WriteLock(&mutex);
            (*p)--;
            RWSpinLocking::Unlock(&mutex);
        }
    }, &p);
    h1.get();
    h2.get();
    ASSERT_EQ(0, p);
}

TEST(SpinLockingTest, RWLockRWSync) {
    SpinMutex mutex(RW_SPIN_LOCK_INIT);
    int p = 0;

    auto h1 = std::async(std::launch::async, [&mutex](int *p) {
        for (int i = 0; i < 10000; ++i) {
            ReaderSpinLock lock(&mutex);
            (*p)++;
        }
    }, &p);
    auto h2 = std::async(std::launch::async, [&mutex](int *p) {
        for (int i = 0; i < 10000; ++i) {
            WriterSpinLock lock(&mutex);
            (*p)--;
        }
    }, &p);
    h1.get();
    h2.get();
    ASSERT_EQ(0, p);
}

} // namespace base

} // namespace mai
