#include "base/base.h"
#include "gtest/gtest.h"
#include <atomic>
#include <thread>

namespace mai {

namespace demo {

template<class T>
class Channel {
public:
    bool Offer(T value) {
        for (;;) {
            if (len_.load(std::memory_order_acquire) == capacity_) {
                return false;
            }
            int old_end = end_.load(std::memory_order_acquire);
            int new_end = (old_end + 1) % capacity_;
            if (end_.compare_exchange_strong(old_end, new_end)) {
                buf_[new_end] = value;
                len_.fetch_add(1, std::memory_order_release);
                break;
            }
        }
        return true;
    }
    
    bool Peek(T *value) {
        for (;;) {
            if (len_.load(std::memory_order_acquire) == 0) {
                return false;
            }
            int old_start = start_.load(std::memory_order_acquire);
            int new_start = (old_start + 1) % capacity_;
            if (start_.compare_exchange_strong(old_start, new_start)) {
                *value = buf_[new_start];
                len_.fetch_sub(1, std::memory_order_release);
                break;
            }
        }
        return true;
    }
    
    bool Put(T value) {
        int retries = 0;
        while (!Offer(value)) {
            if (retries++ > 1024) {
                std::this_thread::sleep_for(std::chrono::nanoseconds(10));
            }
        }
        return true;
    }
    
    T Take() {
        T value(0);
        int retries = 0;
        while (!Peek(&value)) {
            if (retries++ > 1024) {
                std::this_thread::sleep_for(std::chrono::nanoseconds(10));
            }
        }
        return value;
    }
    
    const T &buf(int i) { return buf_[i]; }

    static Channel<T> *New(int capacity) {
        size_t alloc_size = sizeof(Channel<T>) + capacity * sizeof(T);
        void *chunk = ::malloc(alloc_size);
        return new (chunk) Channel<T>(capacity);
    }
private:
    Channel(int capacity)
        : capacity_(capacity)
        , len_(0)
        , start_(0)
        , end_(0) {
        memset(buf_, 0, sizeof(T) * capacity_);
    }

    const int capacity_;
    std::atomic_int len_;
    std::atomic_int start_;
    std::atomic_int end_;
    T buf_[];
};


TEST(LangDemoTest, Channel) {
    auto chan = Channel<int>::New(1);
    ASSERT_TRUE(chan->Offer(1));
    ASSERT_FALSE(chan->Offer(2));
    
    int v = 0;
    ASSERT_TRUE(chan->Peek(&v));
    ASSERT_EQ(1, v);
    ASSERT_FALSE(chan->Peek(&v));
}

TEST(LangDemoTest, ChannelNlen) {
    auto chan = Channel<int>::New(16);
    for (int i = 0; i < 16; i++) {
        ASSERT_TRUE(chan->Offer(i));
    }
    
    for (int i = 0; i < 16; i++) {
        //printf("%d\n", chan->buf(i));
        int value = 0;
        ASSERT_TRUE(chan->Peek(&value));
        ASSERT_EQ(i, value) << i;
    }
}

TEST(LangDemoTest, Channel1Reader1Writer) {
    auto chan = Channel<int>::New(16);
    std::vector<int> recived;
    
    std::thread reader([chan, &recived] () {
        int value = 0;
        while (value != -1) {
            value = chan->Take();
            recived.push_back(value);
        }
    });

    static const int kN = 10000;
    for (int i = 0; i < kN; i++) {
        chan->Put(i);
    }
    chan->Put(-1);
    reader.join();
    
    for (int i = 0; i < kN; i++) {
        ASSERT_EQ(i, recived[i]);
    }
}

TEST(LangDemoTest, ChannelNReader1Writer) {
    auto chan = Channel<int>::New(1);
    std::atomic_int count(0);
    
    std::thread reader[4];
    for (int i = 0; i < 4; i++) {
        reader[i] = std::thread([chan, &count](int i) {
            //int value = 0;
            while (chan->Take() != -1) {
                //value = chan->Take();
//                if (value >= 0) {
//                    count.fetch_add(1);
//                }
                //count.fetch_add(1);
                printf("[%d]: %d\n", i, count.fetch_add(1));
            }
        }, i);
    }
    
    static const int kN = 10000;
    for (int i = 0; i < kN; i++) {
        chan->Put(i);
    }
    
    for (int i = 0; i < 4; i++) {
        chan->Put(-1);
    }
    for (int i = 0; i < 4; i++) {
        reader[i].join();
    }
    ASSERT_EQ(kN, count.load());
}

} // namespace demo

} // namespace mai
