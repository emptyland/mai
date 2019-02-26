#include "base/scoped-arena.h"
#include "gtest/gtest.h"

namespace mai {

namespace base {
    
class ScopedArenaTest : public ::testing::Test {
public:
    
    ScopedArena arena_;
};
    
TEST_F(ScopedArenaTest, Sanity) {
    uint8_t *p = static_cast<uint8_t *>(arena_.Allocate(12));
    uint8_t *x = static_cast<uint8_t *>(arena_.Allocate(4));
    ASSERT_EQ(12, x - p);
    
    p = static_cast<uint8_t *>(arena_.Allocate(4));
    ASSERT_EQ(4, p - x);
}
    
TEST_F(ScopedArenaTest, LargeAllocation) {
    uint8_t *p = static_cast<uint8_t *>(arena_.Allocate(12));
    ASSERT_NE(nullptr, p);
    ASSERT_TRUE(arena_.chunks_.empty());
    
    p = static_cast<uint8_t *>(arena_.Allocate(arraysize(arena_.buf_) + 1));
    ASSERT_NE(nullptr, p);
    ASSERT_EQ(1, arena_.chunks_.size());
    ASSERT_EQ(p, arena_.chunks_[0]);
}
    
TEST_F(ScopedArenaTest, TotalBufAllocation) {
    uint8_t *p = static_cast<uint8_t *>(arena_.Allocate(arraysize(arena_.buf_)));
    ASSERT_NE(nullptr, p);
    ASSERT_TRUE(arena_.chunks_.empty());
    ASSERT_EQ(p, arena_.buf_);
    
    p = static_cast<uint8_t *>(arena_.Allocate(4));
    ASSERT_NE(nullptr, p);
    ASSERT_EQ(1, arena_.chunks_.size());
    ASSERT_EQ(p, arena_.chunks_[0]);
}
    
} // namespace base
    
} // namespace mai
