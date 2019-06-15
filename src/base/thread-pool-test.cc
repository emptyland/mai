#include "base/base.h"
#include "gtest/gtest.h"
#include <thread>
#include <future>
#include <atomic>
#include <condition_variable>
#include <deque>

namespace mai {
    
namespace base {
    
class Job {
public:
    virtual void Run() = 0;
};

template<class T>
class BlockingQueue {
public:
    BlockingQueue(size_t limit)
        : limit_(limit) {
    }

    // single writer
    void Add(const T &value) {
        std::unique_lock<std::mutex> lock(mutex_);
        while (q_.size() >= limit_) {
            not_full_cv_.wait(lock, [&]{ return q_.size() < limit_; });
        }
        q_.push_front(value);
        not_empty_cv_.notify_one();
    }
    
    // mutli-reader
    T Take() {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_cv_.wait(lock, [=]{ return !q_.empty(); });
        T result = std::move(q_.back());
        q_.pop_back();
        not_full_cv_.notify_one();
        return result;
    }
    
    std::tuple<bool, T> Peek() { return {}; }
    
    std::tuple<bool, T> Poll() { return {}; }
    
private:
    //std::atomic<Node *> free_;
    size_t const limit_;
    std::mutex mutex_;
    std::condition_variable not_empty_cv_;
    std::condition_variable not_full_cv_;
    std::deque<T> q_;
};
    

TEST(ThreadPool, BlockingQueue) {
    BlockingQueue<int> queue(4);
    
    queue.Add(1);
    ASSERT_EQ(1, queue.Take());
    
    queue.Add(2);
    queue.Add(3);
    queue.Add(4);
    ASSERT_EQ(2, queue.Take());
    ASSERT_EQ(3, queue.Take());
    ASSERT_EQ(4, queue.Take());
}
    
TEST(ThreadPool, BlockingQueueOne2One) {
    BlockingQueue<int> queue(1);
    
    std::thread thd([](auto q) {
        int n;
        int i = 0;
        while ((n = q->Take()) >= 0) {
            ASSERT_EQ(i++, n);
        }
    }, &queue);
    
    for (int i = 0; i < 10000; ++i) {
        queue.Add(i);
    }
    queue.Add(-1);
    thd.join();
}
    
TEST(ThreadPool, BlockingQueue1To4) {
    BlockingQueue<int> queue(100);
    
    std::thread thd[3];
    for (size_t i = 0; i < arraysize(thd); ++i) {
        thd[i] = std::thread([] (auto q) {
            int n;
            while ((n = q->Take()) >= 0) {
                ASSERT_LT(n, 10000);
            }
        }, &queue);
    }

    for (int i = 0; i < 10000; ++i) {
        queue.Add(i);
    }
    
    for (size_t i = 0; i < arraysize(thd); ++i) {
        queue.Add(-1);
    }
    for (auto &t : thd) {
        t.join();
    }
}
    
}
    
}
