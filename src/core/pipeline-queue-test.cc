#include "core/pipeline-queue.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace core {

TEST(PipelineQueueTest, Sanity) {
    PipelineQueue<int> q;
    q.Add(1);
    q.Add(2);
    q.Add(3);
    
    int v;
    ASSERT_TRUE(q.Take(&v));
    ASSERT_EQ(1, v);
    
    ASSERT_TRUE(q.Take(&v));
    ASSERT_EQ(2, v);
    
    ASSERT_TRUE(q.Take(&v));
    ASSERT_EQ(3, v);
}
    
TEST(PipelineQueueTest, PeekAll) {
    PipelineQueue<int> q;
    q.Add(111);
    q.Add(222);
    q.Add(333);
    
    std::vector<int> values{1,2,3};
    q.PeekAll(&values);
    
    ASSERT_EQ(6, values.size());
    EXPECT_EQ(1, values[0]);
    EXPECT_EQ(2, values[1]);
    EXPECT_EQ(3, values[2]);
    
    EXPECT_EQ(111, values[3]);
    EXPECT_EQ(222, values[4]);
    EXPECT_EQ(333, values[5]);
}
    
} // namespace core
    
} // namespace mai
