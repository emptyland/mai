#include "base/ebr.h"
#include "gtest/gtest.h"
#include <thread>

namespace mai {
    
namespace base {
    
static void Delete0(void *chunk, void *) {
    ::free(chunk);
}
    
TEST(EbrTest, Sanity) {
    EbrGC gc(&Delete0, nullptr);
    auto rs = gc.Init(Env::Default());
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    gc.Register();
    gc.Enter();
    
    gc.Exit();
}
    
TEST(EbrTest, Writer) {
    EbrGC gc(&Delete0, nullptr);
    auto rs = gc.Init(Env::Default());
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    gc.Register();
    gc.Limbo(::malloc(sizeof(int)));
    gc.Cycle();
}
    
struct TestNode {
    int val;
    TestNode *next;
};
    
static void Delete1(void *chunk, void *) {
    auto n = static_cast<TestNode *>(chunk);
    delete n;
}
    
TEST(EbrTest, ConcurrentReadWrite) {
    EbrGC gc(&Delete1, nullptr);
    auto rs = gc.Init(Env::Default());
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    std::atomic<TestNode *> list(new TestNode{0, nullptr});
    std::atomic<int> current(0);
    
    std::thread writer([&list, &current](EbrGC *gc) {
        for (int i = 1; i < 40000; ++i) {
            TestNode *n = new TestNode{i, nullptr};
            TestNode *head;
            do {
                head = list.load(std::memory_order_relaxed);
                n->next = head;
            } while (!list.compare_exchange_weak(head, n));
            current.store(i, std::memory_order_release);
            gc->Cycle();
        }
    }, &gc);
    std::thread deleter([&list, &current](EbrGC *gc) {
        auto trigger = 1;
        while (trigger >= 1 && trigger < 40000 - 1) {
            TestNode *old = list.load(std::memory_order_acquire);
            if (old->val % 3 == 0) {
                TestNode *n;
                TestNode *head;
                do {
                    head = list.load(std::memory_order_relaxed);
                    n = head->next;
                } while (!list.compare_exchange_weak(head, n));
                gc->Limbo(head);
            }
            gc->Cycle();
            trigger = current.load(std::memory_order_acquire);
        }
    }, &gc);
    std::thread reader([&list, &current](EbrGC *gc) {
        gc->Register();
        auto trigger = 1;
        while (trigger >= 1 && trigger < 40000 - 1) {
            gc->Enter();
            TestNode *head = list.load(std::memory_order_acquire);
            ASSERT_NE(nullptr, head);
            ASSERT_GE(head->val, 1);
            gc->Exit();
            trigger = current.load(std::memory_order_acquire);
        }
    }, &gc);
    
    writer.join();
    deleter.join();
    reader.join();
    
    auto head = list.load(std::memory_order_relaxed);
    while (head) {
        auto prev = head;
        head = head->next;
        delete prev;
    }
}
    
} // namespace base
    
} // namespace mai
