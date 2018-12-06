#include "core/lru-cache-v1.h"
#include "base/slice.h"
#include "mai/env.h"
#include "gtest/gtest.h"
#include <thread>

namespace mai {
    
namespace core {
    
using ::mai::base::Slice;
    
class LRUCacheTest : public ::testing::Test {
public:
    static Error Load(std::string_view key, LRUHandle **result, void *arg0, void *) {
        auto h = LRUHandle::New(key, 4, reinterpret_cast<uintptr_t>(arg0));
        if (!h) {
            return MAI_CORRUPTION("Out of memory!");
        }
        h->AddRef();
        *result = h;
        return Error::OK();
    }

    Env *env_ = Env::Default();
};

TEST_F(LRUCacheTest, Sanity) {
    LRUCache cache(env_->GetLowLevelAllocator(), 7);
    
    auto h = LRUHandle::New("aaa", sizeof(int), 100);
    cache.Insert(h->key(), h, nullptr);
    
    h = LRUHandle::New("bbb", sizeof(int), 200);
    cache.Insert(h->key(), h, nullptr);
    
    base::intrusive_ptr<LRUHandle> handle(cache.Get("aaa"));
    ASSERT_FALSE(handle.is_null());
    ASSERT_EQ(100, handle->id);
    
    handle = cache.Get("bbb");
    ASSERT_FALSE(handle.is_null());
    ASSERT_EQ(200, handle->id);
    
    handle.reset(nullptr);
}
    
TEST_F(LRUCacheTest, AutoPurpe) {
    LRUCache cache(env_->GetLowLevelAllocator(), 7);
    for (int i = 0; i < 8; ++i) {
        std::string key(base::Slice::Sprintf("k.%d", i));
        auto h = LRUHandle::New(key, sizeof(int), (i + 1) * 100);
        cache.Insert(h->key(), h, nullptr);
    }

    ASSERT_EQ(7, cache.size());
    ASSERT_EQ(7, cache.capacity());
    
    std::string k0("k.0");
    ASSERT_EQ(nullptr, cache.Get(k0));
    
    for (int i = 1; i < 8; ++i) {
        std::string key(base::Slice::Sprintf("k.%d", i));
        auto h = cache.Get(key);
        ASSERT_NE(nullptr, h);
        ASSERT_EQ((i + 1) * 100, h->id);
    }
}
    
TEST_F(LRUCacheTest, InsertRehash) {
    LRUCache cache(env_->GetLowLevelAllocator(), 1237);
    for (int i = 0; i < 10000; ++i) {
        std::string key(Slice::Sprintf("k.%d", i));
        auto h = LRUHandle::New(key, sizeof(int), i);
        cache.Insert(h->key(), h, nullptr);
    }
}
    
TEST_F(LRUCacheTest, ConcurrentInsert) {
    LRUCache cache(env_->GetLowLevelAllocator(), 1237);

    std::thread worker_thrd[4];
    for (int i = 0; i < arraysize(worker_thrd); ++i) {
        worker_thrd[i] = std::thread([&](auto slot) {
            auto base = slot * 10000;
            for (int j = base; j < base + 10000; ++j) {
                std::string key(Slice::Sprintf("k.%d", j));
                auto h = LRUHandle::New(key, sizeof(int), j);
                cache.Insert(h->key(), h, nullptr);
            }
        }, i);
    }

    for (auto &thrd : worker_thrd) {
        thrd.join();
    }
    
    int hit = 0;
    for (int i = 0; i < 40000; ++i) {
        std::string key(Slice::Sprintf("k.%d", i));
        auto h = cache.Get(key);
        if (h) {
            ASSERT_EQ(i, h->id);
            ++hit;
        }
    }
    ASSERT_EQ(1237, hit);
}
    
TEST_F(LRUCacheTest, ConcurrentGet) {
    LRUCache cache(env_->GetLowLevelAllocator(), 1237);
    std::atomic<int> current(0);
    
    std::thread wr_thrd([&cache, &current] () {
        for (int i = 0; i < 40000; ++i) {
            std::string key(Slice::Sprintf("k.%d", i));
            auto h = LRUHandle::New(key, sizeof(int), i);
            cache.Insert(h->key(), h, nullptr);
            current.store(i, std::memory_order_release);
        }
    });
    
    int hit = 0;
    while (true) {
        auto n = current.load(std::memory_order_acquire);
        if (!n) {
            continue;
        }
        std::string key(Slice::Sprintf("k.%d", n));
        auto h = cache.Get(key);
        if (h) {
            ASSERT_EQ(h->id, n);
            ++hit;
        }
        if (n == 40000 - 1) {
            break;
        }
    }
    wr_thrd.join();
    ASSERT_GT(hit, 20000);
}
    
TEST_F(LRUCacheTest, ConcurrentGetOrLoad) {
    LRUCache cache(env_->GetLowLevelAllocator(), 10000);
    
    std::thread work_thrd([&cache] () {
        for (int i = 0; i < 40000; ++i) {
            auto id = rand() % 40000;
            std::string key(Slice::Sprintf("k.%d", id));
            base::intrusive_ptr<LRUHandle> h;
            Error rs = cache.GetOrLoad(key, &h, nullptr, &Load,
                                       reinterpret_cast<void *>(id));
            ASSERT_TRUE(rs.ok()) << rs.ToString();
            ASSERT_FALSE(h.is_null());
            ASSERT_EQ(id, h->id);
        }
    });
    for (int i = 0; i < 40000; ++i) {
        auto id = rand() % 40000;
        std::string key(Slice::Sprintf("k.%d", id));
        base::intrusive_ptr<LRUHandle> h;
        Error rs = cache.GetOrLoad(key, &h, nullptr, &Load,
                                   reinterpret_cast<void *>(id));
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        ASSERT_FALSE(h.is_null());
        ASSERT_EQ(id, h->id);
    }
    
    work_thrd.join();
}
    
TEST_F(LRUCacheTest, ConcurrentGetOrLoadV2) {
    LRUCache cache(env_->GetLowLevelAllocator(), 1237);
    
    std::thread work_thrd([&cache] () {
        for (int i = 0; i < 40000; ++i) {
            auto id = rand() % 1237;
            std::string key(Slice::Sprintf("k.%d", id));
            base::intrusive_ptr<LRUHandle> h;
            Error rs = cache.GetOrLoad(key, &h, nullptr, &Load,
                                       reinterpret_cast<void *>(id));
            ASSERT_TRUE(rs.ok()) << rs.ToString();
            ASSERT_FALSE(h.is_null());
            ASSERT_EQ(id, h->id);
        }
    });
    for (int i = 0; i < 40000; ++i) {
        auto id = rand() % 1237;
        std::string key(Slice::Sprintf("k.%d", id));
        base::intrusive_ptr<LRUHandle> h;
        Error rs = cache.GetOrLoad(key, &h, nullptr, &Load,
                                   reinterpret_cast<void *>(id));
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        ASSERT_FALSE(h.is_null());
        ASSERT_EQ(id, h->id);
    }
    
    work_thrd.join();
}
    
} // namespace core
    
} // namespace mai
